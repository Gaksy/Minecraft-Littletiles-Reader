#!/usr/bin/env python3
"""把材质包（目录 / zip）与原版素材合并成一个可直接喂给 reader 的 assets 根。

为什么需要"合并"而不是直接用材质包：
  * 我们的 block:meta -> 六面贴图 映射表是按 **1.12.2 原版** 的
    blockstates/models 解出来的（材质包大多是 1.13+ 命名，模型对不上）；
  * 但贴图文件本身基本同名（dirt.png / stone.png / glass_white.png …），
    材质包覆盖这些文件、其余回退原版，就是游戏里资源包叠加的行为。

产物（默认 assets/pack，与 assets/ 一起被 gitignore）：
  <out>/block_textures.tsv    六面贴图 + tintindex 映射表（材质包优先解析）
  <out>/block_ids.tsv         方块 id -> 名称表（原版直接复制）
  <out>/textures/<路径>.png   映射表里真正引用到的贴图，逐张挑出来复制
  <out>/pack-source.txt       记录这份产物来自哪个材质包，便于回溯

用法：
  python3 tools/build_assets_from_pack.py --pack "texture/INCEPTION texture V1.5.zip"
  python3 tools/build_assets_from_pack.py --pack ~/packs/my.zip --out assets/mypack
  python3 tools/build_assets_from_pack.py --pack ./packs/vanilla_compatible --use-pack-models

zip 与 rar 都能直接给：rar 会用系统自带的 bsdtar（或 brew 的 unar）解到临时目录。

--use-pack-models 只在材质包自带 **1.12.2 命名** 的 blockstates/models 时才有意义
（此时它连模型一起覆盖）。默认只用材质包的贴图。
"""

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_VANILLA = REPO_ROOT / "assets" / "1.12.2"
DEFAULT_OUT = REPO_ROOT / "assets" / "pack"

sys.path.insert(0, str(Path(__file__).resolve().parent))
import resolve_block_textures as rbt  # noqa: E402  (要在 sys.path 调整之后导入)


class PackSource:
    """统一的材质包读取接口：目录与 zip 都能用。"""

    def __init__(self, path):
        self.path = Path(path)
        self._zip = None
        self._temp_dir = None
        if not self.path.exists():
            raise SystemExit("找不到材质包：%s" % self.path)

        if self.path.is_file() and self.path.suffix.lower() == ".rar":
            # Python 读不了 rar，交给系统工具解到临时目录，再当普通目录用
            self._temp_dir = tempfile.TemporaryDirectory(prefix="ltpack-")
            self.path = self._extract_rar(self.path, Path(self._temp_dir.name))

        if self.path.is_dir():
            names = self._walk(self.path)
        elif zipfile.is_zipfile(self.path):
            self._zip = zipfile.ZipFile(self.path)
            names = self._zip.namelist()
        else:
            raise SystemExit("%s 既不是目录，也不是可读的 zip/rar" % self.path)

        self.prefix = self._find_pack_prefix(names)
        self.names = names

    @staticmethod
    def _walk(root):
        return [
            str(p.relative_to(root)).replace(os.sep, "/")
            for p in root.rglob("*")
        ]

    @staticmethod
    def _extract_rar(rar_path, target):
        """用 bsdtar（macOS 自带，能读 rar）或 unar 解压到 target。"""
        commands = [
            ["bsdtar", "-xf", str(rar_path), "-C", str(target)],
            ["unar", "-q", "-o", str(target), str(rar_path)],
        ]
        for command in commands:
            if shutil.which(command[0]) is None:
                continue
            done = subprocess.run(command, capture_output=True, text=True)
            if done.returncode == 0 and any(target.rglob("*")):
                return target
        raise SystemExit(
            "解压 rar 失败：需要系统自带的 bsdtar 或 brew 的 unar；"
            "也可以手动解压后 --pack <目录>"
        )

    @staticmethod
    def _find_pack_prefix(names):
        """材质包可能在 zip 根、也可能套了一层文件夹；找到含 assets/ 的那一层。"""
        candidates = []
        for name in names:
            marker = name.find("assets/")
            if marker >= 0:
                candidates.append(name[:marker])
        if not candidates:
            raise SystemExit("这个材质包里没有 assets/ 目录，可能不是资源包")
        # 取最短前缀，避免把 assets/x/y 之类的深层路径当成包根
        return min(candidates, key=len)

    def exists(self, rel):
        return (self.prefix + rel) in self.names

    def read(self, rel):
        full = self.prefix + rel
        if self._zip is not None:
            return self._zip.read(full)
        return (self.path / full).read_bytes()

    def read_full(self, name):
        """按包内完整路径读取（name 是 namelist 里的那种）。"""
        if self._zip is not None:
            return self._zip.read(name)
        return (self.path / name).read_bytes()

    def listdir(self, rel_dir):
        """列出某个目录下的条目名（不含更深的层级）。"""
        full = self.prefix + rel_dir.rstrip("/") + "/"
        found = set()
        for name in self.names:
            if not name.startswith(full):
                continue
            rest = name[len(full):]
            if rest and "/" not in rest.rstrip("/"):
                found.add(rest.rstrip("/"))
        return sorted(found)

    def texture_index(self):
        """{相对 textures 的路径（含 .png）: 包内完整路径}。

        同一路径在多个命名空间下都有时优先 minecraft（原版命名）。
        """
        index = {}
        for name in self.names:
            if not name.startswith(self.prefix + "assets/"):
                continue
            rest = name[len(self.prefix + "assets/"):]
            parts = rest.split("/")
            if len(parts) >= 4 and parts[1] == "textures" and parts[-1].endswith(".png"):
                key = "/".join(parts[2:])
                if key not in index or parts[0] == "minecraft":
                    index[key] = name
        return index


def make_pack_assets_root(pack, out_dir):
    """把材质包摊成 `<tmp>/assets/minecraft/...` 的形状，好让 Resolver 直接读。

    不落盘：这里返回一个"读得到 json"的临时根目录，只复制 json/模型。
    """
    assets_root = out_dir / "_pack_assets" / "minecraft"
    for sub in ("blockstates", "models"):
        for name in pack.listdir("assets/minecraft/%s/" % sub):
            if not name.endswith(".json"):
                continue
            target = assets_root / sub / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(pack.read("assets/minecraft/%s/%s" % (sub, name)))
        # models 下面还有 block/ 子目录
        for name in pack.listdir("assets/minecraft/%s/block/" % sub):
            if not name.endswith(".json"):
                continue
            target = assets_root / sub / "block" / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(pack.read("assets/minecraft/%s/block/%s" % (sub, name)))
    return assets_root


def find_texture(index, rel):
    """在贴图索引里找 rel（形如 blocks/dirt）对应的 png，返回包内完整路径。

    1.12 的模型写 `blocks/xxx`，1.13+ 写 `block/xxx`，两种都试一下。
    """
    rel = rel.split(":")[-1] if rel.startswith("minecraft:") else rel
    for candidate in (rel, rel.replace("blocks/", "block/"),
                      rel.replace("block/", "blocks/")):
        found = index.get(candidate + ".png")
        if found:
            return found
    return None


def read_table_rows(tsv_path):
    """读回映射表，返回 (键, 六个贴图路径) 列表。"""
    rows = []
    for line in Path(tsv_path).read_text(encoding="utf-8").splitlines():
        if not line or line.startswith("#"):
            continue
        parts = line.split("\t")
        if len(parts) < 7:
            continue
        rows.append((parts[0], [p for p in parts[1:7] if p != "-"]))
    return rows


def build(pack_path, vanilla, out_dir, use_pack_models):
    pack = PackSource(pack_path)
    if not (vanilla / "blockstates").is_dir():
        raise SystemExit("原版素材目录不完整：%s" % vanilla)

    out_dir.mkdir(parents=True, exist_ok=True)
    print("材质包：%s" % pack_path)
    texture_index = pack.texture_index()
    print("  包内根目录：%r，贴图文件 %d 个" % (pack.prefix, len(texture_index)))

    overlay_roots = []
    if use_pack_models:
        extracted = make_pack_assets_root(pack, out_dir)
        overlay_roots.append(str(extracted))
        print("  已提取材质包的 blockstates/models（%d 个 json）"
              % len(list(extracted.rglob("*.json"))))

    table_path = out_dir / "block_textures.tsv"
    keys = rbt.dump_table(str(vanilla), str(table_path),
                          overlay_roots=overlay_roots)
    print("  映射表：%d 个键 -> %s" % (keys, table_path))

    # 只把映射表真正引用到的贴图挑出来，避免把几百 MB 的 PBR / CTM 也抄一遍
    from_pack = from_vanilla = missing = 0
    missing_names = []
    for _, paths in read_table_rows(table_path):
        for rel in paths:
            target = out_dir / "textures" / (rel + ".png")
            if target.exists():
                continue
            pack_full = find_texture(texture_index, rel)
            if pack_full is not None:
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(pack.read_full(pack_full))
                from_pack += 1
                continue
            source = vanilla / "textures" / (rel + ".png")
            if source.is_file():
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(source, target)
                from_vanilla += 1
                continue
            missing += 1
            missing_names.append(rel)

    shutil.copyfile(vanilla / "block_ids.tsv", out_dir / "block_ids.tsv")
    (out_dir / "pack-source.txt").write_text(
        "材质包：%s\n包内根目录：%s\n模型叠加：%s\n"
        % (pack_path, pack.prefix or "(包根)", "是" if use_pack_models else "否"),
        encoding="utf-8",
    )
    print("  贴图：材质包 %d 张，原版兜底 %d 张，缺失 %d 张"
          % (from_pack, from_vanilla, missing))
    if missing_names:
        print("    缺失示例：%s" % ", ".join(sorted(set(missing_names))[:5]))
    print("\n完成，用这个目录跑导出：")
    print("  LITTLETILES_ASSETS=%s ./LittleTilesReader" % out_dir)
    print("  或者运行时在 \"assets root\" 那一问里直接填路径")
    return 0


def main():
    parser = argparse.ArgumentParser(description="材质包 -> reader 可用的 assets 根")
    parser.add_argument("--pack", required=True,
                        help="材质包目录，或 .zip / .rar 文件（rar 自动解压）")
    parser.add_argument("--vanilla", type=Path, default=DEFAULT_VANILLA,
                        help="原版 1.12.2 素材目录（默认 assets/1.12.2）")
    parser.add_argument("--out", type=Path, default=DEFAULT_OUT,
                        help="产物目录（默认 assets/pack）")
    parser.add_argument("--use-pack-models", action="store_true",
                        help="让材质包的 blockstates/models 也参与解析（仅 1.12.2 命名的包）")
    args = parser.parse_args()
    return build(args.pack, args.vanilla, args.out, args.use_pack_models)


if __name__ == "__main__":
    sys.exit(main())
