# 已核实的问题与基线

每条尽量附可复现证据。修改核心代码前请先读本文件。

## 1. 已修复

| 问题 | 证据 | 修复 |
|---|---|---|
| **缺失区块二次访问崩溃**：region 文件存在、但该 chunk 槽为空时，第 1 次调用抛异常，第 2 次调用 SIGSEGV | 探针连续调用两次，修复前 `exit=139` | `5a1cdf2`：读取失败路径统一清缓存后重抛（`Anvil.cpp`） |
| 错误语义反了：空槽抛 `mc_chunk_exists` / "The chunk is exists" | 同上 | `5a1cdf2`：新增 `mc_chunk_not_exist = 7` 与正确文案（`Anvil.cpp`） |
| 区块长度还原时首字节 `<< 32`（int 移位越界，UB） | 编译告警 `-Wshift-count-overflow` | `5a1cdf2`：改为 `<< 24`（`Anvil.cpp`）。注意：现有测试数据长度首字节为 0，新旧结果相同，属**隐患修复** |
| `CacheManagerBase` const 版 `GetCachePointer` 用 `.y` 索引 `Coordinate2D` | `Anvil.h`（该重载一旦实例化即编译失败） | `5a1cdf2`：改为 `.z` |
| macOS 上 vcpkg 依赖未被使用，`find_package(Boost)` 落到已移除的 `FindBoost` | 配置报错 | `5a1cdf2`：统一依赖来源优先级 |
| libnbt++ 每次配置拉 HEAD，构建不可复现 | `3rdparty/CMakeLists.txt` | `fc7dde0`：钉 commit `687e4303…` |
| `ReadChunk` 的 `p_boxes_count` 输出参数从未被赋值 | `ChunkTileEntities.cpp` | `ecd30e4`：在共用的 `ReadTileEntities` 中填充 |
| **同种方块的不同染色被整条丢弃**：`ReadBlockTileNbt` 用 block id 作 map 键，遇到重复 block id 直接 `continue` | 实测 chunk(-136,49)：NBT 里共 8037 个 box，读取器只报告 4456（**丢 45%**）；全量数据有 526 个 tile 条目因此被丢 | 材质键改为 `(block id, color)`（新增 `TileMaterial`），并把染色保存到 `TileEntity` / `LtSurfaceMesh` |
| **完整方块只导出一两个面**：分组网格里跨方块焊接顶点，`Euler::add_face` 静默拒收后续方块的面 | 最小复现 `.ai/tools/lt_cube_face_probe.cpp`：16 个方块 96 个面只接受 24 个 | 顶点只在单个方块内焊接（`CgalWorldBlocks.cpp`），并统计 `add_face` 返回值。见 2.7 |
| **LT 结构下面的完整方块丢了面**：邻居剔除把 LittleTiles 宿主方块当成实心方块 | 探针 `.ai/tools/lt_host_probe.cpp`：LT 宿主位置的 block id 是 257（非空气），而那里可能只摆了一个花盆 | 改为按面判断：只有 tile 把该面按 grid 铺满时才剔除（`covered_face_mask()`）。见 2.8 |

## 2. 未修复

### 2.1 服务器化阻塞项

1. **`SetRegionFolder()` 不清缓存 → 跨来源数据串号**
   `Anvil.cpp` 只改路径，缓存 key（`RegionCoordinate`）不含数据源标识。
   复用同一 `AnvilReader` 处理不同上传会返回上一次的数据 —— 这是**数据隔离**问题。
2. **缓存无上限**：`mca_cache_` 每 region 存整份 `.mca`；`chunk_cache_` 每 region 一个 32×32 槽、
   每槽一份已解析 NBT 树，只在显式 `Clear()` 时释放。
3. **无任何线程安全**：见 `architecture.md` 第 5 节。
4. **库边界没有错误转换**：`main()` 无 try/catch，异常直接终止进程。

### 2.2 已确认缺陷

5. **`LittleTilesErrorCode` 越界读**：`lt_unknow_angle = 0`（`LittleTilesException.h`），
   但查表为 `STD_ERROR_CODE[code - 1]`（`LittleTilesException.cpp`）→ 索引 -1；
   且基类把 `error_code == 0` 视为"无异常"，该异常文案会变成 "No exception."。
6. **`ChunkData::p_chunk_level` 未初始化**（`Anvil.h`）且从未被赋值；目前无人读取，属埋雷。
7. **`ReadMcaFile` 对空文件 UB**：`resize(st_size)` 后取 `&*begin()`（`Anvil.cpp`），
   且 `GetFileStat` 返回值未检查（`Anvil.cpp`）。
8. **`GALIB_DEBUG` 被无条件 `#define`**（`GalibNamespaceDef.h`）：调试 `printf` 常驻；
   并把 `catch(...)` 换成更窄的 `catch(std::exception&)`，非 std 异常会穿透。
   进度输出已可用运行时开关关掉（`galib::SetProgressEnabled()`，CLI 的"是否打印进度提示与耗时 /
   Print progress and timing?"选项），但编译期仍无法整体裁掉这些打印；服务器化时建议把它变成
   CMake 选项或换成可注入的 logger。
9. ~~**CLI 的"几何中心"选项永远无效**~~ —— 已修复：`scanf("%c")` 会读到前一个 `%d` 残留的换行符，
   已改为 `scanf(" %c", &choice)`；同时新增"归一化到单位尺寸"选项（见 2.6）。
   （修复前实测输入 `y` 与 `n` 产出的 OBJ 逐字节相同。）
10. **`#if WIN32` 永不成立**（`main.cpp`）：MSVC 定义的是 `_WIN32`。
11. **OFF 导出是空实现**：`writeMeshToOff` 函数体只有一句局部变量声明
    （`CgalLittletilesBuilder.cpp`），调用它的 `WriteToOff`（`:278`）因而什么也不产出。
12. ~~**CMake 声明 C++14、代码使用 C++17**~~ —— 已修复：`CMAKE_CXX_STANDARD` 改为 17
    （代码使用 if-init、嵌套命名空间定义与 `std::filesystem`，CGAL 6.x 也要求 C++17）。
    此前在 clang 下靠扩展特性勉强编过，MSVC `/std:c++14` 会直接失败。

### 2.5 导出路径与输出目录

17. **输出失败时只打印文件名、不建目录**（已修复）：`MergeAndWriteToObj` 直接 `ofstream`
    打开 `../out_file/xxx.obj`，而 `ofstream` **不会创建目录**，目录不存在时只会打印
    「无法打开文件」并且什么都不产出（参考实现的 Java 版有 `folder.mkdirs()`）。
    现在会先 `create_directories` 建出父目录，并打印**规范化后的绝对路径**，
    便于定位产物；`/out_file/` 也已加入 `.gitignore`。

    输出路径仍由 `main.cpp,24` 的宏 `OUT_OBJ_FILE_NAME = "../out_file/marge_obj_from_chunk_"`
    决定，是**相对运行时工作目录**的：在构建目录下运行会写到 `<repo>/out_file/`，
    在仓库根目录下运行会写到 `<repo>/../out_file/`。

    贴图不再与 OBJ 平铺在同一层，而是写进 OBJ 旁边的 `<obj 名>_textures/` 子目录
    （`map_Kd` 指向该子目录，MTL 仍在 OBJ 同级——Blender 按 `mtllib` 相对 OBJ 找 MTL）。
    这样一次导出只多一个目录，不会再往 out_file 里撒几十上百张 PNG。

### 2.6 输出归一化与数值精度

18. **OBJ 顶点精度不足**（已修复）：写出用 `std::ostream` 默认精度（6 位有效数字），
    而顶点是 Minecraft 世界坐标（例如 x ≈ -2164），6 位有效数字意味着**量化步长约 0.01 方块**
    （grid=64 时相当于 0.64 个网格单位），几何被静默改变。实测同一 chunk 的体积
    因这项舍入偏差 0.27%。现在写出时设置 `setprecision(9)`，量化步长降到 1e-6 方块量级。

19. **新增输出归一化**（`MergeAndWriteToObj` 的第 4 个参数）：

    | 模式 | 参数 | 效果 |
    |---|---|---|
    | 原始坐标 | `geom_center=false` | 保持 Minecraft 世界坐标（离原点可达数千单位） |
    | 居中 | `geom_center=true`（默认） | 包围盒中心平移到原点，体积不变 |
    | 居中 + 单位缩放 | 再加 `normalize_scale=true` | 再等比缩放到最长边 = 1（会丢失"1 单位 = 1 方块"比例） |

    CLI 会依次询问这两项，提示语形如
    `Move the model center to the origin? (y/n) [y]:` 与
    `Also scale the longest edge to 1 unit (changes the real size)? (y/n) [n]:`，
    **直接回车使用方括号里的推荐值**（居中 = y，缩放 = n）。
    未居中的模型导入 Blender 后离原点很远（例如 x ≈ -2164、z ≈ 797），
    而视口默认 Clip End = 100，会看起来"什么都没导入"。

### 2.3 导出质量（与 UV / 材质相关）

13. **顶点不焊接**：合并时对每张 mesh 的每个顶点都 `add_vertex`，且每个 mesh 后清空映射
    （`CgalLittletilesBuilder.cpp`）→ 顶点数 ≈ 8 × tile 数，面之间不共享顶点。
14. **per-tile 颜色与 meta 全丢**：`block_id_` 已存于 `LtSurfaceMesh`，但 OBJ 只写 `v`/`f`
    （无 `vt`/`vn`/`usemtl`/`mtllib`/group）。
15. **没有 UV、法线、材质**：`UVData` 类型存在（`CgalTypeDef.h`），但
    `CalculateFaceUv` 直接 `return {}`（`CgalTypeDef.cpp`）。
16. **内部面不剔除**：相邻/相交 tile 之间会留下看不到的面。

### 2.4 裁剪路径产生大量碎三角形（"杂乱辅助线"）—— 已修复

> **修复状态**：已用"半空间裁剪"替换 CGAL 布尔求交，并把平面面片输出为 n 边形。
> 见本节末尾的「修复方案与实测效果」。以下为修复前的问题记录。

越界 tile 的裁剪使用 `corefine_and_compute_intersection`
（`CgalLittletilesBuilder.cpp`），实测副作用如下（`test_region_medim` chunk (-136,49)，4456 个 tile）：

| 指标 | 数值 |
|---|---|
| 未被裁剪的 tile（8 顶点 / 12 面） | 4034 |
| 被裁剪的 tile（面数 > 12） | 409，单 tile 最多 **42 面**（是干净 tile 的 3.5 倍） |
| 裁剪额外产生的顶点 | 2010（平均每个被裁剪 tile +4.9） |
| 裁剪额外产生的面 | 4020（平均 +9.8） |

原因分析：

1. `corefine_and_compute_intersection` 是**两个三角网格之间的布尔运算**，会沿交线把两个网格都细分；
   而这里的裁剪盒正好是 **tile 自己的盒子**（`CreateMeshFromTileEntity(..., false)`），
   于是被裁剪网格与裁剪盒必然存在**共面重合的面** —— 共面布尔正是 CGAL 最容易产生
   碎三角形、细长片与 T 形接点的场景。
2. 合并导出时**不做顶点焊接**（`CgalLittletilesBuilder.cpp`）：
   实测 37658 个顶点去重后只有 24243 个，**35.6% 是重复顶点**。
3. 输出全部是三角形，被切的面变成三角扇，线框视图里会显示大量内部边。

关于裁剪体的语义（需要确认）：V3 技术文档第 272 段写"该图形会跟**偏移前的几何结构**做交集运算"，
但同一段又说"蓝色部分为**方块边界**"；README 的 MATLAB 示例里蓝框是
`LtBlock(0,0,0,grid,grid,grid)`（整方块边界）。当前 C++ 实现按"偏移前的盒子"裁剪。
按全量数据统计，两者的差异是：

```
带偏移数据的 box 总数                     : 28911
超出自身盒子（当前会被裁剪）               : 11932 (41.3%)
超出方块边界 [0, grid]                     : 7976 (27.6%)
仅超出自身盒子、仍在方块内（当前"白切"）    : 3956
```

可行替代方案（尚未实施）：

- **不做切割**：直接输出偏移后六面体的 6 个面（参考实现即如此），完全没有切割边。
- **半空间裁剪替代布尔**：tile 是凸六面体、裁剪体是 AABB，交集仍是凸多面体；
  用 Sutherland–Hodgman 对每个面裁剪 6 个半空间，新顶点只会出现在切割面上，
  且可在整数 grid 单位下精确计算；输出 n 边形而非三角扇。
- **若保留 CGAL**：改用 `CGAL::Polygon_mesh_processing::clip(mesh, plane)` 连续 6 次平面裁剪，
  避免两个网格的共面布尔。

#### 修复方案与实测效果

实现：新增 `cgal_support::ClipTileEntityToBox()`
（声明 `CgalLtSupport.h`，实现 `CgalLtSupport.cpp`），`addTilesFromBlockTilesEntities`
不再调用 `corefine_and_compute_intersection`；同时 `CreateMeshFromTileEntity` 对**平面**四边形
输出单个 n 边形，只有**扭曲（非平面）**四边形才按 `Flipped` 规则拆成两个三角形。

算法要点（实现时踩过的坑都写在这里）：

1. 保留半空间 `n·p >= d` 一侧时，**切面的外法线是 −n**（不是 +n）。方向反了会让切面与相邻面
   在共享边上同向，CGAL 会判定为非法面并拒收（表现为网格不闭合）。
2. Sutherland–Hodgman 在"顶点恰好落在裁剪平面上"时会产生**重复点**，必须去重，
   否则 CGAL 以非法多边形为由拒收整面。
3. 角点独立偏移可能让两个角点**落到同一位置**（退化四边形），同样会因重复顶点被拒收；
   需要先按坐标合并退化点，再决定输出 n 边形还是三角形。
4. 扭曲四边形必须**先拆成三角形再裁剪**，否则裁剪结果与原布尔运算不一致
   （实测 chunk (0,1) 体积差 10.6%，修正后为 1.2e-6）。

实测效果（对比修改前的同版本二进制）：

| 指标 | 修改前 | 修改后 |
|---|---|---|
| 单个被裁剪 tile 最大面数 | 42 | **7** |
| chunk (-136,49) 总面数 | 57492 | **26134**（2.2 倍减少） |
| chunk (-136,49) 顶点数 | 37658 | 35432 |
| chunk (-136,49) OBJ 体积 | 1973 KB | **1473 KB**（−25%） |
| chunk (0,1) OBJ 体积 | 8.0 KB | **3.8 KB**（−53%） |
| 输出面元 | 全部三角形 | 四边形为主（含少量三角形/五边形） |

等价性验证（`lt_clip_compare`，逐 tile 比较新旧两条路径）：

```
10 个 chunk、共 878 个越界 tile：
  体积不一致(相对 > 1e-6) : 0     最大相对差 ~4e-9（浮点噪声）
  包围盒不一致            : 0
  网格闭合性              : 新结果无边界边（封闭）
```

注意：裁剪体仍是「偏移前的盒子」（与原实现一致）。若要改为「方块边界 `[0, grid]`」，
只需把 `ClipTileEntityToBox` 里的 `box_min/box_max` 换成 `(0,0,0)`–`(grid,grid,grid)`，
但这会改变语义，需先确认（见上文 2.4 中关于裁剪体的疑问）。

### 2.7 完整方块（普通方块）只剩一两个面 —— 已修复

**症状**：开启"完整方块导出"后，Blender 里每个普通方块只看到一个（或个别）面，
不是闭合立方体；同一批数据里 LittleTiles 的 tile 却是正常的。

**根因不在导出，也不在 Blender**，而在 `CgalWorldBlocks.cpp` 的顶点焊接策略：

1. CGAL 的 `Euler::add_face`（`Surface_mesh::add_face(range)` 最终走这条）
   有两条硬性前置条件：
   - 新面的**每个顶点必须是孤立顶点或边界顶点**；
   - 新面的**每条边必须不存在，或存在但为边界半边**（即只有一面）。
2. 原实现把「同一 `(id, meta)` 分组」内所有方块的顶点**按世界坐标全局焊接**成一张表。
   于是方块 A 的 6 个面写完、A 闭合后，A 的 8 个顶点全都变成了**内部顶点**；
   紧接着写与 A 相邻的方块 B 时，只要 B 的面用到 A 的顶点或边，就会被拒绝。
3. `add_face` 拒绝时**不抛异常、也不返回 false**，只返回 `null_face`。
   原实现没检查返回值，于是面被静默丢弃 —— 这就是"完整方块只剩单面"的由来。

**证据**：最小复现 `.ai/tools/lt_cube_face_probe.cpp`（4×1×4 同种方块，96 个面）：

```
A 全局焊接（修复前）：期望面 96，add_face 接受 24，被拒绝 72
B 方块内焊接（修复后）：期望面 96，add_face 接受 96，被拒绝 0
```

**修复**：立方体的 8 个角只在**单个方块内**共享（`std::array<Vertex_index, 8>`
按 0/1 偏移做下标），不跨方块焊接；同时统计 `add_face` 的返回值，
在 `[worldblocks]` 调试输出里打印「被 CGAL 拒绝 N 个」，避免再次静默丢面。
每个方块因此是一张独立闭合曲面 —— 这正是 OBJ 导出想要的形式，
也顺带摆脱了原先按世界坐标打包 64 位键带来的越界隐患。

**实测验证**（`test_region_large` chunk (-7,-26)）：

| 场景 | 结果 |
|---|---|
| 不剔除相邻面 | 16510 个方块 × 6 = **99060 面，被拒绝 0**，分组网格实际保存 99060 |
| 开启邻居剔除 | 99060 → **4573 面**（剔除 94487），被拒绝 0 |
| 11×11 区域（-7,-26 到 -2,-21，121 个 chunk） | 1,945,017 个方块 → 103257 面，被拒绝 0；整份 OBJ 2,034,653 面 / 2,982,366 顶点 / 192 MB，耗时约 43 s |

按材质逐面统计法线方向也能对上：不剔除时 `blocks_dirt` 的 15630 个方块
在 ±x / ±z / +y 各 15630 个面、−y 方向 15825 个面，即每个方块都是完整六面体。

### 2.8 LittleTiles 结构下面的完整方块被削掉顶面 —— 已修复

**症状**：`test_region` chunk (0,0) 里有一个完整方块，上面放了一个用 tile 拼的花盆；
导出后那个完整方块的**顶面不见了**，地面像开了个洞。

**根因**：邻居剔除把"LittleTiles 宿主方块"当成了实心方块。
探针 `.ai/tools/lt_host_probe.cpp` 实测该位置的 `(block_id, meta) = (257, 0)`：
LittleTiles 用自己的方块占了这个世界位置，所以 `is_air()` 为假，
于是下面那个完整方块的 `+y` 面被判定为"看不见"而剔除。
但 257 号方块渲染的是 tile（这里只有一个小花盆），并不填满整个方块的立方体。

**修复**：`BuildWorldBlockMeshes` 的剔除条件改为"只有另一个**普通**实心方块才挡得住"：

```cpp
if (!neighbor.is_air() && !neighbor.little_tiles_host) { /* 剔除 */ }
```

**中间版本与最终版本**：先改成"LT 宿主一律不挡面"（宁可有面也不要丢面），
但那样会让真正被 tile 完全填满的方块也留下多余的面（还与 tile 表面共面，可能 z-fighting）。
最终实现是按面判断覆盖：

`BlockTileEntities::covered_face_mask()` 把该位置**没有角度偏移**的 tile 盒子投影到方块 6 个面上，
按 grid 栅格化（grid×grid 个格子），全部格子被铺满时该面才算"被挡住"；
带角度偏移的 tile 是斜面/异形，盒子不代表实际形状，保守当作没覆盖。
掩码按 `TileFaceID` 位序（EAST/WEST/SOUTH/NORTH/UP/DOWN）存进 `ChunkBlocks::State`，
剔除时取邻居的**反向面**（`face_index ^ 1`）。

**实测影响**（`test_region_large` 11×11 的普通方块面）：

| 版本 | 普通方块面 | 说明 |
|---|---|---|
| 最原始（LT 宿主一律挡面） | 103257 | 1486 个面被错误剔除（= 大模型里的"洞"） |
| 中间版（LT 宿主一律不挡面） | 118783 | 多出 14040 个多余面（tile 确实铺满的） |
| **当前（按面覆盖判定）** | **104743** | 只保留真正没被铺满的面 |

小样本 `test_region` (0,0) 半径 1 共 148 个 LT 宿主，但**没有一个面被铺满**（花盆只占一小块），
所以面数仍是 4881 —— 那个完整方块的顶面回来了。

覆盖判定的真实样本（探针 `.ai/tools/lt_coverage_probe.cpp`，chunk (-7,-26)）：

```
宿主 (-97, 63, -401) grid=128 覆盖掩码=0x3f：六个面全铺满
   tile[1] box=(127,0,0)-(128,128,128) 偏移=0
   tile[2] box=(95,0,0)-(127,128,128) 偏移=0
   tile[3] box=(0,0,0)-(95,128,128) 偏移=0
宿主 (-107, 64, -413) grid=2 覆盖掩码=0x22：WEST(-x) DOWN(-y)
   tile[1] box=(1,0,0)-(2,1,2) 偏移=0
   tile[3] box=(0,0,0)-(1,2,2) 偏移=0
掩码分布：0x00 105 个，0x22 16 个，0x3f 2 个（共 123 个宿主）
```

前者的三个盒子正好铺满 128³ 整个方块（是拿 tile 砌的实心方块），六面都该挡；
后者只有西面和底面被铺满（一个贴墙的角落），另外四个面保留。

## 3. 输出非确定性（重要）

同一输入、同一个二进制，**OBJ 输出有两种形式，每次进程运行随机二选一**：

```
10 次运行 chunk(0,1) → 2 个不同 md5（7 : 3）
顶点列表：10 次完全相同
面集合（循环旋转归一化 + 排序后比较）：10 次完全相同
```

即几何与绕向始终正确，变化的是**面的输出顺序**与**每个面的起始顶点**。
推断来源在 CGAL 裁剪路径内部（受堆布局影响的容器遍历顺序），非本项目代码所为。

影响：**同一输入拿不到逐字节相同的输出**，将来做"同输入 → 同哈希"的结果缓存会踩坑。
可能解法：导出前规范化（面按最小顶点索引轮转 + 排序）。

## 4. 测试数据与基线

| 目录 | region 数 | 说明 |
|---|---|---|
| `test_region/` | 4 | 小样本（推荐 chunk **(0,0)**、半径 1） |
| `test_region_medim/` | 11 | 中等样本（推荐 chunk **(-136,49)**、半径 5，即 11×11，只用到 `r.-5.1.mca`） |
| `test_region_large/` | 16 | 大样本（推荐 chunk **(-7,-26)**、半径 5，即 11×11） |

基线数字（**Debug** 构建；由 `python3 tools/benchmark.py` 复核，逐次记录见 `docs/benchmark.md`）：

| 输入 | 结果 |
|---|---|
| `test_region` chunk (0,0) 半径 1 | 236 tiles + 1394 个普通方块 → 4881 面 / 12832 顶点 → 0.3 MB OBJ，约 0.1 s |
| `test_region_medim` chunk (-136,49) 半径 5 | 55561 tiles + 124192 个普通方块 → 388744 面 / 699708 顶点 → 34 MB OBJ，约 9 s |
| `test_region_large` 11×11（-7,-26 → -2,-21） | 324427 tiles + 1945017 个普通方块（104743 个面）→ 2,036,139 面 / 2,986,790 顶点 → 178 MB OBJ，约 50 s（含覆盖判定开销） |

> 旧记录里的 "chunk (-136,49) 4456 tiles / 37658 顶点 / 57492 面" 是**染色去重修复与
> 半空间裁剪之前**的数字：前者丢了 45% 的染色 tile，后者把面全拆成三角形。
> 以上表为准。

> 37658 > 8×4456 = 35648：多出的部分是偏移越界的 tile 经
> `corefine_and_compute_intersection` 裁剪后产生的额外顶点。

## 5. README 与代码不一致之处

README 现分为两份内容相同的文档：[`README.md`](../README.md)（English）与
[`README.zh-CN.md`](../README.zh-CN.md)（简体中文），改动其中之一时请同步另一份。

下表是**历史记录**：这些不一致已在重写 README 时修正（"当前状态"改为实际实现、
依赖表与语言版本更新、已知限制单列一节），保留在此是为了说明当初的偏差。

| # | 原先 README 的说法 | 代码实际 |
|---|---|---|
| 1 | "V2 已实现：每个 tile 导出为 OFF，再用 Python 把 OFF 转 OBJ" | C++ 的 OFF 写出是空函数；现在直接合并写 OBJ。该描述是 V1 流程 —— **已修正** |
| 2 | 示例 SNBT 使用 `bBox` / `tile` / `min` / `size` / `count` 键 | 解析器读的是 `content.tiles[].block` + `boxes`/`box`，**没有**这些键。该 SNBT 喂给 Reader 会失败，它只服务于 MATLAB 辅助脚本 —— README 现已把它放在"Matlab 支持"一节下，不再暗示它是 Reader 的输入 |
| 3 | 依赖表写 CGAL 5.6；语言写 C++14 | 实测 CGAL 6.2.1；代码实际需要 C++17 —— **已修正** |
| 4 | 处理流程止于"OBJ 构建器" | 未提及 1.12 专有的 `Level` 硬编码、无 `DataVersion` 校验、只支持 zlib 压缩类型 —— README 新增"已知限制"一节说明 |
