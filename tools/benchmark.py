#!/usr/bin/env python3
"""跑一组固定的导出基准，把结果记成 Markdown。

每个测试存档的推荐参数（区块坐标 + 扫描半径）写死在 CASES 里，
改这里就等于改"官方基准"；README 与 docs/known-issues.md 的推荐值要与它一致。

用法：
    python3 tools/benchmark.py                    # 只跑并打印
    python3 tools/benchmark.py --write docs/benchmark.md   # 追加一节到结果文件
    python3 tools/benchmark.py --case test_region_large    # 只跑其中一个

选定的导出参数（与 README 的示例命令一致）：
    语言 en-US、完整方块 y、剔除相邻面 y、居中 y、单位化 n、进度与耗时 y
（基准固定用英文界面：输出全是 ASCII，解析更稳，也顺带回归英文文案。）
"""

import argparse
import os
import platform
import re
import shutil
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass, field
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_READER = REPO_ROOT / "cmake-build-debug" / "LittleTilesReader"
DEFAULT_ASSETS = REPO_ROOT / "assets" / "1.12.2"

# 各存档的推荐参数：(目录名, 区块 x, 区块 z, 扫描半径)
CASES = [
    ("test_region", 0, 0, 1),
    ("test_region_medim", -136, 49, 5),
    ("test_region_large", -7, -26, 5),
]

# 交互式 CLI 回答顺序：先选语言，再是存档目录、x、z、半径、完整方块、剔面、居中、
# 单位化、进度、素材目录（留空 = 自动探测；材质包导出用 --assets 指向合并素材根）
CLI_LANGUAGE = "2"  # 2 = en-US：输出全是 ASCII，脚本解析更稳
CLI_ANSWERS = "y\ny\ny\nn\ny\n\n"


@dataclass
class Result:
    """一次导出的全部可记录指标。"""

    name: str
    chunk: tuple
    radius: int
    seconds: float = 0.0
    seconds_read: float = 0.0
    seconds_blocks: float = 0.0
    seconds_write: float = 0.0
    wall_seconds: float = 0.0  # 进程墙钟（含启动与写盘），用于对照 CLI 自报的耗时
    chunks_found: int = 0
    chunks_missing: int = 0
    tiles: int = 0
    blocks: int = 0
    block_faces: int = 0
    culled_faces: int = 0
    rejected_faces: int = 0
    merged_vertices: int = 0
    merged_faces: int = 0
    materials: int = 0
    textures: int = 0
    obj_bytes: int = 0
    assets_root: str = ""
    warnings: list = field(default_factory=list)

    def as_row(self) -> str:
        span = self.radius * 2 + 1
        megabytes = self.obj_bytes / 1024 / 1024
        return (
            f"| `{self.name}` | {self.chunk[0]}, {self.chunk[1]} | {self.radius} "
            f"| {span}×{span} | {self.chunks_found} | {self.tiles} | {self.blocks} "
            f"| {self.merged_faces} | {self.merged_vertices} | {megabytes:.1f} MB "
            f"| {self.seconds:.1f} s |"
        )


TABLE_HEADER = (
    "| 存档 | 区块 (x, z) | 半径 | 扫描 | chunk | tile | 普通方块 "
    "| 合并面数 | 合并顶点 | OBJ 体积 | 耗时 |\n"
    "|---|---|---|---|---|---|---|---|---|---|---|"
)


def run_case(reader: Path, assets: Path, case, work_dir: Path) -> Result:
    """在私有目录里跑一次导出，避免和用户自己的 out_file/ 打架。"""
    name, chunk_x, chunk_z, radius = case
    result = Result(name=name, chunk=(chunk_x, chunk_z), radius=radius)

    run_dir = work_dir / f"run_{name}"
    run_dir.mkdir(parents=True, exist_ok=True)
    region = REPO_ROOT / name

    stdin_text = (
        f"{CLI_LANGUAGE}\n{region}\n{chunk_x}\n{chunk_z}\n{radius}\n{CLI_ANSWERS}"
    )
    env = dict(os.environ, LITTLETILES_ASSETS=str(assets))
    started = time.monotonic()
    completed = subprocess.run(
        [str(reader)],
        input=stdin_text,
        capture_output=True,
        text=True,
        cwd=run_dir,
        env=env,
        timeout=3600,
    )
    wall_seconds = time.monotonic() - started
    output = completed.stdout + completed.stderr

    def find(pattern, group=1, default=0):
        matched = re.search(pattern, output)
        return matched.group(group) if matched else default

    result.chunks_found = int(find(r"chunks: (\d+) found"))
    result.chunks_missing = int(find(r"(\d+) missing"))
    result.tiles = int(find(r"(\d+) LittleTiles tiles"))
    result.blocks = int(find(r"\[worldblocks\] emitted (\d+) blocks"))
    result.block_faces = int(find(r"emitted \d+ blocks, (\d+) faces"))
    result.culled_faces = int(find(r"\(culled (\d+)"))
    result.rejected_faces = int(find(r"rejected by CGAL (\d+)"))
    result.merged_vertices = int(find(r"merged mesh: vertices: (\d+)"))
    result.merged_faces = int(find(r"\nfaces: (\d+)"))
    result.materials = int(find(r"materials (\d+)"))
    result.textures = int(find(r"textures written (\d+)"))
    result.assets_root = find(r"assets root: (.*)", default="(未知)").strip()
    result.seconds = float(find(r"total: ([\d.]+) s"))
    result.seconds_read = float(find(r"read & mesh ([\d.]+) s"))
    result.seconds_blocks = float(find(r"plain blocks ([\d.]+) s"))
    result.seconds_write = float(find(r"write ([\d.]+) s"))
    result.wall_seconds = wall_seconds

    if completed.returncode != 0:
        result.warnings.append(f"exit code {completed.returncode}")
    if result.merged_faces == 0:
        result.warnings.append("no merged face count parsed (output format changed?)")
    if result.rejected_faces:
        result.warnings.append(f"{result.rejected_faces} faces rejected by CGAL")

    obj_path = Path(find(r'exported merged mesh to: "(.*)"', default=""))
    if obj_path.is_file():
        result.obj_bytes = obj_path.stat().st_size
    else:
        result.warnings.append("exported OBJ not found")
    return result


def build_type_of(reader: Path) -> str:
    """从 CMakeCache 里读构建类型，读不到就标 unknown。"""
    cache = reader.parent / "CMakeCache.txt"
    if not cache.is_file():
        return "unknown"
    matched = re.search(r"^CMAKE_BUILD_TYPE:\w+=(.*)$", cache.read_text(), re.M)
    return matched.group(1).strip() or "unknown" if matched else "unknown"


def git_description() -> str:
    commit = subprocess.run(
        ["git", "-C", str(REPO_ROOT), "rev-parse", "--short", "HEAD"],
        capture_output=True,
        text=True,
    ).stdout.strip()
    dirty = subprocess.run(
        ["git", "-C", str(REPO_ROOT), "status", "--porcelain"],
        capture_output=True,
        text=True,
    ).stdout.strip()
    return commit + ("（工作区有未提交改动）" if dirty else "")


def environment_note(reader: Path) -> str:
    cpu = subprocess.run(
        ["sysctl", "-n", "machdep.cpu.brand_string"],
        capture_output=True,
        text=True,
    ).stdout.strip()
    if not cpu:
        cpu = platform.processor() or "unknown CPU"
    return (
        f"- 时间：{time.strftime('%Y-%m-%d %H:%M')}\n"
        f"- 提交：{git_description()}\n"
        f"- 构建：{build_type_of(reader)}（{reader}）\n"
        f"- 机器：{cpu} / {platform.system()} {platform.release()}\n"
        f"- 参数：完整方块 y、剔除相邻面 y、居中 y、单位化 n"
    )


def markdown_section(results, reader: Path) -> str:
    lines = [f"## {time.strftime('%Y-%m-%d %H:%M')}", "", environment_note(reader), ""]
    lines.append(TABLE_HEADER)
    lines.extend(result.as_row() for result in results)
    lines.append("")
    for result in results:
        lines.append(
            f"- `{result.name}`：总耗时 {result.seconds:.1f} s"
            f" = 读取与建网格 {result.seconds_read:.1f}"
            f" + 普通方块网格 {result.seconds_blocks:.1f}"
            f" + 写出文件 {result.seconds_write:.1f}"
            f"；普通方块面 {result.block_faces}"
            f"（剔除 {result.culled_faces}，被拒 {result.rejected_faces}）"
            f"；材质 {result.materials} / 贴图 {result.textures}"
            f"；素材根 {result.assets_root}"
        )
        for warning in result.warnings:
            lines.append(f"  - ⚠️ {warning}")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="LittleTilesReader 导出基准")
    parser.add_argument("--reader", type=Path, default=DEFAULT_READER)
    parser.add_argument("--assets", type=Path, default=DEFAULT_ASSETS,
                        help="素材根目录（相对路径会按当前 shell 目录解析成绝对路径）")
    parser.add_argument("--case", help="只跑指定存档（目录名）")
    parser.add_argument("--write", type=Path, help="把结果追加到这个 Markdown 文件")
    parser.add_argument("--keep", action="store_true", help="保留临时输出目录")
    args = parser.parse_args()

    if not args.reader.is_file():
        print(f"找不到可执行文件：{args.reader}", file=sys.stderr)
        return 1
    # 用例在私有临时目录里跑，所以素材根必须是绝对路径，否则 reader 会找不到
    args.assets = args.assets.resolve()
    if not (args.assets / "block_textures.tsv").is_file():
        print(
            f"找不到素材目录：{args.assets}（见 docs/texture-mapping.md 第 4 节）",
            file=sys.stderr,
        )
        return 1

    cases = [case for case in CASES if not args.case or case[0] == args.case]
    if not cases:
        print(f"没有匹配的存档：{args.case}", file=sys.stderr)
        return 1

    work_dir = Path(tempfile.mkdtemp(prefix="ltbench-"))
    try:
        results = []
        for case in cases:
            print(f"[benchmark] {case[0]} chunk ({case[1]}, {case[2]}) 半径 {case[3]} …")
            result = run_case(args.reader, args.assets, case, work_dir)
            print(
                f"            {result.merged_faces} 面 / {result.merged_vertices} 顶点 / "
                f"{result.obj_bytes / 1024 / 1024:.1f} MB / {result.seconds:.1f} s"
            )
            results.append(result)

        section = markdown_section(results, args.reader)
        if args.write:
            target = args.write if args.write.is_absolute() else REPO_ROOT / args.write
            if not target.exists():
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_text(
                    "# 导出基线与实测数据\n\n"
                    "由 `python3 tools/benchmark.py --write docs/benchmark.md` 生成，"
                    "每次运行追加一节。\n"
                    "各存档的推荐参数写在 `tools/benchmark.py` 的 `CASES` 里，"
                    "与 README「测试存档」表一致。\n\n",
                    encoding="utf-8",
                )
            with target.open("a", encoding="utf-8") as handle:
                handle.write(section)
            print(f"\n已追加到 {target}")
        else:
            print()
            print(section)
        return 0
    finally:
        if args.keep:
            print(f"临时输出保留在 {work_dir}")
        else:
            shutil.rmtree(work_dir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
