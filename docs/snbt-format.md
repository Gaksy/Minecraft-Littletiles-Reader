# LittleTiles 结构 SNBT 格式（两代方言 + 互转）

游戏里用 LittleTiles 的"结构"功能复制一块建筑时，复制出来的就是一段 **SNBT 文本**
（用户通常存成 `.txt`，新版蓝图导出为 `.struct`）。本项目的 SNBT 入口读的就是它。

与存档路径的区别：存档里的数据是**区块 + 方块坐标**，结构里是**结构空间坐标**
（跨很多方块），且结构没有"周围的普通方块"，因此不涉及完整方块导出与邻居剔除。

> 格式的**逐条依据**（上游符号名、验证记录、lt3d 的拆解）见
> [`reference-lt3d-1.20-snbt.md`](reference-lt3d-1.20-snbt.md)。
> 本文是使用口径，那份是研究记录。

## 1. 两种方言

### 1.1 旧版：1.12.2（LittleTiles 1.5.x）

```text
{tiles:[ {boxes:[[I;x1,y1,z1,x2,y2,z2,...], ...], tile:{block:"minecraft:wool:5", color:-13487566}}, ... ],
 grid:32, count:14748, min:[I;30,0,28], size:[I;708,370,928],
 pos:[[I;x,y,z], ...],
 structure:{name:"...", id:"fixed", blocks:[I;]},
 children:[ {tiles:[...], count:..., structure:{...}}, ... ]}
```

- `tiles` 是**列表**：每个元素 `{ tile 材料, boxes/bBox 盒子 }`；
- 方块名把元数据写成第三个冒号字段（`minecraft:log:1`），也可以写成 `block` + `meta:1`；
- `color` 只染色 tile 才有；`count` 是**本级**盒子数；
- 子结构放在 `children`，**继承父层 grid**（子层自己写 `grid` 会被忽略，与 1.12.2 一致）。

### 1.2 新版：1.16+（1.20 / 1.21，LittleTiles 1.6+）

```text
{s:{id:"fixed", n:"结构名", b:[I;...]},
 t:{"minecraft:oak_log[axis=y]":[[I;-1],[I;0,0,0,16,16,16]],
    "minecraft:lime_wool":[[I;-13487566],[I;0,0,0,8,8,8],[I;8,8,8,16,16,16]]},
 grid:16, c:[ { 子结构 }, ... ],
 min:[I;0,0,0], size:[I;48,16,32], tiles:10, boxes:10, trans:1b}
```

- `t` 是**映射**：方块状态字符串 → int 数组流；长度为 1 的数组是**颜色标记**，
  作用于它后面的盒子，直到下一个颜色标记（所以同一个方块的不同染色在同一个键下）；
- 颜色 `-1` 表示**未染色**（等于 LittleTiles 的白色）；
- 方块名是**扁平化名字 + 方块状态**（`minecraft:lime_wool`、`minecraft:oak_log[axis=y]`）；
- `s` 里的名字键是 **`n`**（不是 `name`）；`b` 是 `blocks`，`ex` 是 `signal`；
- `tiles` / `boxes` 是**递归合计**（本层 + 所有子结构），`trans` 表示含半透明方块；
- 子结构放在 `c`，**每层各自带 grid**（等于默认 16 时该键不写）；`min`/`size`/计数只在根。

### 1.3 怎么判断版本

`LtStructure::DetectDialect()` 按字段**类型**判断（与 LittleTiles 的
`OldLittleTilesDataParser.isOld` 同思路）：

| 情况 | 结论 |
|---|---|
| `tiles` 是**列表** | 1.12.2 |
| `tiles` 是整数（或存在 `t` 映射） | 1.20 |
| 两者都不是 | 不是结构文件，直接报错 |

## 2. 盒子编码（两代相同，也是存档里的编码）

来自官方 `LittleBox.create`（对照 LT 1.21 源码，并用 LT 1.12.2 的字节码复核）：

| 数组长度 | 含义 |
|---|---|
| `6` | 普通 AABB：`[x1,y1,z1,x2,y2,z2]` |
| `> 6` 且 `array[6] < 0` | 带角度偏移的 transformable box |
| `7` 或 `11` 且 `array[6] >= 0` | 旧 slice 格式，**按普通 AABB 处理** |
| 其它 | 非法 |

> ⚠️ 只判断"长度 > 6"是不够的：旧 slice 的数组也会超过 6，把它们当偏移解会得到垃圾几何。

**带角度偏移时的载荷**（`LittleTransformableBox`）：

- `array[6]` 是指示位：bit `i*3 + {0,1,2}` 分别表示第 `i` 个角的 x/y/z 是否偏移；
- 角点顺序是 `BoxCorner` 枚举序 = **EUN, EUS, EDN, EDS, WUN, WUS, WDN, WDS**；
- 偏移量接在 `array[7]` 之后，每 32 位装两个 **16 位有符号**数（**高 16 位在前**）；
- 读取顺序是**正序**：角点从 EUN 到 WDS，每个角内 x→y→z，依次消费偏移量；
- flip 位在 **bit 24~29**（官方 `getFlipped(i)` = `bitIs(indicator, 24 + i)`），
  顺序与 `Facing` 序数一致：down/up/north/south/west/east；
- 指示位的**最高位（bit 31）**是"这是 transformable box"的标记（`LittleBox.create`
  要求 `array[6] < 0`）。

写回时用 `EncodeBoxArray()`（`LittleTilesEntity.cpp`）做**逆运算**：指示位 + 两个 16 位打包，
实测整栋房子的 21748 个数组（含 5348 个变换盒子）**逐字节还原**。

> 曾经的早期 MATLAB 工具（`python/IntArrayInterpreter.py`）是**从尾部倒着读**的，
> 在 20000 组随机输入里有约一半结果不同；本仓库实测那栋房子里 5314 个变换盒子里有
> **360 个（6.8%）** 会被读错。已按官方实现改正（`DecodeBoxAngleData`）。

## 3. 坐标与 UV

- 结构坐标是 **grid 单位**（`grid` 字段，默认 16），`min` 是结构原点；
- **每一层有自己的 grid**：读进来的每个分组都记住来源层的 grid
  （`LtStructure::Group::grid`），网格化时按**该层**的 grid 缩放到方块单位再减去 `min`。
  旧版子结构继承父层，新版每层独立（缺省 16），两者都被正确处理；
- UV 用"**所在方块单元内的相对坐标**"（`pos - floor(pos)`）——一个网格会横跨多个
  方块单元，不能像存档路径那样用网格级方块坐标反推；
- 裁剪策略与存档路径一致（偏移后超出自身盒子时才裁），由 `ClipTileEntityToBox` 完成。

## 4. 子结构与行为参数

结构里可以嵌子结构：门（`door`/`slidingDoor`/`advancedDoor`）、粒子发射器
（`particle_emitter`）、灯、告示牌等，它们带动画与开关参数（`axisCenter`、`duration`、
`state`、`animation` 里的曲线数组……）。当前实现**只取几何**，忽略行为参数，
即导出的是"关门状态下的静态模型"。

互转时**结构元数据原样带走**（只改容器键名与结构 id），因为两代的门参数编码方式
并不相同（LittleTiles 自己也是把 1.12.2 的门参数**重新编码**成 1.20 的时间线）。
因此：

| 结构 | 互转后的表现 |
|---|---|
| `fixed` / `ladder` / `message` / `storage` / 传感器等（两代字段同名或只有键名差异） | 键名改写即可（`name`↔`n`、`blocks`↔`b`、`signal`↔`ex`、`parent`↔`k`），可用 |
| 门（`door`/`slidingDoor`/`doorActivator`/`advancedDoor` ↔ `axis`/`sliding`/`activator`/`door`） | id 会改写，**动画/开关参数只做透传**，游戏内可能需要重新设置（CLI 会给出 note） |

## 5. 两代互转（1.20 ↔ 1.12.2）

命令行：

```sh
# 1.12.2 结构 -> 1.20（默认写到输入文件旁边）
LittleTilesReader --convert 1.20 --input House.txt

# 指定输出，或反向转换
LittleTilesReader --convert 1.12.2 --input House.struct --out House_1.12.2.txt
```

结构文件本身就是**纯文本 `.txt`**（游戏里复制到剪贴板的就是这段文本；1.20 蓝图另存为
`.struct`，两者都能读）。所以转换产物也是 `.txt`：不给 `--out` 时写成
`<输入名>_<目标版本>.txt`，`--out result`（不带扩展名）会补成 `result.txt`。

也可以走 job 文件（`"mode": "snbt_convert"`，见 [`job.md`](job.md)），
或交互模式：输入结构文件后会多问一句"是否转版本"。

**方块名映射**用的是 LittleTiles 自己那张表（`1.12.2.txt`，412 条，见
`reference-lt3d-1.20-snbt.md` §5），落在 `BlockStateMap`：

```
minecraft:wool:5        -> minecraft:lime_wool
minecraft:log:1         -> minecraft:spruce_log[axis=y]
minecraft:oak_log[axis=y] -> minecraft:log            （反表）
```

查不到的名字（各模组方块）按 LittleTiles 的兜底规则处理：**砍掉第二个冒号之后的内容 /
方块状态**，保留基础名，并在结束时报出清单，例如：

```
warning: 11 block names have no counterpart in 1.20 (they keep their base name), e.g.: kirosblocks:...
```

### 5.1 有意保留的差异（不算 bug）

| 现象 | 原因 |
|---|---|
| 组的**顺序**可能变（同方块不同染色会并到一个键下） | 1.20 的 `t` 是映射：同方块的多组会被合并；LittleTiles 自己保存时也是哈希序。几何与材质不受影响 |
| 显式的 `color:-1` 转过去会变成"无颜色" | `-1` 就是 LittleTiles 的白色（`LittleElement.isColored()` = `color != WHITE`），两代语义相同；只有**跨代**转换才会归一化，同代重写会原样保留 |
| 1.12.2 里 `tID`、`invisible` 等 tile 私有键在转成 1.20 后会丢 | 1.20 的 `t` 只有"方块状态 + 颜色 + 盒子"三个维度，没有位置放这些键（同代重写不丢） |
| 1.20 的 `trans` 转到 1.12.2 会丢 | 1.12.2 没有这个字段（导出几何不受影响） |
| 1.20 侧网格与父层不同的子结构，转到 1.12.2 时会被**换算到根层网格** | 1.12.2 全结构只有一个网格；只有整数倍时能精确换算，否则会给出 note 并原样保留 |

## 6. 模组方块与贴图

结构里会出现模组方块，必须另外准备贴图（见生成端的 `tools/add_mod_textures.py`，
该脚本不随本库分发）：

- `littletiles:ltcoloredblock[:meta]`：LT 自己的彩色方块，**白色底图 + tile 颜色**。
  meta 0..11 对应贴图 `ltcolored0..11`（meta 10 = clay），12 起是 light_clean /
  岩浆 / 白色岩浆；`ltcoloredblock2` 是另外五个变体。
- `kirosblocks:colored_*`：Kiro's Basic Blocks，同样是**白色可染色**贴图。

染色走现有烘焙链路：`TileMaterial{block_id, color}` → 逐像素乘色。实测
`ltcolored10` 底色 223 × 颜色 `0x64`(100) → 87.4，导出贴图实测 87 ✔

## 7. 实测

### 7.1 1.12.2 房屋（`SHB_05_Contemporary_Style_House_v1.1.txt`）

| 项目 | 数值 |
|---|---|
| 文件 | 807 KB 单行 SNBT |
| grid / min / size | 32 / [30,0,28] / [708,370,928]（22.1 × 11.6 × 29.0 方块） |
| 盒子 | 18044（顶层 14748 + 子结构 3296），其中变换盒子 5314 |
| 材质分组 / 子结构 / 层数 | 142 / 46 / 47 |
| 导出 | 105727 面 / 146044 顶点，6.3 MB，0.9 s（Release 构建，无素材包=纯几何） |
| 导出包围盒 | 22.1250 × 11.5625 × 29.0000 方块（与 `size/grid` **完全一致**） |
| 互转后导出 | 顶点/面/包围盒与原件完全相同；转出再转回后两者**逐字节相同** |

### 7.2 1.20/1.21 结构（lt3d 的 `public/presets/1.21/`）

| 文件 | 盒子 / 组 / 子结构 | 导出包围盒（方块） | 对应 `size/grid` |
|---|---|---|---|
| `block.txt` | 10 / 10 / 4 | 3.0 × 1.0 × 2.0 | 48/16, 16/16, 32/16 ✔ |
| `block_face.txt` | 38 / 34 / 0 | 11.0 × 1.0 × 14.0 | 176/16, 16/16, 224/16 ✔ |
| `crane_truck.txt` | 4892 / 40 / 0 | 10.8125 × 3.9375 × 3.25 | 173/16, 63/16, 52/16 ✔ |

自动化测试见 `tests/lt_structure_test.cpp`（`ctest`；也可把真实结构文件作为参数传入）。
