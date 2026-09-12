#!/usr/bin/env python3
"""把模组（LittleTiles 本体、Kiro's Basic Blocks 等）的贴图并进映射表。

为什么需要单独一步：原版映射表是按 `minecraft:` 命名空间建的，而 LittleTiles
的结构文件里会出现 `littletiles:ltcoloredblock:10`、`kirosblocks:colored_*`
这类方块。它们**不在原版素材里**，得从各自模组 jar 的 assets 中解析出来。

模组素材的目录结构（从 jar 的 `assets/<ns>/` 解出）与原版一致：
    <mod_root>/blockstates/  models/block/  textures/blocks/
所以可以直接复用 resolve_block_textures.Resolver。

产物（默认 assets/pack_snbt）：
    block_textures.tsv      原版映射表 + 模组方块（键为 `<ns>:<名字>`）
    block_ids.tsv           从原版复制（SNBT 路径不需要，save 路径需要）
    textures/blocks/...     原版贴图
    textures/<ns>/blocks/... 模组贴图（带命名空间目录，避免与重名贴图撞车）

用法：
    python3 tools/add_mod_textures.py \
        --mod-root assets/littletiles_1.5.66 --namespace littletiles \
        --mod-root assets/kirosblocks_1.2.2 --namespace kirosblocks \
        --base assets/1.12.2 --out assets/pack_snbt
"""

import argparse
import shutil
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(Path(__file__).resolve().parent))
import resolve_block_textures as rbt  # noqa: E402

FACES = rbt.FACES

# LittleTiles 的彩色方块：meta 与贴图不是通过 blockstate 属性表达的，
# 而是靠方块元数据映射到 `colored_block_*` 模型。这里按贴图编号还原：
#   meta 0..11 -> littletiles/blocks/ltcolored<meta>
#   meta 12    -> light_clean（与 clean 共用贴图）、13 -> 岩浆、14 -> 白色岩浆
# `ltcoloredblock2` 则是另外五个变体。
# 依据：blockstates/ltcoloredblock.json 的变体顺序与 textures/blocks/ltcoloredN.png
# 的编号顺序一致（clean=0 … clay=10、strips=11），实测房屋用的 meta 0/10 也对得上。
LT_COLORED_META = {
    ('littletiles', 'ltcoloredblock'): {
        12: 'littletiles/blocks/ltcolored0',
        13: 'minecraft/blocks/lava_still',
        14: 'littletiles/blocks/white_lava_still',
    },
    ('littletiles', 'ltcoloredblock2'): {
        0: 'littletiles/blocks/ltgravel',
        1: 'littletiles/blocks/ltsand',
        2: 'littletiles/blocks/ltstone',
        3: 'littletiles/blocks/ltwood',
        4: 'littletiles/blocks/white_opaque_water',
    },
}


def normalize_ref(namespace, ref):
    """把模型里的贴图引用归一成 "<命名空间目录>/<路径>"（相对 textures/）。"""
    if ':' in ref:
        other, path = ref.split(':', 1)
    else:
        other, path = namespace, ref
    return '%s/%s' % (other, path)


def resolve_namespace(mod_root, namespace, rows, missing, fallback_root):
    """解析一个模组命名空间下所有 blockstate，产出映射表行。

    模组模型往往继承原版父模型（`block/cube_all` 之类），所以 Resolver 要把
    原版目录作为兜底，否则绝大多数方块都会解析不出面。
    """
    resolver = rbt.Resolver([str(mod_root), str(fallback_root)])
    for name in rbt.list_blockstates(str(mod_root)):
        faces, error = resolver.faces_of_blockstate(name)
        if not faces or not faces['up'][0]:
            missing.append((name, error or 'faces 为空'))
            continue
        paths, tints = [], []
        for face in FACES:
            ref, tint = faces[face]
            paths.append(normalize_ref(namespace, ref) if ref else '-')
            tints.append(str(tint))
        rows['%s:%s' % (namespace, name)] = paths + tints
    # 彩色方块的 meta 变体（只对 LittleTiles 有意义）
    for (ns, block), table in LT_COLORED_META.items():
        if ns != namespace:
            continue
        for meta, texture in table.items():
            rows['%s:%s:%d' % (ns, block, meta)] = [texture] * 6 + ['-1'] * 6
        # meta 0..11 就是 ltcolored0..11
        base = rbt.Resolver([str(mod_root)])
        variants = base._load('blockstates', block)
        if variants:
            order = list(variants.get('variants', {}).keys())
            for meta in range(12):
                if meta < len(order):
                    rows['%s:%s:%d' % (ns, block, meta)] = [
                        'littletiles/blocks/ltcolored%d' % meta] * 6 + ['-1'] * 6


def write_tsv(path, rows):
    with open(path, 'w', encoding='utf-8') as handle:
        handle.write('# block(+meta)\t' + '\t'.join(FACES) + '\t' +
                     '\t'.join(f + '_tint' for f in FACES) + '\n')
        for key in sorted(rows):
            handle.write('%s\t%s\n' % (key, '\t'.join(rows[key])))


def read_tsv_rows(path):
    rows = {}
    for line in Path(path).read_text(encoding='utf-8').splitlines():
        if not line or line.startswith('#'):
            continue
        parts = line.split('\t')
        if len(parts) >= 13:
            rows[parts[0]] = parts[1:]
    return rows


def copy_textures(rows, out_dir, root_by_namespace, stats):
    """按映射表逐张挑贴图，统一放到 textures/<命名空间>/... 下。"""
    for key, row in rows.items():
        for rel in row[:6]:
            if rel == '-':
                continue
            namespace, _, rest = rel.partition('/')
            target = out_dir / 'textures' / namespace / (rest + '.png')
            if target.exists():
                continue
            root = root_by_namespace.get(namespace)
            found = False
            for root in ([root] if root else []) + list(root_by_namespace.values()):
                candidate = root / 'textures' / (rest + '.png')
                if candidate.is_file():
                    target.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copyfile(candidate, target)
                    stats[namespace] = stats.get(namespace, 0) + 1
                    found = True
                    break
            if not found:
                stats.setdefault('缺失', []).append('%s -> %s' % (key, rel))


def main():
    parser = argparse.ArgumentParser(description='把模组贴图并进映射表')
    parser.add_argument('--mod-root', action='append', default=[],
                        help='解出来的模组素材目录（可多次传入）')
    parser.add_argument('--namespace', action='append', default=[],
                        help='与 --mod-root 一一对应的命名空间')
    parser.add_argument('--base', type=Path, default=REPO_ROOT / 'assets' / '1.12.2',
                        help='原版素材目录（提供基础映射表与兜底贴图）')
    parser.add_argument('--out', type=Path, default=REPO_ROOT / 'assets' / 'pack_snbt')
    args = parser.parse_args()

    if len(args.mod_root) != len(args.namespace):
        raise SystemExit('--mod-root 与 --namespace 数量必须一致')

    out_dir = args.out
    out_dir.mkdir(parents=True, exist_ok=True)

    base_rows = read_tsv_rows(args.base / 'block_textures.tsv')
    # 原版路径统一加 minecraft/ 前缀：这样所有贴图都落在 textures/<命名空间>/ 下，
    # 模组与同名原版贴图不会互相覆盖。
    rows = {}
    for key, row in base_rows.items():
        rows[key] = ['minecraft/' + p if p != '-' else '-' for p in row[:6]] + list(row[6:])
    print('原版映射表：%d 个键' % len(rows))
    unresolved = []
    for mod_root, namespace in zip(args.mod_root, args.namespace):
        before = len(rows)
        resolve_namespace(Path(mod_root), namespace, rows, unresolved, args.base)
        print('  %-14s 解析出 %d 个键（来自 %s）' % (namespace, len(rows) - before, mod_root))
    if unresolved:
        print('  未解析的 blockstate %d 个，例如：%s'
              % (len(unresolved), ', '.join(n for n, _ in unresolved[:5])))

    write_tsv(out_dir / 'block_textures.tsv', rows)
    shutil.copyfile(args.base / 'block_ids.tsv', out_dir / 'block_ids.tsv')

    stats = {}
    root_by_namespace = {'minecraft': args.base}
    for mod_root, namespace in zip(args.mod_root, args.namespace):
        root_by_namespace[namespace] = Path(mod_root)
    copy_textures(rows, out_dir, root_by_namespace, stats)
    print('复制贴图：', {k: v for k, v in stats.items() if k != '缺失'})
    if stats.get('缺失'):
        print('  缺失 %d 张（不会致命，导出时该面退化为没有贴图）：%s'
              % (len(stats['缺失']), stats['缺失'][:3]))
    print('\n完成：%s' % out_dir)
    print('用法： LITTLETILES_ASSETS=%s ./LittleTilesReader' % out_dir)
    return 0


if __name__ == '__main__':
    sys.exit(main())
