#!/usr/bin/env python3
"""检查导出的 OBJ 到底引用了哪些贴图——用来把"导出问题"和"Blender 问题"分开。

用法：
    python3 tools/check_obj_textures.py out_file/marge_obj_from_chunk_0_0_to_1_1.obj
    python3 tools/check_obj_textures.py <obj> --list      # 逐个材质列出

判定依据是分辨率：原版 Minecraft 1.12.2 的方块贴图一律 16x16，
材质包通常是 128/256/512/1024。全是 16x16 说明这次导出用的确实是原版素材；
若出现 512 之类，说明导出没问题，Blender 里看到旧材质就是 Blender 侧的原因。
"""

import argparse
import struct
import sys
from collections import Counter
from pathlib import Path


def png_size(path):
    """读 PNG 头拿分辨率；不是 PNG 或读不了返回 None。"""
    try:
        with open(path, "rb") as handle:
            header = handle.read(24)
    except OSError:
        return None
    if len(header) < 24 or header[:8] != b"\x89PNG\r\n\x1a\n":
        return None
    return struct.unpack(">II", header[16:24])


def parse_obj(path):
    """返回 (mtllib 文件名列表, {材质名: 面数})。"""
    libraries = []
    usage = Counter()
    current = None
    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        for line in handle:
            if line.startswith("mtllib"):
                libraries.append(line.split(None, 1)[1].strip())
            elif line.startswith("usemtl"):
                current = line.split(None, 1)[1].strip()
            elif line.startswith("f ") and current is not None:
                usage[current] += 1
    return libraries, usage


def parse_mtl(path):
    """返回 [(材质名, map_Kd 路径)]。"""
    entries = []
    current = None
    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        for line in handle:
            if line.startswith("newmtl"):
                current = line.split(None, 1)[1].strip()
            elif line.startswith("map_Kd") and current is not None:
                entries.append((current, line.split(None, 1)[1].strip()))
    return entries


def main():
    parser = argparse.ArgumentParser(description="检查 OBJ 引用的贴图")
    parser.add_argument("obj", type=Path)
    parser.add_argument("--list", action="store_true", help="逐个材质打印")
    args = parser.parse_args()

    obj_path = args.obj.resolve()
    if not obj_path.is_file():
        print("找不到 OBJ：%s" % obj_path, file=sys.stderr)
        return 1

    libraries, usage = parse_obj(obj_path)
    print("OBJ：%s" % obj_path)
    print("  usemtl 用到的材质：%d 个" % len(usage))
    if not libraries:
        print("  没有 mtllib —— 这个 OBJ 只有几何，没有材质")
        return 1

    total_missing = 0
    resolutions = Counter()
    rows = []
    for library in libraries:
        mtl_path = obj_path.parent / library
        if not mtl_path.is_file():
            print("  mtllib %s：找不到" % library)
            total_missing += 1
            continue
        entries = parse_mtl(mtl_path)
        print("  mtllib %s：%d 个材质" % (library, len(entries)))
        for material, texture in entries:
            size = png_size(mtl_path.parent / texture)
            rows.append((material, texture, size))
            if size is None:
                total_missing += 1
            else:
                resolutions[size] += 1

    if args.list:
        print("\n材质 -> 贴图：")
        for material, texture, size in rows:
            shown = "%dx%d" % size if size else "缺失/非 PNG"
            print("  %-44s %-52s %s" % (material, texture, shown))

    print("\n贴图分辨率分布：%s" % dict(sorted(resolutions.items())))
    print("缺失/读不出的贴图：%d" % total_missing)

    widths = [width for (width, _height) in resolutions]
    if not widths:
        print("结论：没有可读的贴图，导出可能不完整")
    elif max(widths) <= 16:
        print("结论：贴图全是 16x16 —— **这次导出用的还是原版素材**")
        print('      看运行时打印的 "assets root:" 那一行指向哪里')
    else:
        print("结论：贴图最大 %dx%d —— 导出用的是材质包素材，导出没问题"
              % (max(widths), max(widths)))
        print("      Blender 里若仍显示原版，多半是它复用了已加载的图片数据块：")
        print("      新建空 .blend 再导入，或删掉旧物体后 File > Purge Unused Data")
    return 0


if __name__ == "__main__":
    sys.exit(main())
