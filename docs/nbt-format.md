# Anvil / NBT / LittleTiles 数据格式

本文件是从**真实存档**里验证过的格式说明（生成脚本见 `.ai/tools/`，如未保留可按文中命令自行复现）。

## 1. region 文件（.mca）

`r.<rx>.<rz>.mca`：

| 偏移 | 含义 |
|---|---|
| 0 – 4095 | 1024 个 4 字节索引项：高 3 字节 = 起始扇区号，低 1 字节 = 扇区数 |
| 4096 – 8191 | 1024 个 4 字节时间戳 |
| 扇区 | 每 chunk：4 字节长度（大端，含压缩类型字节）+ 1 字节压缩类型 + 压缩数据 |

实现见 `Anvil.cpp:206`（索引解析）、`Anvil.cpp:264`（解压）、`Anvil.cpp:293`（NBT 解码）。

- **压缩类型只支持 2（zlib）**，其他类型会被直接拒绝。
- 区块在 region 内的槽位下标 = `region_chunk_x + region_chunk_z * 32`。

坐标换算（`MinecraftCoord.cpp:33,40,44`）：

```
region_coord       = floor(chunk / 32)
region_chunk_coord = CoordSwap(chunk, 32)
chunk(from block)  = CoordSwap2D(block.xz, 16)
```

`CoordSwap(v, base)` 对负数取 `(base-1) - ((-v-1) mod base)`，是"负坐标映射到 0..base-1"的关键，不要自行重写。

## 2. chunk NBT

- **1.12**（本项目测试数据）：`root → "Level" → "TileEntities"`
- **1.18+**：level 内容摊平到根上

当前 `AnvilReader` **硬编码** `.at("Level")`（`Anvil.cpp:176`），只支持 1.12 结构；
新增的 `readChunkNbt` 已同时兼容两种结构。**没有任何 `DataVersion` 校验**，版本不符会静默误解析。

## 3. LittleTiles tile entity

对 `test_region*` 全量扫描（32 个含 LT 数据的 chunk / 1769 个 TE / 3595 个 tile）：

```
TileEntities[i] = {
    id      : "minecraft:littletilestileentity"
    x, y, z : int            // 世界方块坐标
    grid    : int            // 可选！缺失时解析器默认 16
    content : {
        tiles    : [ { block: string, box|boxes: int[], color: int? }, ... ]
        children : ...
    }
}
```

| 字段 | 实测统计 |
|---|---|
| TE 字段 | `grid`(1640)、`x/y/z`(1769)、`id`(1769)、`content`(1755)；另有 14 个非 LT 实体带 `Items`/`Lock` |
| content 字段 | `tiles`(1755)、`children`(1755) |
| tile 字段 | `block`(3595)、`boxes`(2849)、`box`(746)、**`color`(1584 ≈ 44%)** |
| grid 取值 | 64(979) / 32(139) / 8(245) / 4(103) / 2(169) / 1(5) / **缺失(129)** |
| block id | 13 种；最高频 `minecraft:quartz_ore`(1548)、`minecraft:purpur_block`(819)、`minecraft:bedrock`(757) |

### 3.1 解析器当前读取的 key

| NBT key | 位置 | 代码 |
|---|---|---|
| `TileEntities` | chunk level | `ChunkTileEntities.cpp:102-107` |
| `grid`（可选） | block TE | `BlockTileEntities.cpp:66` |
| `id` | block TE | `BlockTileEntities.cpp:71` |
| `content.tiles` | block TE | `BlockTileEntities.cpp:74` |
| `x/y/z` | block TE | `BlockTileEntities.cpp:82-86` |
| `block` | tile | `BlockTileEntities.cpp:94` |
| `boxes`（列表）/ `box`（单值） | tile | `BlockTileEntities.cpp:166,169` |

### 3.2 尚未解析但确实存在的数据

1. **`color`（ARGB int）**：实测值如 `0xFFFFBE00`、`0xFF242424`，alpha 恒为 `0xFF`。
   覆盖约 44% 的 tile。这是做材质/上色的关键信息，目前被完全丢弃。
2. **`block` 字符串里的 meta 后缀**：如 `minecraft:stone:3`、`minecraft:stained_glass:15`；
   LT 自己的染色块是 `littletiles:ltcoloredblock`。

### 3.3 box / boxes 的编码

每个 box 是 int 数组，前 6 个整数为 `[x1,y1,z1,x2,y2,z2]`（grid 单位）。
长度 > 6 时：`arr[6]` 是**状态位域**，`arr[7..]` 是偏移值（每个 int32 打包两个 16 位有符号数，高 16 位在前）。

状态位域布局：

| 位 | 含义 |
|---|---|
| `3i .. 3i+2`（i=0..7） | 第 i 个角点的 x / y / z 偏移是否启用 |
| 24 / 25 / 26 | Flipped：down / up / north |
| 27 / 28 / 29 | Flipped：south / west / east |

角点顺序（`AngleID`，`LittleTiles.h:31`）：

```
EUN=0  EUS=1  EDN=2  EDS=3  WUN=4  WUS=5  WDN=6  WDS=7
命名 = E/W(x) + U/D(y) + N/S(z)，表示该轴取 pos_1 还是 pos_2
```

偏移值消费顺序：按 AngleID 升序，每个角点内依次 x → y → z（`setAngleOffsetStateData_`，`BlockTileEntities.cpp:237`）。

> **验证记录**：README 的示例 SNBT（状态 `-2135499923` = `0x80B6DB6D`）经 C++ 算法、
> 仓库内 `python/IntArrayInterpreter.py`、以及 README 给出的 MATLAB 输出三者结果**完全一致**
> （EUN(-2,0,-1) … WDS(2,0,1)）。位域与偏移解码已被确认正确。

## 4. 坐标系统

| 类型 | 定义 | 位置 |
|---|---|---|
| `BlockCoordinate` | `Coordinate3D<int32>` 世界方块坐标 | `MinecraftCoord.h:33` |
| `ChunkCoordinate` | `Coordinate2D<int32>` 世界区块坐标 | `MinecraftCoord.h:45` |
| `RegionCoordinate` | `Coordinate2D<int32>` region 文件坐标 | `MinecraftCoord.h:37` |
| `RegionChunkCoordinate` | `Coordinate2D<int32>` 区块在 region 内的 0..31 | `MinecraftCoord.h:41` |
| `EntityCoordinate` | `Coordinate3D<float>` | `MinecraftCoord.h:29` |
| `LittleTilesCoord` | `Coordinate3D<double>` | `LittleTilesCoord.h:28` |
| `GridType` / `OffsetType` | `int32_t` / `int16_t` | `LittleTilesCoord.h:26-27` |

从 tile 到世界坐标：`顶点(grid 单位) / grid + 方块坐标`
（`getVerticesApplyGrid`、`applyGrid` `CgalLtSupport.cpp:201`、`applyWorldOffset` `:181`）。

## 5. 浮点精度

- `CgalTypeDef.h:30,36`：Windows → `FloatType=float` + `Simple_cartesian<float>`；
  macOS → `double`。**同一输入在 Windows/macOS 上几何结果可能不同。**
- 但 `LittleTilesCoord` 恒为 `double`，`applyGrid` 又除以 `static_cast<float>(grid)`
  （`CgalLtSupport.cpp:201` 附近），`margeAndWriteToObj` 里还硬编码了
  `CGAL::Simple_cartesian<double>`（`CgalLittletilesBuilder.cpp:151`）→ 目前是混用状态。
- 服务器端若要"同输入同输出"，建议统一为 `double`（尚未实施）。
