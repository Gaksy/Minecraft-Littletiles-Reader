#!/usr/bin/env python3
"""生成 1.12.2 的"方块数字 ID -> 方块名"映射表。

1.12 的区块数据（Level.Sections[].Blocks）存的是数字 ID（1 字节/方块），
而 LittleTiles 的贴图表与模型都是用方块名索引的，所以需要这张表。

数据源：PrismarineJS/minecraft-data 的 data/pc/1.12/blocks.json
（1.12 与 1.12.2 的方块注册表相同）。下载后用：

  python3 generate_block_id_table.py <blocks.json> <out.tsv>

输出格式（TSV）: <id>\t<name>        例如  1\tstone
"""
import json
import sys


def main():
    source, out_path = sys.argv[1], sys.argv[2]
    entries = json.load(open(source))
    by_id = {}
    for entry in entries:
        block_id = entry.get('id')
        name = entry.get('name')
        if isinstance(block_id, int) and name:
            by_id.setdefault(block_id, name)

    with open(out_path, 'w') as handle:
        handle.write('# id\tname\n')
        for block_id in sorted(by_id):
            handle.write('%d\t%s\n' % (block_id, by_id[block_id]))

    print('已生成 %s（%d 个 ID）' % (out_path, len(by_id)))
    for probe in (0, 1, 2, 3, 7, 41, 54, 251, 252):
        print('  id %-4d -> %s' % (probe, by_id.get(probe, '（缺）')))


main()
