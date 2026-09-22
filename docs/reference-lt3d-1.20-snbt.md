# 参考实现研究：lt3d 与 LittleTiles 1.20 的 SNBT 结构格式

研究对象：

| 项目 | 版本 | 用途 |
|---|---|---|
| [LeekZhangx/lt3d](https://github.com/LeekZhangx/lt3d) | 浏览器端查看器（Vite + Three.js） | **1.20/1.21 结构 SNBT 的权威样本与解析口径**（本次分析的主要对象） |
| [CreativeMD/LittleTiles](https://github.com/CreativeMD/LittleTiles) | 分支 `1.20`（`c5694e1`） | 写入格式的**产出方**：`LittleGroup.save` / `LittleCollection.save` |
| [CreativeMD/LittleTiles](https://github.com/CreativeMD/LittleTiles) | 分支 `1.12` | 1.12.2 方言的产出方：`LittlePreview.savePreview` / `LittleNBTCompressionTools.writePreviews` |

> 两个上游仓库都**不随本库分发**，本文只记录结论与符号名（便于 grep 复核）。
> 本地副本路径只用于本次核对，不写进仓库。

## 0. 结论（先看这段）

1. **1.20 的结构 SNBT 与 1.12.2 是两套方言**，但共用同一套盒子编码与同一套坐标语义。
   区分方式与 LittleTiles 自己的判断一致：`tiles` 是**列表**→ 1.12.2；`tiles` 是**整数**（且存在 `t` 映射）→ 1.16+。
2. **lt3d 的 `src/core/adapter/LtAdapterV_1_21.js` 读的就是 1.20 方言**，它的
   `VersionDetector`（`txt.includes('t:{')` / `txt.includes('tile:{')`）与本项目的
   `LtStructure::DetectDialect()` 思路一致。
3. **lt3d 只做到了"能看"，没做到"能写回"，也没做版本互转**：
   - 它把三种输入统一成 `LtIR`，但 IR 里丢掉了 `min` / `structure`（只留 `id`、`name`）；
   - 子结构的 `grid` 根本没读（`_parseNode` 不看 `grid`）；
   - 没有序列化器，也没有 1.12.2 ↔ 1.20 的方块名映射。
4. **跨版本互转的关键数据在 LittleTiles 自己手里**：1.20 的
   `OldLittleTilesDataParser` 在读旧结构时会加载 mod 内资源 `1.12.2.txt`
   （412 条 `旧名§新名`），这就是 1.12.2 元数据 → 扁平化方块状态的权威映射表。
   本库把这张表内嵌（见下文 §5），并照搬它的兜底规则。

## 1. 1.20 方言的字段表

产出方（1.20 分支）：

```
LittleGroup.save(group)        = saveChild(group) + size / min / tiles / boxes / trans
LittleGroup.saveChild(group)   = s / t / grid / c / e
LittleCollection.save(content) = t: 方块状态 -> [颜色, 盒子, 颜色, 盒子, ...]
```

因此**任意一层**（根或子结构）的形状是：

```text
{
  s: { id:"fixed", n:"名字", b:[I;...], ex:[...], ... },   // 结构元数据（有结构时才有）
  t: { "minecraft:oak_log[axis=y]": [ [I;颜色], [I;x1,y1,z1,x2,y2,z2,...], ... ], ... },
  grid: 32,                                                // 该层的网格；等于默认 16 时**整个键不写**
  c: [ { 子结构 }, ... ],                                  // 子结构（有子结构时才写）
  e: { "扩展id": { 子结构 } },                             // 扩展子结构（本库忽略几何，原样保留）

  // 以下仅在根（LittleGroup.save，而非 saveChild）：
  min:  [I;x,y,z],        // 结构原点，grid 单位
  size: [I;x,y,z],        // 结构尺寸，grid 单位
  tiles: 18044,           // 递归总计：LittleTile 对象数
  boxes: 21748,           // 递归总计：盒子数
  trans: 1b               // 含半透明方块时才写（布尔）
}
```

要点（都有上游符号名可查）：

| 结论 | 依据 |
|---|---|
| `t` 的键是**方块状态字符串**（含 `[axis=y]` 这样的属性，属性序按方块定义序） | `LittleBlockRegistry.saveState` / `loadState` |
| 长度为 1 的 `[I;c]` 是**颜色标记**，作用于其后的盒子直到下一个标记 | `LittleCollection.load` / `save` |
| 颜色 `-1` 等于 `ColorUtils.WHITE`，即**未染色**（`LittleElement.isColored()`） | `LittleElement.isColored()` |
| 一个键下可以出现**多组**颜色标记（同方块不同染色） | `LittleCollection.save` 的 `[tile.color]` 前缀 |
| 颜色标记后没有任何盒子 → 该组在加载时被丢弃 | `LittleCollection.load` 的 `tileBoxes` 判断 |
| `grid` 只在**非默认**（≠16）时写出 | `LittleGrid.set` |
| 子结构的坐标与父级**同一空间**（无额外偏移），但**各层网格独立**；加载时子级会被换算到父级网格 | `LittleGroup.saveChild` 写 `grid`；`LittleGroup.load` 调 `convertTo(LittleGrid.get(nbt))` |
| `min` / `size` / `tiles` / `boxes` / `trans` 只出现在根 | `LittleGroup.save` 与 `saveChild` 的分工 |
| `tiles` 与 `boxes` 都是**递归合计** | `LittleGroup.totalTiles` / `totalBoxes` |
| 1.20 的 `t` 里可以直接写 1.12.2 名（`minecraft:wool:5`），`loadState` 会查映射表 | `LittleBlockRegistry.loadState(name, checkOld)` |

lt3d 的样本可以直接验证以上几条（`public/presets/1.21/`）：

| 文件 | 实测（本库读出的结果） |
|---|---|
| `block.txt` | `boxes:10, tiles:10`；10 个盒子、10 组、4 个子结构（1 个三级） |
| `block_face.txt` | `boxes:38, tiles:34`（其中一个键 6 个盒子） |
| `crane_truck.txt` | `boxes:4892, tiles:40`、`trans:1b`、无 `grid`（默认 16） |

导出包围盒与 `size/grid` 完全一致（16 网格）：`block.txt` 为 3×1×2 方块，
`block_face.txt` 为 11×1×14，`crane_truck.txt` 为 10.8125×3.9375×3.25。

## 2. 1.12.2 方言（对照）

```text
{
  structure: { name:"...", id:"fixed", blocks:[I;...] },
  grid: 32,                                  // 只在根、且非默认时写
  min:[I;...], size:[I;...], count:14748,    // count = 该层盒子数（previews.size()）
  pos: [ [I;x,y,z], ... ],                   // 仅当递归盒子数 ≥ 2000（LittlePreview.lowResolutionMode）
  tiles: [ { boxes:[[I;...],...], tile:{ block:"minecraft:wool:5", color:-13487566 } }, ... ],
  children: [ { tiles:[...], count:..., structure:{...} }, ... ]
}
```

要点：

| 结论 | 依据 |
|---|---|
| `tiles` 是**列表**，每个元素一组（同方块同染色），`boxes` 为多盒、`bBox`（或 `box`）为单盒 | `LittleNBTCompressionTools.writePreviews` / `ordinaryPreviewHandler` |
| 方块名把元数据写成**第三个冒号字段**（`minecraft:log:1`），也可写成 `block` + `meta` | `LittleTile.saveTileExtra` / `loadTileExtra` |
| 染色是 `tile.color`，无该键 = 未染色（`LittleTile` 而非 `LittleTileColored`） | `LittleTileColored.saveTileExtra` |
| `count` = **该层**盒子数；子结构各自有 `count` | `LittlePreview.savePreview`（`previews.size()`）与 `getTotalCount` |
| `pos` 的阈值判断用**递归**总数，但写出的位置只来自**该层** | `LittlePreview.savePreview`（`totalSize()` 判定 + `previews.size()` 遍历） |
| 子结构**继承父级网格**：子层不写 `grid`，读取时父级 context 直接传下去 | `LittlePreview.saveChildPreviews` / `LittlePreviews.getChild(context, nbt)` |

## 3. lt3d 的实现拆解（我们借了什么、避开了什么）

| 文件 | 作用 | 与本项目的关系 |
|---|---|---|
| `src/core/version/VersionDetector.js` | `tile:{` → 1.12，`t:{` → 1.21 | 与 `DetectDialect` 同思路（本库用类型而非子串，更稳） |
| `src/core/adapter/LtAdapterV_1_12.js` | 1.12 的 `tiles[]` → IR | 与本库 1.12 读取一致（`bBox` 单盒、`tile.block/color`） |
| `src/core/adapter/LtAdapterV_1_21.js` | 1.20 的 `t` → IR | 与本库 1.20 读取一致（颜色标记流） |
| `src/core/transformer/1.21/handlers/ElementHandlerV_1_21.js` | `t` 的颜色/盒子分流 | 本库 `ReadModernGroups` 的规则来源之一 |
| `src/core/ir/LtIR.js` | 统一中间表示 | **只保留 grid/size/tiles/structure(id,name)/children**：`min`、结构元数据、每层 grid 都丢了 |
| `src/core/geometry/transformable/*` | 角点偏移 / flip 的几何实现 | 与本库 `DecodeBoxAngleData` 的位域口径一致（bit 24..29 = down/up/north/south/west/east） |
| `src/core/tile3d/material/*` | 112/1.21 两套贴图表 | 说明**两代的方块名/贴图名不是一套**，跨版本必须显式映射 |

lt3d 没有做、而本项目必须做的三件事：

1. **写回**：没有序列化器 → 本库新增 `LtStructure::ToSnbt()`（两套方言都能写）。
2. **每层 grid**：`_parseNode` 不读 `grid` → 本库按层保存（`LtStructure::Group::grid`），
   父层 32、子层 16 的结构不会再被整体缩放。
3. **方块名映射**：lt3d 只是按版本选不同的贴图表 → 本库内嵌 LittleTiles 自己的
   `1.12.2.txt` 映射表做互转。

## 4. 与我们 `docs/snbt-format.md` 旧描述的差异

旧文档里"新版（1.16+）"的最小示例（`{boxes,grid,min,size,t:{...},tiles}`）**是对的**，
但缺了这几点，本次已补进 `docs/snbt-format.md`：

1. `t` 的**多条颜色标记**（同方块不同染色）与"空组被丢弃"；
2. 子结构 `c`、扩展 `e`、`trans`；
3. `s` 里的**名字键是 `n`**（不是 `name`，`name` 是 1.12.2 的写法）；
4. **每层 grid** 的写/读规则（1.20 每层独立、默认 16；1.12.2 子层继承父层）；
5. `tiles` / `boxes` 是**递归计数**，而 1.12.2 的 `count` 是**本级**盒子数。

## 5. 跨版本方块名映射（内嵌数据）

| | 内容 |
|---|---|
| 来源 | LittleTiles `1.20` 分支 `src/main/resources/1.12.2.txt`（412 行） |
| 格式 | 每行 `旧名` + `U+00A7` + `新名`，例如 `minecraft:wool:5§minecraft:lime_wool` |
| 覆盖 | `minecraft` 345 条 + `littletiles` 67 条；**其他模组方块不在表内** |
| 用途 | 1.12.2 → 1.20 查表；1.20 → 1.12.2 用其反表（同名多对一时取最早出现者） |
| 兜底 | 与 LittleTiles 一致：查不到时**丢掉第二个冒号之后的部分**（模组方块的扁平名通常没有元数据） |
| 落库形式 | `Galib/Minecraft/BlockStateMapData.cpp`，由 `tools/gen_block_state_map.py` 生成；`Galib/Minecraft/BlockStateMap.cpp` 负责查表 |
| 许可证 | LittleTiles 为 **LGPL-3.0**（与本库相同），已在 `docs/release-and-licensing.md` 中署名 |

## 6. 验证记录（本文结论的实测证据）

测试数据：`test_snbt/SHB_05_Contemporary_Style_House_v1.1.txt`（1.12.2，807 KB 单行）
与 lt3d 的 `public/presets/1.21/*.txt`。

| 验证项 | 结果 |
|---|---|
| 1.12.2 解析 | 18044 盒子 / 142 组 / 46 子结构 / grid 32，与既有基线一致 |
| 1.20（1.21）解析 | `block.txt` 10 盒 / `block_face.txt` 38 盒 / `crane_truck.txt` 4892 盒，与文件里的 `boxes:` 一致 |
| 导出几何（1.20 输入） | 包围盒 = `size/grid`（3×1×2、11×1×14、10.8125×3.9375×3.25） |
| 1.12.2 → 1.20 → 1.12.2 | 21748 个 `[I;...]` 数组**逐个字节相同**（含 5348 个变换盒子）；142 组方块+染色、47 层的 `count` 全部一致 |
| 1.20 → 1.12.2 → 1.20 | 22 个数组全同（`block.txt`），组数/盒数不变 |
| 导出 OBJ 对照（同一房屋） | 原件 / 转成 1.20 / 再转回 1.12.2：顶点 146044、面 105727、包围盒完全相同，**后两者逐字节相同** |

自动化：`tests/lt_structure_test.cpp`（`ctest`，46 项断言 + 可选传入真实结构文件）。
