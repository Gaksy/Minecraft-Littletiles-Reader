# 已核实的问题与基线

每条尽量附可复现证据。修改核心代码前请先读本文件。

## 1. 已修复

| 问题 | 证据 | 修复 |
|---|---|---|
| **缺失区块二次访问崩溃**：region 文件存在、但该 chunk 槽为空时，第 1 次调用抛异常，第 2 次调用 SIGSEGV | 探针连续调用两次，修复前 `exit=139` | `5a1cdf2`：读取失败路径统一清缓存后重抛（`Anvil.cpp:133-170`） |
| 错误语义反了：空槽抛 `mc_chunk_exists` / "The chunk is exists" | 同上 | `5a1cdf2`：新增 `mc_chunk_not_exist = 7` 与正确文案（`Anvil.cpp:226`） |
| 区块长度还原时首字节 `<< 32`（int 移位越界，UB） | 编译告警 `-Wshift-count-overflow` | `5a1cdf2`：改为 `<< 24`（`Anvil.cpp:241`）。注意：现有测试数据长度首字节为 0，新旧结果相同，属**隐患修复** |
| `CacheManagerBase` const 版 `GetCachePointer` 用 `.y` 索引 `Coordinate2D` | `Anvil.h:75`（该重载一旦实例化即编译失败） | `5a1cdf2`：改为 `.z` |
| macOS 上 vcpkg 依赖未被使用，`find_package(Boost)` 落到已移除的 `FindBoost` | 配置报错 | `5a1cdf2`：统一依赖来源优先级 |
| libnbt++ 每次配置拉 HEAD，构建不可复现 | `3rdparty/CMakeLists.txt` | `fc7dde0`：钉 commit `687e4303…` |
| `readChunk` 的 `p_boxes_count` 输出参数从未被赋值 | `ChunkTileEntities.cpp` | `ecd30e4`：在共用的 `readTileEntities_` 中填充 |

## 2. 未修复

### 2.1 服务器化阻塞项

1. **`setRegionFolder()` 不清缓存 → 跨来源数据串号**
   `Anvil.cpp:72` 只改路径，缓存 key（`RegionCoordinate`）不含数据源标识。
   复用同一 `AnvilReader` 处理不同上传会返回上一次的数据 —— 这是**数据隔离**问题。
2. **缓存无上限**：`mca_cache_` 每 region 存整份 `.mca`；`chunk_cache_` 每 region 一个 32×32 槽、
   每槽一份已解析 NBT 树，只在显式 `clear()` 时释放。
3. **无任何线程安全**：见 `architecture.md` 第 5 节。
4. **库边界没有错误转换**：`main()` 无 try/catch，异常直接终止进程。

### 2.2 已确认缺陷

5. **`LittleTilesErrorCode` 越界读**：`lt_unknow_angle = 0`（`LittleTilesException.h:28`），
   但查表为 `STD_ERROR_CODE[code - 1]`（`LittleTilesException.cpp:42`）→ 索引 -1；
   且基类把 `error_code == 0` 视为"无异常"，该异常文案会变成 "No exception."。
6. **`ChunkData::p_chunk_level` 未初始化**（`Anvil.h:190`）且从未被赋值；目前无人读取，属埋雷。
7. **`readMcaFile_` 对空文件 UB**：`resize(st_size)` 后取 `&*begin()`（`Anvil.cpp:198`），
   且 `GetFileStat` 返回值未检查（`Anvil.cpp:195`）。
8. **`GALIB_DEBUG` 被无条件 `#define`**（`GalibNamespaceDef.h:27`）：调试 `printf` 常驻；
   并把 `catch(...)` 换成更窄的 `catch(std::exception&)`，非 std 异常会穿透。
9. **CLI 的"几何中心"选项永远无效**：`main.cpp:71` 的 `scanf("%c")` 读到前一个 `%d` 残留的换行。
   实测输入 `y` 与 `n` 产出的 OBJ **逐字节相同**。
10. **`#if WIN32` 永不成立**（`main.cpp:19`）：MSVC 定义的是 `_WIN32`。
11. **OFF 导出是空实现**：`writeMeshToOff` 函数体只有一句局部变量声明
    （`CgalLittletilesBuilder.cpp:273`），调用它的 `writeToOff`（`:278`）因而什么也不产出。
12. **CMake 声明 C++14、代码使用 C++17**（`CMakeLists.txt:35` vs `BlockTileEntities.cpp:102`）。

### 2.3 导出质量（与 UV / 材质相关）

13. **顶点不焊接**：合并时对每张 mesh 的每个顶点都 `add_vertex`，且每个 mesh 后清空映射
    （`CgalLittletilesBuilder.cpp:157-204`）→ 顶点数 ≈ 8 × tile 数，面之间不共享顶点。
14. **per-tile 颜色与 meta 全丢**：`block_id_` 已存于 `LtSurfaceMesh`，但 OBJ 只写 `v`/`f`
    （无 `vt`/`vn`/`usemtl`/`mtllib`/group）。
15. **没有 UV、法线、材质**：`UVData` 类型存在（`CgalTypeDef.h:43`），但
    `calculateFaceUV` 直接 `return {}`（`CgalTypeDef.cpp:76`）。
16. **内部面不剔除**：相邻/相交 tile 之间会留下看不到的面。

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
| `test_region/` | 4 | 小样本（chunk (0,1) 含 22 个 tile） |
| `test_region_medim/` | 11 | 中等样本（chunk (-136,49) 含 4456 个 tile） |

基线数字（**Debug** 构建）：

| 输入 | 结果 |
|---|---|
| `test_region` chunk (0,1) | 22 tiles → 243 顶点 / 398 面 → 8 KB OBJ |
| `test_region_medim` chunk (-136,49) | 4456 tiles → 37658 顶点 / 57492 面 → 2.0 MB OBJ，约 2.5 s |

> 37658 > 8×4456 = 35648：多出的部分是偏移越界的 tile 经
> `corefine_and_compute_intersection` 裁剪后产生的额外顶点。

## 5. README 与代码不一致之处

| # | README 的说法 | 代码实际 |
|---|---|---|
| 1 | "V2 已实现：每个 tile 导出为 OFF，再用 Python 把 OFF 转 OBJ" | C++ 的 OFF 写出是空函数；现在直接合并写 OBJ。该描述是 V1 流程 |
| 2 | 示例 SNBT 使用 `bBox` / `tile` / `min` / `size` / `count` 键 | 解析器读的是 `content.tiles[].block` + `boxes`/`box`，**没有**这些键。该 SNBT 喂给 Reader 会失败（它只服务于 MATLAB 辅助脚本） |
| 3 | 依赖表写 CGAL 5.6；语言写 C++14 | 实测 CGAL 6.2.1 可用；代码实际需要 C++17 |
| 4 | 处理流程止于"OBJ 构建器" | 未提及 1.12 专有的 `Level` 硬编码、无 `DataVersion` 校验、只支持 zlib 压缩类型 |
