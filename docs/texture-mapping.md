# 贴图 / UV 映射方案（1.12.2）

本文记录模型贴图与 UV 的实现依据。结论均已对照真实素材与权威实现核实。

## 1. 基本语义：按位置裁剪取样（A 方案）

一块 tile 只使用它所在**方块贴图的一部分**：tile 在方块内的位置与尺寸，直接决定它采样贴图的哪一小块。
因此满屏 1/64 的小 tile 拼起来正好还原整张贴图（无缝），而不是每个小方块各显示一张缩小的完整贴图。

推论：UV 不需要从 NBT 读（也读不到，1.12 的 tile 数据只有 `block` / `color` / `box(es)` / `grid`），
而是**从几何推导**：面在方块内的位置 → 贴图上的位置。

## 2. 六个面的 UV 约定

模型 JSON 本身不描述朝向，这是硬编码规范。下表按 `prismarine-viewer` 的实现核实
（`viewer/lib/models.js` 的 `elemFaces`，它实现的是 MC 模型规范），
坐标 `x,y,z ∈ [0,1]` 为**方块内**归一化坐标（x: 西→东，y: 下→上，z: 北→南），
`u,v ∈ [0,1]` 为贴图坐标（**v = 0 在贴图顶部**）：

| 面 | 法线 | u | v |
|---|---|---|---|
| up | +y | x | z |
| down | −y | 1−x | z |
| east | +x | 1−z | 1−y |
| west | −x | z | 1−y |
| north | −z | 1−x | 1−y |
| south | +z | x | 1−y |

注意：OBJ 的 `vt` 以**左下角**为原点，所以写出时要翻转：`vt_v = 1 − v`。
（参考实现的 Java 导出器也是这么做的：`addTexCoord(u, 1.0F - v)`。）

裁剪产生的新顶点同样适用：切割保持面在同一平面上，UV 在面内是仿射的，
所以半空间裁剪出来的多边形只要按顶点位置套同一个公式即可，不需要额外处理。

## 3. 从资源包解析贴图

流程（1.12.2 客户端 jar 内含全部所需素材）：

```
blockstates/<name>.json  → variants → { "model": "<model>" }
models/block/<model>.json → parent 链 + textures 映射（子模型覆盖父模型）
                          → elements[].faces.<face>.texture (如 "#all")
                          → 递归解析 "#引用" 得到 "blocks/xxx"
textures/blocks/xxx.png
```

### 3.1 方块元数据（meta）→ blockstate 文件

1.12 里带 meta 的方块，其 blockstate 文件名多为**颜色/变体前缀**，需要一张映射表
   （`tools/resolve_block_textures.py` 已实现）：

| 方块 | meta → blockstate 名 |
|---|---|
| `wool` | 颜色序（0 白 … 8 silver … 15 黑）→ `silver_wool` |
| `stained_glass` | → `black_stained_glass` |
| `stained_hardened_clay` | → `pink_stained_hardened_clay` |
| `stone` | 0 stone / 1 granite / 2 smooth_granite / 3 diorite / 4 smooth_diorite / 5 andesite / 6 smooth_andesite |
| `stonebrick` | 0 stonebrick / 1 mossy / 2 cracked / 3 chiseled |
| `planks` / `log` / `leaves` / `sapling` … | 各自的变体名 |

1.12 颜色元数据顺序：`white, orange, magenta, light_blue, yellow, lime, pink, gray, silver,
cyan, purple, blue, brown, green, red, black`。

### 3.2 实测（对项目测试数据里的 12 种方块）

用原生 1.12.2 素材解析，11 种成功且贴图文件存在：

```
minecraft:quartz_ore              -> blocks/quartz_ore.png
minecraft:purpur_block            -> blocks/purpur_block.png
minecraft:bedrock                 -> blocks/bedrock.png
minecraft:coal_ore                -> blocks/coal_ore.png
minecraft:stone:3                 -> blocks/stone_diorite.png
minecraft:stained_glass:15        -> blocks/glass_black.png
minecraft:stonebrick:2            -> blocks/stonebrick_cracked.png
minecraft:stone                   -> blocks/stone.png
minecraft:stone:2                 -> blocks/stone_granite_smooth.png
minecraft:wool:8                  -> blocks/wool_colored_silver.png
minecraft:stained_hardened_clay:6 -> blocks/hardened_clay_stained_pink.png
littletiles:ltcoloredblock        -> 失败（LT mod 自带素材，需另提供）
```

### 3.3 解析模型时的两个关键规则

1. **子模型一旦定义 `elements`，就完全覆盖父模型的 elements**（不是合并）；
   只有 `textures` 是逐级合并的。
2. **同一个面可能出现多次**（多层模型）。例如草方块 `block/grass` 有两个 element：
   基础立方体（`down=#bottom`、`up=#top`、四周=`#side`）与叠加层（四周=`#overlay`，带 tintindex）。
   每个面只取**第一层**（基础层），否则会拿到透明的 overlay，渲染结果明显不对。

> 多层模型（草、以及 LT 里带 overlay 的情况）目前只导出基础层；
> 真正还原需要按层输出多个重叠面并处理 alpha 混合，列为后续项。

## 4. 颜色（tile 自带染色）

约 44% 的 tile 带 `color`（ARGB int，例如 `0xFFFFBE00`，alpha 恒为 0xFF）。
MC 的原生渲染是"贴图像素 × 该颜色"。两种落地方式：

| 方式 | 做法 | 取舍 |
|---|---|---|
| 烘焙进贴图 | 每个 (贴图, 颜色) 组合生成一张 PNG（Java 参考实现的做法） | OBJ/MTL 下 Blender 能正确显示；但纹理数量随颜色种类爆炸 |
| 材质因子 | 材质只记贴图，颜色作为材质参数（glTF 的 `baseColorFactor`） | GLB/Web 下最自然；OBJ/MTL 下多数导入器**不会**把 `Kd` 与 `map_Kd` 相乘 |

→ 这也是建议最终走 **GLB** 的原因之一。

### 4.1 实测：染色组合数量很少，"按 (贴图, 颜色) 烘焙"完全可行

对 `test_region*` 全量统计：

```
唯一 (block, color) 组合 : 16
唯一 block               : 13
唯一颜色                 : 6   （0xFF000000 / 0xFF242424 / 0xFF3C3C3C /
                                0xFFFF2424 / 0xFFFF9100 / 0xFFFFBE00）
```

也就是说整个测试数据集只需要 **16 个材质**。既然目标是 OBJ+MTL 的专业流程
（Blender 不会把 `Kd` 与 `map_Kd` 相乘），**按 (贴图, 颜色) 烘焙染色 PNG** 是最省事且
开箱即用的做法——参考实现（Java 模组）也是这么做的，而这里只有 16 个组合，不存在纹理爆炸问题。

工具链约束：本机 `python3` **没有 Pillow**，只有标准库 `zlib`。
因此染色烘焙要么用纯 Python 自行解码/编码 PNG（PNG 有 zlib + 过滤器，可解），
要么在 C++ 侧用已有的 zlib 实现（读写都自己做），要么装一个 Pillow / libpng 依赖。

## 5. 素材来源与放置

1.12.2 客户端 jar（Mojang 官方）内含 `assets/minecraft/{blockstates,models,textures}`，
   无需额外资源包：

```sh
# 1) 取版本清单 → 1.12.2 的版本 json → downloads.client.url
# 2) 下载 client.jar（约 9.7 MB，sha1 0f275bc1…）
# 3) 解出 assets/minecraft/{blockstates,models,textures/blocks} → assets/1.12.2/
```

本项目的 `assets/` 目录**不进入版本库**（见 `.gitignore`），需要时按上面步骤重建。
LT 自身方块（`littletiles:*`）的贴图在 LT mod jar 的 assets 里，需要另行提供。

## 5.1 UV 验证模型（人工核对用）

```sh
python3 tools/resolve_block_textures.py assets/1.12.2 --table assets/1.12.2/block_textures.tsv
python3 tools/make_uv_test_model.py assets/1.12.2 out_file/uv_test
# -> out_file/uv_test/uv_test.obj + uv_test.mtl + 所需贴图（已复制到同目录）
```

样本刻意选"面与面差异明显"的方块，导入 Blender 后按 `.` 定位，逐项确认：

| 样本 | 应该看到 |
|---|---|
| `grass` | 侧面**草在上、土在下**（v 方向反了会立刻看出来）；顶面是草、底面是土 |
| `crafting_table` | 六面各用不同贴图（顶面图案不对称，可判断是否旋转/镜像） |
| `furnace` | 开口应出现在 **-z 面**（north），且不能镜像 |
| `pumpkin` | 脸在 -z 面，侧面是南瓜皮 |
| `log:0` | 顶/底是年轮，侧面是树皮 |
| 两个半高样本（x=10、x=12） | 上半块侧面显示**草**的部分，下半块显示**土**的部分 → 验证"按位置裁剪取样" |

数值自检（不需要 Blender）：把生成的 UV 拿去采样 `grass_side.png`，
上半块样本的 v 落在贴图上半（偏绿），下半块落在下半（偏棕），与预期一致。

## 6. 实现计划（C++ 侧）

> **进度**：第 1、2、3 步的"几何 + 贴图"部分已实现（见 6.1）；颜色烘焙（第 4 步）未做。

1. **资源解析表**：由 `tools/resolve_block_textures.py` 预处理成紧凑映射表
   （`block(+meta) → 六个面的贴图路径`），C++ 只读表，不引入 JSON 依赖；
   服务器场景下每上传一个资源包预处理一次并缓存。
2. **UV 计算**：在建网格时（还是 grid 单位、方块内坐标精确已知）为每个**半边**（面角）记录 UV；
   或在写出时依据每个顶点保留的"方块内本地坐标"属性 + 面法线现算。
   不能在写出时用最终世界坐标反推——导出归一化（居中/缩放）会改变小数部分。
3. **输出**：OBJ 增加 `vt` 与 `usemtl`，MTL 按 (贴图, 颜色) 生成；
   或直接输出 GLB（材质用 baseColorTexture + baseColorFactor）。
4. **待处理细节**：楼梯/原木等"有属性变体"的方块需要 meta→state 映射；
   透明与双面（玻璃/树叶）；动画纹理取第一帧；tint（草/树叶的颜色表）。

### 6.1 已实现部分

| 组件 | 位置 | 说明 |
|---|---|---|
| 贴图表 | `Minecraft/TextureSupport/BlockTextureTable.h/.cpp` | 读 TSV、按 `block(:meta)` 查表；附带六面 UV 计算与法线→朝向判定。不依赖 CGAL / nbt++ |
| UV 记录 | `CgalLittletilesBuilder.cpp` 的合并过程 | 建网格后坐标是"方块内 0..1"，但导出前还会做居中/缩放，所以**在合并时**用 `世界坐标 − 方块坐标` 记下每个顶点的本地坐标；写出时再按面朝向算 UV |
| 写出 | 同上的 `MergeAndWriteToObj` | 输出 `mtllib` / `vt` / `usemtl` / `f v/vt`；同时生成 `.mtl` 并把用到的 PNG 复制到 OBJ 同目录（输出可整体搬走） |
| 调用 | `main.cpp` | 素材目录优先读环境变量 `LITTLETILES_ASSETS`，否则依次尝试 `./assets/1.12.2` 与 `../assets/1.12.2`；都找不到就只导出几何并给出提示 |

实测（chunk (-136,49)，8037 个 tile）：

```
顶点 63980 / 面 47070（与加贴图前完全一致，几何未受影响）
唯一 vt 5008 个，全部落在 [0,1] 内
5008 个 vt 中有 5004 个是"非 0/1 的中间值" → 确实在做按位置裁剪取样
材质 6 个、复制贴图 6 张
```

尚未实现：**颜色烘焙**。当前 MTL 的 `Kd` 写成白色，染色信息尚未用到；
Blender 不会把 `Kd` 与 `map_Kd` 相乘，因此下一步要按 (贴图, 颜色) 生成染色 PNG
（实测整个测试数据集只有 16 个组合，成本很低）。
