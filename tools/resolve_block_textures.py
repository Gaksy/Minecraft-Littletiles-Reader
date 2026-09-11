#!/usr/bin/env python3
"""把 Minecraft 1.12.2 的 block:meta 解析成六个面的贴图路径。

用法:
  python3 resolve_block_textures.py <assets_root> [block_id ...]      # 打印若干方块的解析结果
  python3 resolve_block_textures.py <assets_root> --table <out.tsv>   # 生成完整映射表给 C++ 读

assets_root 需包含 blockstates/ models/ textures/（由客户端 jar 的 assets/minecraft 解出）。

映射表格式（TSV，每行一个材质键）:
  <block 或 block:meta>\t<down>\t<up>\t<north>\t<south>\t<west>\t<east>
贴图路径相对 assets_root/textures，省略 .png 后缀。
"""
import json, os, sys

# 1.12 的染色方块元数据顺序
COLORS = ['white','orange','magenta','light_blue','yellow','lime','pink','gray',
          'silver','cyan','purple','blue','brown','green','red','black']
# 带元数据的方块族: block 名 -> meta 后缀
FAMILIES = {
    'wool': lambda m: COLORS[m] + '_wool',
    'stained_glass': lambda m: COLORS[m] + '_stained_glass',
    'stained_glass_pane': lambda m: COLORS[m] + '_stained_glass_pane',
    'stained_hardened_clay': lambda m: COLORS[m] + '_stained_hardened_clay',
    'carpet': lambda m: COLORS[m] + '_carpet',
    'concrete': lambda m: COLORS[m] + '_concrete',
    'concrete_powder': lambda m: COLORS[m] + '_concrete_powder',
    'stone': lambda m: ['stone','granite','smooth_granite','diorite','smooth_diorite',
                        'andesite','smooth_andesite'][m],
    'stonebrick': lambda m: ['stonebrick','mossy_stonebrick','cracked_stonebrick',
                             'chiseled_stonebrick'][m],
    'planks': lambda m: ['oak_planks','spruce_planks','birch_planks','jungle_planks',
                         'acacia_planks','dark_oak_planks'][m],
    'log': lambda m: ['oak_log','spruce_log','birch_log','jungle_log'][m],
    'log2': lambda m: ['acacia_log','dark_oak_log'][m],
    'sapling': lambda m: ['oak_sapling','spruce_sapling','birch_sapling','jungle_sapling',
                          'acacia_sapling','dark_oak_sapling'][m],
    'leaves': lambda m: ['oak_leaves','spruce_leaves','birch_leaves','jungle_leaves'][m],
    'leaves2': lambda m: ['acacia_leaves','dark_oak_leaves'][m],
}
FACES = ['down','up','north','south','west','east']

class Resolver:
    def __init__(self, root):
        self.root = root
        self.cache = {}

    def _load(self, sub, name):
        key = (sub, name)
        if key in self.cache: return self.cache[key]
        path = os.path.join(self.root, sub, name + '.json')
        data = json.load(open(path)) if os.path.exists(path) else None
        self.cache[key] = data
        return data

    def blockstate(self, block, meta):
        name = block.split(':')[-1]
        if meta:
            table = FAMILIES.get(name)
            if table:
                try: name = table(meta)
                except IndexError: pass
        return self._load('blockstates', name)

    def faces_of_blockstate(self, name):
        """按 blockstate 文件名解析出六个面的贴图路径。"""
        bs = self._load('blockstates', name)
        if bs is None: return None, 'blockstate 缺失'
        variants = bs.get('variants', {})
        # 优先 normal，否则取第一个（带属性的方块只取第一个变体）
        variant = variants.get('normal') or next(iter(variants.values()), None)
        if variant is None: return None, 'variants 为空'
        if isinstance(variant, list): variant = variant[0]
        model_name = variant['model'].split(':')[-1]
        if model_name.startswith('block/'): model_name = model_name[len('block/'):]
        textures, faces = self.model(model_name)
        result = {}
        for face in FACES:
            ref = faces.get(face)
            result[face] = self.resolve_texture(ref, textures) if ref else None
        return result, None

    def model(self, name, depth=0):
        """返回 (textures 合并表, faces 的 texture 引用表)"""
        if depth > 10: return {}, {}
        data = self._load('models/block', name)
        if data is None: return {}, {}
        textures, faces = {}, {}
        parent = data.get('parent')
        if parent:
            # parent 可能是 "block/xxx" / "minecraft:block/xxx"
            pname = parent.split(':')[-1]
            if pname.startswith('block/'): pname = pname[len('block/'):]
            textures, faces = self.model(pname, depth + 1)
        textures = dict(textures); textures.update(data.get('textures', {}))
        # MC 的规则：子模型一旦定义 elements，就**完全覆盖**父模型的 elements（不是合并）。
        # 同一个模型里同一个面可能出现多次（多层模型，例如草方块 = 底面层 + overlay 层），
        # 这里每个面只取第一层，即基础层。
        if 'elements' in data:
            faces = {}
            for element in data['elements']:
                for face, spec in element.get('faces', {}).items():
                    if 'texture' in spec and face not in faces:
                        faces[face] = spec['texture']
        return textures, faces

    def resolve_texture(self, value, textures, depth=0):
        while isinstance(value, str) and value.startswith('#') and depth < 10:
            value = textures.get(value[1:], value)
            depth += 1
        return value if isinstance(value, str) and not value.startswith('#') else None

    def faces_of(self, block, meta):
        bs = self.blockstate(block, meta)
        if bs is None: return None, 'blockstate 缺失'
        variants = bs.get('variants', {})
        # 优先 normal，否则取第一个（含属性变体时只取第一个）
        variant = variants.get('normal') or next(iter(variants.values()), None)
        if variant is None: return None, 'variants 为空'
        if isinstance(variant, list): variant = variant[0]
        model_name = variant['model'].split(':')[-1]
        if model_name.startswith('block/'): model_name = model_name[len('block/'):]
        textures, faces = self.model(model_name)
        result = {}
        for face in FACES:
            ref = faces.get(face)
            result[face] = self.resolve_texture(ref, textures) if ref else None
        return result, None

def list_blockstates(root):
    directory = os.path.join(root, 'blockstates')
    return sorted(f[:-5] for f in os.listdir(directory) if f.endswith('.json'))

def dump_table(root, out_path):
    """生成完整映射表: minecraft:<blockstate> 与 minecraft:<family>:<meta> 两类键。"""
    r = Resolver(root)
    keys = {}
    for name in list_blockstates(root):
        faces, _ = r.faces_of_blockstate(name)
        if faces and faces['up']:
            keys['minecraft:' + name] = faces
    # 带元数据的方块族：正向枚举变体名，反推回 block:meta
    for family, mapper in FAMILIES.items():
        for meta in range(16):
            try: variant = mapper(meta)
            except IndexError: break
            if not os.path.exists(os.path.join(root, 'blockstates', variant + '.json')): continue
            faces, _ = r.faces_of_blockstate(variant)
            if faces and faces['up']:
                keys['minecraft:%s:%d' % (family, meta)] = faces

    with open(out_path, 'w') as f:
        f.write('# block(+meta)\tdown\tup\tnorth\tsouth\twest\teast\n')
        for key in sorted(keys):
            faces = keys[key]
            f.write('%s\t%s\n' % (key, '\t'.join(faces[face] or '-' for face in FACES)))
    return len(keys)

def main():
    root = sys.argv[1]
    if len(sys.argv) >= 4 and sys.argv[2] == '--table':
        count = dump_table(root, sys.argv[3])
        print('已生成映射表: %s（%d 个键）' % (sys.argv[3], count))
        return
    r = Resolver(root)
    blocks = sys.argv[2:] or ['minecraft:quartz_ore']
    if not sys.argv[2:]:  # 默认跑测试数据里出现的方块
        blocks = ['minecraft:quartz_ore','minecraft:purpur_block','minecraft:bedrock',
                  'minecraft:coal_ore','minecraft:stone:3','minecraft:stained_glass:15',
                  'minecraft:stonebrick:2','minecraft:stone','minecraft:stone:2',
                  'minecraft:wool:8','minecraft:stained_hardened_clay:6',
                  'littletiles:ltcoloredblock']
    ok = fail = 0
    for b in blocks:
        parts = b.split(':')
        name = parts[0] + ':' + parts[1]
        meta = int(parts[2]) if len(parts) > 2 else 0
        faces, err = r.faces_of(name, meta)
        if faces and faces['up']:
            ok += 1
            path = os.path.join(root, 'textures', faces['up'] + '.png')
            exists = '✓' if os.path.exists(path) else '✗ 文件不存在'
            uniq = sorted(set(v for v in faces.values() if v))
            print('%-38s -> %-28s 六面贴图: %s  %s' % (b, faces['up'] + '.png', ','.join(uniq), exists))
        else:
            fail += 1
            print('%-38s -> 解析失败: %s' % (b, err or 'faces 为空'))
    print('\n成功 %d / 失败 %d' % (ok, fail))

main()
