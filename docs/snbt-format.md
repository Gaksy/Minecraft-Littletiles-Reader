# LittleTiles 结构 SNBT 格式

游戏里用 LittleTiles 的"结构"功能复制一块建筑时，复制出来的就是一段 **SNBT 文本**
（用户通常存成 `.txt`，新版导出为 `.struct`）。本项目的 SNBT 入口读的就是它。

与存档路径的区别：存档里的数据是**区块 + 方块坐标**，结构里是**结构空间坐标**
（跨很多方块），且结构没有"周围的普通方块"，因此不涉及完整方块导出与邻居剔除。

## 1. 两种方言

**旧版（1.12 时代，本仓库测试样本即此格式）**

```text
{tiles:[ {boxes:[[I;x1,y1,z1,x2,y2,z2,...], ...], tile:{block:"...", color:-1}}, ... ],
 min:[I;30,0,28], size:[I;708,370,928], pos:[...], children:[...],
 grid:32, count:14748, structure:{id:"fixed", name:"..."}}
```

**新版（1.16+）**

```text
{boxes:34, grid:32, min:[I;0,0,0], size:[I;32,96,96],
 t:{"minecraft:dirt":[[I;-1],[I;0,0,32,32,32,64]], ...}, tiles:4}
```

新版把 `tiles` 列表压成了 `t` 映射：**方块全名 → int 数组列表**，其中长度为 1 的数组
是**颜色标记**（作用于它后面的盒子）。本项目目前支持旧版；新版只需在读取器里多一层
转换（`t` 的每个键值对直接变成"一个分组"）。

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
- 角点顺序是 `BoxCorner` 枚举序 = **EUN, EUS, EDN, EDS, WUN, WUS, WDN, WDS**
  （CreativeCore 1.12.2 的字节码已核对）；
- 偏移量接在 `array[7]` 之后，每 32 位装两个 **16 位有符号**数（**高 16 位在前**）；
- 读取顺序是**正序**：角点从 EUN 到 WDS，每个角内 x→y→z，依次消费偏移量
  （官方 `getTiltedCorners` 的字节码：`activeBits` 从 0 递增）；
- flip 位在 **bit 24~29**（官方 `getFlipped(i)` = `bitIs(indicator, 24 + i)`），
  顺序与 `Facing` 序数一致：down/up/north/south/west/east。

> 曾经的早期 MATLAB 工具（`python/IntArrayInterpreter.py`）是**从尾部倒着读**的，
> 在 20000 组随机输入里有约一半结果不同；本仓库实测那栋房子里 5314 个变换盒子里有
> **360 个（6.8%）** 会被读错。已按官方实现改正（`DecodeBoxAngleData`）。

## 3. 坐标与 UV

- 结构坐标是 **grid 单位**（`grid` 字段，实测房屋为 32），`min` 是结构原点；
- 网格化时按 `1/grid` 缩放到方块单位并减去 `min`，模型即落在原点附近，
  之后交给 OBJ 构建器的居中 / 归一化选项；
- UV 用"**所在方块单元内的相对坐标**"（`pos - floor(pos)`）——一个网格会横跨多个
  方块单元，不能像存档路径那样用网格级方块坐标反推；
- 裁剪策略与存档路径一致（偏移后超出自身盒子时才裁），由 `ClipTileEntityToBox` 完成。

## 4. 子结构（children）

结构里可以嵌子结构：门（`door`/`slidingDoor`/`advancedDoor`）、粒子发射器
（`particle_emitter`）、灯等，它们带动画与开关参数（`axisCenter`、`duration`、
`state`、`animation` 里的曲线数组……）。当前实现**只取几何**、忽略行为参数，
即导出的是"关门状态下的静态模型"。

## 5. 模组方块与贴图

结构里会出现模组方块，必须另外准备贴图（见 `tools/add_mod_textures.py`）：

- `littletiles:ltcoloredblock[:meta]`：LT 自己的彩色方块，**白色底图 + tile 颜色**。
  meta 0..11 对应贴图 `ltcolored0..11`（meta 10 = clay），12 起是 light_clean /
  岩浆 / 白色岩浆；`ltcoloredblock2` 是另外五个变体。
- `kirosblocks:colored_*`：Kiro's Basic Blocks，同样是**白色可染色**贴图
  （`mcmod.info` 原文：white dyeable versions of certain block textures）。

染色走现有烘焙链路：`TileMaterial{block_id, color}` → 逐像素乘色。实测
`ltcolored10` 底色 223 × 颜色 `0x64`(100) → 87.4，导出贴图实测 87 ✔

## 6. 实测（`SHB_05_Contemporary_Style_House_v1.1.txt`）

| 项目 | 数值 |
|---|---|
| 文件 | 807 KB 单行 SNBT |
| grid / min / size | 32 / [30,0,28] / [708,370,928]（22.1 × 11.6 × 29.0 方块） |
| 盒子 | 18044（顶层 14748 + 子结构 3296），其中变换盒子 5314 |
| 材质分组 / 子结构 | 142 / 46 |
| 导出 | 104715 面 / 142363 顶点 / 32 个材质 / 7.6 MB，2.6 s |
| 导出包围盒 | 22.1250 × 11.5625 × 29.0000 方块（与 `size/grid` **完全一致**） |
