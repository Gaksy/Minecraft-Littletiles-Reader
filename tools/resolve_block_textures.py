#!/usr/bin/env python3
"""把 Minecraft 1.12.2 的 block:meta 解析成六个面的贴图路径与 tintindex。

用法:
  python3 resolve_block_textures.py <assets_root> [block_id ...]      # 打印若干方块的解析结果
  python3 resolve_block_textures.py <assets_root> --table <out.tsv>   # 生成完整映射表给 C++ 读

assets_root 需包含 blockstates/ models/ textures/（由客户端 jar 的 assets/minecraft 解出）。

映射表格式（TSV，每行一个材质键）:
  <block 或 block:meta>
    <down> <up> <north> <south> <west> <east>       # 贴图路径，相对 assets_root/textures，省略 .png
    <down_tint> ... <east_tint>                     # 该面的 tintindex，-1 表示不染色

tintindex 对应 MC 的"生物群系染色"：带 tintindex 的面在游戏里会被乘上生物群系颜色
（草方块顶面、树叶等），详见 docs/texture-mapping.md。
"""
import json, os, sys

# 1.12 的染色方块元数据顺序
COLORS = ['white', 'orange', 'magenta', 'light_blue', 'yellow', 'lime', 'pink', 'gray',
          'silver', 'cyan', 'purple', 'blue', 'brown', 'green', 'red', 'black']
# 带元数据的方块族: block 名 -> 由 meta 得到 blockstate 名
FAMILIES = {
    'wool': lambda m: COLORS[m] + '_wool',
    'stained_glass': lambda m: COLORS[m] + '_stained_glass',
    'stained_glass_pane': lambda m: COLORS[m] + '_stained_glass_pane',
    'stained_hardened_clay': lambda m: COLORS[m] + '_stained_hardened_clay',
    'carpet': lambda m: COLORS[m] + '_carpet',
    'concrete': lambda m: COLORS[m] + '_concrete',
    'concrete_powder': lambda m: COLORS[m] + '_concrete_powder',
    'stone': lambda m: ['stone', 'granite', 'smooth_granite', 'diorite', 'smooth_diorite',
                        'andesite', 'smooth_andesite'][m],
    'stonebrick': lambda m: ['stonebrick', 'mossy_stonebrick', 'cracked_stonebrick',
                             'chiseled_stonebrick'][m],
    'planks': lambda m: ['oak_planks', 'spruce_planks', 'birch_planks', 'jungle_planks',
                         'acacia_planks', 'dark_oak_planks'][m],
    'log': lambda m: ['oak_log', 'spruce_log', 'birch_log', 'jungle_log'][m],
    'log2': lambda m: ['acacia_log', 'dark_oak_log'][m],
    'sapling': lambda m: ['oak_sapling', 'spruce_sapling', 'birch_sapling', 'jungle_sapling',
                          'acacia_sapling', 'dark_oak_sapling'][m],
    'leaves': lambda m: ['oak_leaves', 'spruce_leaves', 'birch_leaves', 'jungle_leaves'][m],
    'leaves2': lambda m: ['acacia_leaves', 'dark_oak_leaves'][m],
}
FACES = ['down', 'up', 'north', 'south', 'west', 'east']


class Resolver:
    """按 root 列表解析 blockstate/model/texture，先找到的优先。

    列表里通常第一个是材质包、最后一个是原版：材质包只覆盖它有的文件，
    其余自动回退到原版（与 Minecraft 的资源包叠加规则一致）。
    """

    def __init__(self, roots):
        self.roots = [roots] if isinstance(roots, str) else list(roots)
        self.root = self.roots[-1]
        self.cache = {}

    def _load(self, sub, name):
        key = (sub, name)
        if key in self.cache:
            return self.cache[key]
        data = None
        for root in self.roots:
            path = os.path.join(root, sub, name + '.json')
            if os.path.exists(path):
                with open(path) as handle:
                    data = json.load(handle)
                break
        self.cache[key] = data
        return data

    def blockstate_name(self, block, meta):
        name = block.split(':')[-1]
        # 注意 meta=0 也要走族映射：例如 leaves:0 -> oak_leaves、wool:0 -> white_wool
        table = FAMILIES.get(name)
        if table:
            try:
                name = table(meta)
            except IndexError:
                pass
        return name

    def model(self, name, depth=0):
        """返回 (textures 合并表, faces 表)；faces 的 value 是 (贴图引用, tintindex)。"""
        if depth > 10:
            return {}, {}
        data = self._load('models/block', name)
        if data is None:
            return {}, {}
        textures, faces = {}, {}
        parent = data.get('parent')
        if parent:
            parent_name = parent.split(':')[-1]
            if parent_name.startswith('block/'):
                parent_name = parent_name[len('block/'):]
            textures, faces = self.model(parent_name, depth + 1)
        textures = dict(textures)
        textures.update(data.get('textures', {}))
        # MC 的规则：子模型一旦定义 elements 就完全覆盖父模型的 elements（不是合并）。
        # 同一个面可能出现多次（多层模型，如草方块 = 基础层 + overlay 层），每个面只取第一层。
        if 'elements' in data:
            faces = {}
            for element in data['elements']:
                for face, spec in element.get('faces', {}).items():
                    if 'texture' in spec and face not in faces:
                        tint = spec.get('tintindex')
                        faces[face] = (spec['texture'], -1 if tint is None else tint)
        return textures, faces

    def resolve_texture(self, value, textures, depth=0):
        while isinstance(value, str) and value.startswith('#') and depth < 10:
            value = textures.get(value[1:], value)
            depth += 1
        return value if isinstance(value, str) and not value.startswith('#') else None

    def faces_of_blockstate(self, name):
        """按 blockstate 文件名解析出六个面的 (贴图路径, tintindex)。"""
        bs = self._load('blockstates', name)
        if bs is None:
            return None, 'blockstate 缺失'
        variants = bs.get('variants', {})
        # 优先 normal，否则取第一个（带属性变体的方块只取第一个）
        variant = variants.get('normal') or next(iter(variants.values()), None)
        if variant is None:
            return None, 'variants 为空'
        if isinstance(variant, list):
            variant = variant[0]
        model_name = variant['model'].split(':')[-1]
        if model_name.startswith('block/'):
            model_name = model_name[len('block/'):]
        textures, faces = self.model(model_name)
        result = {}
        for face in FACES:
            if face not in faces:
                result[face] = (None, -1)
                continue
            ref, tint = faces[face]
            result[face] = (self.resolve_texture(ref, textures), tint)
        return result, None

    def faces_of(self, block, meta):
        return self.faces_of_blockstate(self.blockstate_name(block, meta))


def list_blockstates(root):
    directory = os.path.join(root, 'blockstates')
    return sorted(f[:-5] for f in os.listdir(directory) if f.endswith('.json'))


def dump_table(root, out_path, overlay_roots=()):
    """生成完整映射表：minecraft:<blockstate> 与 minecraft:<family>:<meta> 两类键。

    overlay_roots 优先级高于 root（列表前面的优先），但方块清单取自 root——
    调用方应把"提供 blockstates 清单的那一份"（通常是原版 1.12.2）当作 root。
    """
    resolver = Resolver(list(overlay_roots) + [root])
    keys = {}
    for name in list_blockstates(root):
        faces, _ = resolver.faces_of_blockstate(name)
        if faces and faces['up'][0]:
            keys['minecraft:' + name] = faces
    # 带元数据的方块族：正向枚举变体名，反推回 block:meta
    for family, mapper in FAMILIES.items():
        for meta in range(16):
            try:
                variant = mapper(meta)
            except IndexError:
                break
            # 变体清单同样以 root 为准，避免材质包引入的额外 blockstate 混进表里
            if not os.path.exists(os.path.join(root, 'blockstates', variant + '.json')):
                continue
            faces, _ = resolver.faces_of_blockstate(variant)
            if faces and faces['up'][0]:
                keys['minecraft:%s:%d' % (family, meta)] = faces

    with open(out_path, 'w') as handle:
        handle.write('# block(+meta)\t' + '\t'.join(FACES) + '\t' +
                     '\t'.join(f + '_tint' for f in FACES) + '\n')
        for key in sorted(keys):
            faces = keys[key]
            paths = [faces[f][0] or '-' for f in FACES]
            tints = [str(faces[f][1]) for f in FACES]
            handle.write('%s\t%s\n' % (key, '\t'.join(paths + tints)))
    return len(keys)


def main():
    root = sys.argv[1]
    if len(sys.argv) >= 4 and sys.argv[2] == '--table':
        count = dump_table(root, sys.argv[3])
        print('已生成映射表: %s（%d 个键）' % (sys.argv[3], count))
        return

    resolver = Resolver(root)
    blocks = sys.argv[2:] or [
        'minecraft:quartz_ore', 'minecraft:purpur_block', 'minecraft:bedrock',
        'minecraft:coal_ore', 'minecraft:stone:3', 'minecraft:stained_glass:15',
        'minecraft:stonebrick:2', 'minecraft:stone', 'minecraft:stone:2',
        'minecraft:wool:8', 'minecraft:stained_hardened_clay:6',
        'minecraft:grass', 'minecraft:log:0', 'minecraft:leaves:0',
        'littletiles:ltcoloredblock',
    ]
    ok = fail = 0
    for entry in blocks:
        parts = entry.split(':')
        block = parts[0] + ':' + parts[1]
        meta = int(parts[2]) if len(parts) > 2 else 0
        faces, err = resolver.faces_of(block, meta)
        if faces and faces['up'][0]:
            ok += 1
            path = os.path.join(root, 'textures', faces['up'][0] + '.png')
            tints = sorted({tint for _, tint in faces.values() if tint >= 0})
            print('%-32s -> %-30s tintindex: %-8s %s' % (
                entry, faces['up'][0] + '.png', tints or '无',
                '✓' if os.path.exists(path) else '✗ 文件不存在'))
        else:
            fail += 1
            print('%-32s -> 解析失败: %s' % (entry, err or 'faces 为空'))
    print('\n成功 %d / 失败 %d' % (ok, fail))


if __name__ == '__main__':
    main()
