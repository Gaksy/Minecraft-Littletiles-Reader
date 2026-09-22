# 导出基线与实测数据

由测试数据侧的 `python3 tools/benchmark.py --write docs/benchmark.md` 生成，每次运行追加一节。
各存档的推荐参数写在 `tools/benchmark.py` 的 `CASES` 里，与 README「测试存档」表一致。
（`tools/` 与 `data/` 都不随本库分发，见 `docs/README.md` 的说明。）

## 2026-09-12 03:56

- 时间：2026-09-12 03:56
- 提交：8a2675f（工作区有未提交改动）
- 构建：Debug（/Users/external_elliott/Development/minecraft-littletiles-reader/cmake-build-debug/LittleTilesReader）
- 机器：Apple M3 / Darwin 25.6.0
- 参数：完整方块 y、剔除相邻面 y、居中 y、单位化 n

| 存档 | 区块 (x, z) | 半径 | 扫描 | chunk | tile | 普通方块 | 合并面数 | 合并顶点 | OBJ 体积 | 耗时 |
|---|---|---|---|---|---|---|---|---|---|---|
| `test_region` | 0, 0 | 1 | 3×3 | 9 | 236 | 1394 | 4881 | 12832 | 0.3 MB | 0.1 s |
| `test_region_medim` | -136, 49 | 5 | 11×11 | 121 | 55561 | 124192 | 388744 | 699708 | 34.3 MB | 9.1 s |
| `test_region_large` | -7, -26 | 5 | 11×11 | 121 | 324427 | 1945017 | 2036139 | 2986790 | 177.5 MB | 50.3 s |

- `test_region`：总耗时 0.1 s = 读取与建网格 0.1 + 普通方块网格 0.1 + 写出文件 0.0；普通方块面 3472（剔除 4892，被拒 0）；材质 6 / 贴图 6
- `test_region_medim`：总耗时 9.1 s = 读取与建网格 6.3 + 普通方块网格 1.4 + 写出文件 1.4；普通方块面 65223（剔除 679929，被拒 0）；材质 77 / 贴图 77
- `test_region_large`：总耗时 50.3 s = 读取与建网格 40.5 + 普通方块网格 2.6 + 写出文件 7.1；普通方块面 104743（剔除 11565359，被拒 0）；材质 120 / 贴图 120
## 2026-09-12 04:07

- 时间：2026-09-12 04:07
- 提交：ab47d8c（工作区有未提交改动）
- 构建：Debug（/Users/external_elliott/Development/minecraft-littletiles-reader/cmake-build-debug/LittleTilesReader）
- 机器：Apple M3 / Darwin 25.6.0
- 参数：完整方块 y、剔除相邻面 y、居中 y、单位化 n

| 存档 | 区块 (x, z) | 半径 | 扫描 | chunk | tile | 普通方块 | 合并面数 | 合并顶点 | OBJ 体积 | 耗时 |
|---|---|---|---|---|---|---|---|---|---|---|
| `test_region` | 0, 0 | 1 | 3×3 | 9 | 236 | 1394 | 4881 | 12832 | 0.3 MB | 0.1 s |
| `test_region_medim` | -136, 49 | 5 | 11×11 | 121 | 55561 | 124192 | 388744 | 699708 | 34.3 MB | 18.7 s |
| `test_region_large` | -7, -26 | 5 | 11×11 | 121 | 324427 | 1945017 | 2036139 | 2986790 | 177.5 MB | 57.2 s |

- `test_region`：总耗时 0.1 s = 读取与建网格 0.1 + 普通方块网格 0.1 + 写出文件 0.0；普通方块面 3472（剔除 4892，被拒 0）；材质 6 / 贴图 6；素材根 /Users/external_elliott/Development/minecraft-littletiles-reader/assets/pack_v14
- `test_region_medim`：总耗时 18.7 s = 读取与建网格 6.0 + 普通方块网格 1.3 + 写出文件 11.5；普通方块面 65223（剔除 679929，被拒 0）；材质 77 / 贴图 77；素材根 /Users/external_elliott/Development/minecraft-littletiles-reader/assets/pack_v14
- `test_region_large`：总耗时 57.2 s = 读取与建网格 38.3 + 普通方块网格 2.5 + 写出文件 16.4；普通方块面 104743（剔除 11565359，被拒 0）；材质 120 / 贴图 120；素材根 /Users/external_elliott/Development/minecraft-littletiles-reader/assets/pack_v14

## 2026-09-22 0.3.0-beta 发布基线（Release 构建，Windows / Linux）

- 时间：2026-09-22
- 提交：5bac1eb（工作区有 0.3.0 未提交改动）
- 构建：Release（`cmake-build-release/LittleTilesReader`），`-DGALIB_VERSION_CHANNEL=beta`，`--version` 输出 `0.3.0-beta (test build)`
- 机器：Windows x64（MSVC /MT，静态 CRT，vcpkg 依赖）与 OpenCloudOS 9.6 x86_64（GCC 12.3，dnf 依赖）
- 参数：普通方块 y、剔除相邻面 y、居中 y、单位化 n

这一节是**发布包**的冒烟基线（`docs/build-guide.md` §7），不是上面 Debug 周期的扫描基线；
用例与数据目录见 `tools/`（不随本库分发）。

| 用例 | Windows x64 耗时 | Linux x86_64 耗时（峰值内存） | 结果（两平台逐项一致） |
|---|---|---|---|
| 存档导出，`base` chunk 0,0 半径 1 | 0.230 s | 0.048 s（15.6 MB） | 9 区块 / 缺 0 / 236 tile / 12975 顶点 / 4940 面 / 6 材质 / 贴图 6 / 缺贴图 0 |
| SNBT 导出，`CoronaSign` | 0.148 s | 0.029 s（8.1 MB） | 1114 meshes |
| SNBT 导出，`SHB_05` 大结构 | 1.144 s | 0.338 s（54.5 MB） | 18044 meshes |
| SNBT 互转，`SHB_05` 1.12.2 → 1.20（0.3.0 新增） | 0.388 s | 0.044 s（38.6 MB） | 18044 box / 142 tile / 47 level / 改名 122 / 未映射方块名 11 / 未映射 tile 20 |

**0.2.0 → 0.3.0 没有回退**：三个老用例的顶点数与面数（12975 / 4940 / 1114 / 18044）与
0.2.0 的实测值**完全相同**，尽管 0.3.0 改了 `BlockTileEntities.cpp` / `LtStructure.cpp` /
`ChunkBlocks.cpp` / `SnbtParser.cpp` 四块核心代码。Windows 与 Linux 的 region OBJ 文件**逐字节相同**。

### 0.3.0 新增能力的回归（`docs/build-guide.md` §7.3）

| 新功能 | 做法 | 结果 |
|---|---|---|
| 1.18+ / 1.20 存档布局 | 同一座建筑分别放在 1.12.2 存档 `base` 与 1.20.1 存档 `新的世界` 的 chunk `-1, 0`，都只导 LittleTiles（`plain_blocks=false`）、都用 1.12 命名的素材包 | **逐字节一致**：两边都是 164 tile / 1355 顶点 / 999 面 / 3 材质 / 缺贴图 0；排序后的顶点列表 SHA256 同为 `0da0751c…86cb`，材质集合与包围盒（X -5.25..5.25，Y -0.50..0.50，Z -8.00..8.00）相同 |
| SNBT → SNBT 互转 | `--convert 1.20` 转 `SHB_05`，再用同一个二进制读回转出的 1.20 文件 | 读回得到 18044 meshes，与原始 1.12.2 文件导出的 18044 一致；Windows 与 Linux 转出的文件**逐字节相同**（sha256 `ef2ae637…32fe`，759890 B）。反向 `--convert 1.12.2` 得到 18044 box / 142 tile / 47 level，与正向计数相同 |
| 跨版本贴图兜底 | 上面两个 1.20 用例都用旧命名的素材包 `pack_v14` | 材质全部解析成功（缺贴图 0），面数与材质集合与 1.12.2 一致 |

> ⚠️ **1.20.1 存档是真实世界，含地形**：把半径放大到 1（9 区块）后两边不可直接比较 ——
> 1.12.2 的 `base` 是测试用空世界（只有结构，220 tile），1.20.1 的世界里同一个区块周围
> 有地形和别的建筑（164 tile / 6441 面，多出来的全是普通方块）。比较结构本身时
> 必须 `plain_blocks=false` 且只取 chunk `-1, 0`。
>
> ⚠️ **macOS 未参与本轮基线重建**（本机不可构建，见 `docs/build-guide.md` §5）：
> 0.3.0 的第三平台数据待 Mac 侧构建后补进本节。
