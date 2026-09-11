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

## 5.1 使用自定义材质包（资源包叠加）

想用别的材质包（工作室内部包不入库，见 `.gitignore` 的 `/texture`）导出时，
先把材质包与原版素材**合并**成一个 assets 根：

```sh
python3 tools/build_assets_from_pack.py --pack "texture/INCEPTION texture V1.4.zip" --out assets/pack_v14
```

工具做三件事：

1. 用**原版 1.12.2** 的 `blockstates`/`models` 解析出 `block:meta → 六面贴图` 映射表。
   材质包大多是 1.13+ 命名（`block/xxx`、扁平化 blockstate），模型对不上，
   所以默认只借用它的贴图；`--use-pack-models` 才会连模型一起覆盖（仅限 1.12.2 命名的包）。
2. 按映射表**逐张**挑贴图：材质包里有就用材质包的，没有就回退原版——
   与游戏内资源包叠加规则一致。只有被引用到的 PNG 会被复制，PBR 法线/高光
   （`*_n.png` / `*_s.png`）不会被带进来。
3. 复制 `block_ids.tsv`，并写一份 `pack-source.txt` 记录来源，便于回溯。

产物是标准 assets 根，两种指定方式：

```sh
LITTLETILES_ASSETS=assets/pack_v14 ./LittleTilesReader     # 环境变量
# 或者运行时在 "assets root (blank = auto-detect):" 一问里直接填路径（可留空）
```

实测（INCEPTION texture V1.4 + 1.12.2 存档）：

| 项目 | 结果 |
|---|---|
| 映射表 | 455 个键（与原版一致） |
| 贴图来源 | 材质包 **159 张** + 原版兜底 142 张，缺失 0 |
| 分辨率 | 原版 16×16 → 材质包 128/256/512/1024 |
| 合并素材根 | `assets/pack_v14` ≈ 69 MB |
| `test_region_medim` 11×11 导出 | OBJ 34.3 MB / 388744 面（**与原版完全一致**）+ 贴图 77 张 18 MB；写出文件 1.4 s → **11.5 s**（烘焙 512²/1024² 变慢） |
| `test_region_large` 11×11 导出 | OBJ 177.5 MB / 2036139 面（与原版一致）+ 120 张贴图；写出文件 7.1 s → **16.4 s** |

（数字来自 `docs/benchmark.md` 里 2026-09-12 那一节，`tools/benchmark.py --assets assets/pack_v14`。）

注意事项：

- `.zip` 直接支持；**`.rar` 请先手动解压**（Python 读不了 rar），再 `--pack <解压目录>`。
- 材质包里的 `mcpatcher/` CTM（连接纹理）暂不支持：用的是它连接纹理的基础贴图。
- 贴图变大后 OBJ 本身不变，但 `..._textures/` 会显著变大（medim 从 0.3 MB 变 18 MB），
  导出 11×11 那种规模前先确认磁盘空间。

## 5.2 UV 验证模型（人工核对用）

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

> **进度**：第 1~3 步已实现（见 6.1），颜色与生物群系染色烘焙也已实现（见 6.2）。

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
| 写出 | 同上的 `MergeAndWriteToObj` | 输出 `mtllib` / `vt` / `usemtl` / `f v/vt`；同时生成 `.mtl`，把用到的 PNG 写进 OBJ 旁边的 `<obj 名>_textures/` 子目录（`map_Kd` 指向该子目录，输出可整体搬走） |
| 调用 | `main.cpp` | 素材目录优先读环境变量 `LITTLETILES_ASSETS`，否则依次尝试 `./assets/1.12.2` 与 `../assets/1.12.2`；都找不到就只导出几何并给出提示 |

实测（chunk (-136,49)，8037 个 tile）：

```
顶点 63980 / 面 47070（与加贴图前完全一致，几何未受影响）
唯一 vt 5008 个，全部落在 [0,1] 内
5008 个 vt 中有 5004 个是"非 0/1 的中间值" → 确实在做按位置裁剪取样
材质 6 个、复制贴图 6 张
```

### 6.2 颜色与生物群系染色烘焙（已实现）

因为 OBJ/MTL 的多数导入器（含 Blender）**不会**把 `Kd` 与 `map_Kd` 相乘，
所以颜色必须乘进像素：材质键从"贴图"扩展为 **(贴图, 生物群系 tint, tile 颜色)**，
每个组合烘焙成一张 PNG，命名形如 `blocks_purpur_block_cffffbe00`。

新增两个模块（都只依赖 zlib，不引入第三方库）：

| 组件 | 位置 | 说明 |
|---|---|---|
| PNG 读写 | `Minecraft/TextureSupport/PngImage.h/.cpp` | 读：8 位、非交错、颜色类型 0/2/3/4/6，统一转 RGBA；写：8 位 RGBA |
| 染色烘焙 | `Minecraft/TextureSupport/TextureBaker.h/.cpp` | 按 `贴图 × tint × tile 颜色` 逐像素相乘；tile 颜色的 alpha 会同时调制透明度 |

**tint 颜色用的是固定默认值**（草 `#91BD59`、树叶 `#79C05A`），不是从 `colormap` 采样：

1. LittleTiles 的存档**不记录每个 tile 的生物群系**，参考实现（Java 模组）也是用一个固定坐标
   去取默认生物群系色，本质相同；
2. `textures/colormap/*.png` 是三角形布局（无效区是白色），不同版本的索引约定还不一致，
   用常量更简单、更可预测。

模型里哪些面带 `tintindex` 已由解析工具记录进映射表（`<face>_tint` 列），
1.12.2 里只有 29 个模型需要染色：`grass`、`leaves`、`vine_*`、`waterlily`、
`stem_*`、`redstone_*`、`tinted_cross`、`flower_pot_fern`。

验证（都不依赖肉眼）：

```
PNG 解码   : 478 张 vanilla 贴图与 Python 参考解码器逐像素一致（0 处不一致）
染色公式   : 4 组（仅 tint / 仅 tile 色 / 两者 / 都不）与 Python 公式逐像素一致
端到端     : chunk(-136,49) 的材质数由 6 增到 8（purpur_block 的 3 种染色 +
             bedrock 的染色被区分开），MTL 引用的 8 张贴图全部存在
```

### 6.3 完整方块（普通方块）导出（已实现）

LittleTiles 的 tile entity 是**挂在普通方块上**的（实测 25/25、112/112 都落在实心方块上），
所以只导 tile 会得到一个"残缺"的模型。完整方块导出补上这一部分：

| 组件 | 位置 | 说明 |
|---|---|---|
| ID→名字表 | `tools/generate_block_id_table.py` + `Minecraft/BlockIdTable.{h,cpp}` | 1.12 的 `Sections[].Blocks` 存的是数字 ID，需要映射回 `minecraft:<name>[:meta]` |
| 区块方块解析 | `Minecraft/ChunkBlocks.{h,cpp}` | 读 `Sections[].Blocks/Data/Add`；`Add` 是高位 nibble 数组（LT 自己的方块 id 1321 就走这里）；再标记出 LT 宿主方块 |
| 立方体生成 | `Minecraft/CgalSupport/CgalWorldBlocks.{h,cpp}` | 跳过空气与 LT 宿主；按 (id, meta) 分组，每组一个网格；可选邻居剔除 |
| 增量写出 | `CgalSupport` 的 `ObjMeshBuilder` | 逐批 `AddMesh` 后即可释放，避免大范围导出时内存爆掉 |

实测（`test_region_large`，chunk (-7,-26) 周围 11×11 个区块，共 121 个区块）：

```
耗时 43.8 s
顶点 2,681,607 / 面 2,034,653 / 唯一 vt 130,175 / usemtl 切换 35,719
材质 120 个（含 tile 染色与草方块顶面的 tint），OBJ 190 MB
```

几个实现中踩到的坑（都已在代码注释里标注）：

1. `Sections[].Y` 是 **TAG_Byte**（不是 Int），按 `tag_int` 取会抛 `std::bad_cast`；
2. 这个 libnbt++ 版本在 macOS 上对数组**不能**用 `value::as<tag_byte_array>()`，
   要用 `static_cast<const nbt::tag_byte_array&>(value.get())`（原代码处理 `box` 时已有先例）；
3. 大范围导出必须**增量合并**：先把所有网格收进 vector 会因内存占用过高被系统杀掉
   （实测 121 个区块在 22 秒时被 kill）；改成逐批合并后内存稳定在 ~70 MB；
4. 别把贴图表放进逐网格的循环里——那会把 TSV 重读几十万次（实测 6 分钟 → 44 秒）。
