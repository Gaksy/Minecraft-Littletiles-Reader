# 导出基线与实测数据

由 `python3 tools/benchmark.py --write docs/benchmark.md` 生成，每次运行追加一节。
各存档的推荐参数写在 `tools/benchmark.py` 的 `CASES` 里，与 README「测试存档」表一致。

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
