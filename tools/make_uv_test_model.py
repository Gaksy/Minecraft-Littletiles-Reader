#!/usr/bin/env python3
"""生成 UV 验证用的小模型（OBJ + MTL + 贴图）。

用途：在 Blender 等软件里检查"六面 UV 约定"和"按位置裁剪取样"是否正确。
样本偏重**面与面差异明显**的方块，便于肉眼判断朝向：

  1. grass            侧面必须"草在上、土在下"（v 方向是否反了一看便知）
  2. crafting_table   六个面各用不同贴图，顶面图案不对称
  3. furnace          正面（开口）应出现在 -z 面，且不能镜像
  4. pumpkin          正面（脸）与侧面不同
  5. log:0 (oak_log)  顶/底是年轮、侧面是树皮
  6. 裁剪测试         同一贴图取"上半块"与"下半块"，验证按位置裁剪取样

用法: python3 make_uv_test_model.py <assets_root> <out_dir>
"""
import os
import shutil
import sys

FACES = ['down', 'up', 'north', 'south', 'west', 'east']


def load_table(assets_root):
    """读取 block_textures.tsv -> {key: {face: texture_path}}"""
    table = {}
    with open(os.path.join(assets_root, 'block_textures.tsv')) as handle:
        for line in handle:
            line = line.rstrip('\n')
            if not line or line.startswith('#'):
                continue
            parts = line.split('\t')
            table[parts[0]] = dict(zip(FACES, parts[1:7]))
    return table


def face_uv(face, x, y, z):
    """按 MC 模型规范给出该面这一点的 (u, v)，v = 0 在贴图顶部。

    x/y/z 为方块内归一化坐标（x 西→东，y 下→上，z 北→南）。
    """
    if face == 'up':
        return (x, z)
    if face == 'down':
        return (1.0 - x, z)
    if face == 'east':
        return (1.0 - z, 1.0 - y)
    if face == 'west':
        return (z, 1.0 - y)
    if face == 'north':
        return (1.0 - x, 1.0 - y)
    if face == 'south':
        return (x, 1.0 - y)
    raise ValueError(face)


def box_faces(box):
    """返回 [(face, [4 个角点])]，角点顺序与引擎绕向一致（外法线朝外）。"""
    x1, y1, z1, x2, y2, z2 = box
    return [
        ('east', [(x2, y1, z2), (x2, y1, z1), (x2, y2, z1), (x2, y2, z2)]),
        ('west', [(x1, y1, z1), (x1, y1, z2), (x1, y2, z2), (x1, y2, z1)]),
        ('south', [(x1, y1, z2), (x2, y1, z2), (x2, y2, z2), (x1, y2, z2)]),
        ('north', [(x2, y1, z1), (x1, y1, z1), (x1, y2, z1), (x2, y2, z1)]),
        ('up', [(x1, y2, z2), (x2, y2, z2), (x2, y2, z1), (x1, y2, z1)]),
        ('down', [(x1, y1, z1), (x2, y1, z1), (x2, y1, z2), (x1, y1, z2)]),
    ]


def material_name(texture):
    return texture.replace('/', '_')


def main():
    assets_root, out_dir = sys.argv[1], sys.argv[2]
    table = load_table(assets_root)
    os.makedirs(out_dir, exist_ok=True)

    # (材质键, 方块内包围盒 [x1,y1,z1,x2,y2,z2], 世界 x 偏移)
    samples = [
        ('minecraft:grass', [0, 0, 0, 1, 1, 1], 0),
        ('minecraft:crafting_table', [0, 0, 0, 1, 1, 1], 2),
        ('minecraft:furnace', [0, 0, 0, 1, 1, 1], 4),
        ('minecraft:pumpkin', [0, 0, 0, 1, 1, 1], 6),
        ('minecraft:log:0', [0, 0, 0, 1, 1, 1], 8),
        # 裁剪取样测试：同一贴图取上半块 / 下半块
        ('minecraft:grass', [0, 0.5, 0, 1, 1.0, 1], 10),
        ('minecraft:grass', [0, 0.0, 0, 1, 0.5, 1], 12),
    ]

    vertices, texcoords, faces = [], [], []
    used_textures = {}

    for key, box, offset in samples:
        if key not in table:
            print('跳过（表中没有）: %s' % key)
            continue
        face_textures = table[key]
        for face, corners in box_faces(box):
            texture = face_textures[face]
            if not texture or texture == '-':
                continue
            used_textures[texture] = True
            indices = []
            for (x, y, z) in corners:
                vertices.append((x + offset, y, z))
                u, v = face_uv(face, x, y, z)
                texcoords.append((u, 1.0 - v))  # OBJ 的 vt 以左下为原点
                indices.append(len(vertices))
            faces.append((material_name(texture), indices))

    obj_path = os.path.join(out_dir, 'uv_test.obj')
    mtl_path = os.path.join(out_dir, 'uv_test.mtl')

    with open(obj_path, 'w') as handle:
        handle.write('# UV 验证模型：六面朝向 + 按位置裁剪取样\n')
        handle.write('mtllib uv_test.mtl\n')
        for (x, y, z) in vertices:
            handle.write('v %g %g %g\n' % (x, y, z))
        for (u, v) in texcoords:
            handle.write('vt %.6f %.6f\n' % (u, v))
        # 本脚本不合并顶点，因此 vt 索引与 v 索引一一对应
        current = None
        for (name, indices) in faces:
            if name != current:
                handle.write('usemtl %s\n' % name)
                current = name
            handle.write('f %s\n' % ' '.join('%d/%d' % (i, i) for i in indices))

    with open(mtl_path, 'w') as handle:
        handle.write('# 由 tools/make_uv_test_model.py 生成\n')
        for texture in sorted(used_textures):
            source = os.path.join(assets_root, 'textures', texture + '.png')
            target = os.path.join(out_dir, os.path.basename(texture) + '.png')
            if os.path.exists(source):
                shutil.copyfile(source, target)
            handle.write('\nnewmtl %s\n' % material_name(texture))
            handle.write('Ka 1.000 1.000 1.000\n')
            handle.write('Kd 1.000 1.000 1.000\n')
            handle.write('d 1.0\n')
            handle.write('map_Kd %s\n' % os.path.basename(target))

    print('已生成:')
    print('  %s  (%d 顶点, %d 面, %d 材质)' % (obj_path, len(vertices), len(faces), len(used_textures)))
    print('  %s' % mtl_path)
    print('  贴图 %d 张已复制到输出目录' % len(used_textures))


main()
