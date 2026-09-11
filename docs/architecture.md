# 架构与调用链

## 1. 总体结构

```
CLI / Web Server（消费者）
        ↓
     galib 核心库（STATIC）
        ↓
   Anvil → NBT → LittleTiles 语义 → CGAL 几何 → Mesh → OBJ/（未来 glTF）
```

`galib` 是静态库目标（`Galib/CMakeLists.txt:3`），`main.cpp` 只是它的一个消费者
（`CMakeLists.txt:64-65`）。库的 include 与依赖（Boost::iostreams / ZLIB / nbt++ / CGAL）
以 `PUBLIC` 暴露，调用方只需链接 `galib`。

命名空间分四层，注意不要混淆：

| 命名空间 | 内容 |
|---|---|
| `galib::coord` | `Coordinate2D` / `Coordinate3D` / `coord_type_traits`（类型化坐标，别退化成裸 double） |
| `galib::minecraft` | `BlockCoordinate` / `ChunkCoordinate` / `RegionCoordinate` / `AnvilReader` / `CacheManagerBase` |
| `galib::minecraft::littletiles` | `LittleTilesCoord` / `GridType` / `OffsetType` / `TileEntity` / `BlockTileEntities` / `ChunkTileEntities` |
| `galib::minecraft::cgal_support` | `LtSurfaceMesh` / `ChunkMesh` / mesh 构建与 OBJ 写出 |

## 2. 两条入口

### 2.1 Anvil 路径（文件 → 模型）

```
main()
 └ AnvilReader::setRegionFolder(const char*)                   Anvil.cpp:72
 └ AnvilReader::getChunkDataReference(ChunkCoordinate)         Anvil.cpp:90
    └ mca_cache_ 未命中 → readMcaFile_ 读整份 .mca 进 ByteArray        Anvil.cpp:97-119
    └ chunk_cache_[region] 不存在 → SingleChunkManager(32)             Anvil.cpp:124-127
    └ getChunkConstIterator_  解析 8KiB 索引 + 2KiB 头                  Anvil.cpp:147,206
    └ decompressChunkBinaryData_  boost::iostreams zlib（仅类型 2）      Anvil.cpp:155,264
    └ decompressChunkBinaryNbtData_  libnbt++ stream_reader              Anvil.cpp:161,293
    └ 返回 {chunk_info, p_chunk_root, p_chunk_level=&root.at("Level")}   Anvil.cpp:173-177
 └ ChunkTileEntities::readChunk(ChunkDataReference&)            ChunkTileEntities.cpp:41
 └ ChunkMesh::addTilesFromChukTileEntities(ChunkTileEntities&)  CgalLittletilesBuilder.cpp:117
 └ margeAndWriteToObj(meshes, path, geom_center)                CgalLittletilesBuilder.cpp:144
```

解析链细节：

```
readTileEntities_                    ChunkTileEntities.cpp:97
 └ 取 level."TileEntities" 列表       ChunkTileEntities.cpp:102-107
 └ BlockTileEntities::readBlockTileNBT        BlockTileEntities.cpp:51
    └ readBoxesTilesNbt_（"boxes" 列表 / "box" 单值）  :155,166,169
    └ setAngleOffsetStateData_（int_array[6..] 位域）  :198,237
```

几何构建：

```
addTilesFromBlockTilesEntities       CgalLittletilesBuilder.cpp:53
 ├ createMeshFromTileEntity（8 顶点 + 12 三角形，含 Flipped 分支）  CgalLtSupport.cpp:38
 ├ 若偏移越界 → cleanupMesh + corefine_and_compute_intersection 裁剪  :86
 ├ applyGrid / applyWorldOffset       CgalLtSupport.cpp:201,181
 └ 逐 tile 存入 ChunkMesh::tiles_in_world_
```

注意：`addTilesFromBlockTilesEntities` 与 `writeMeshToOff` **定义在全局命名空间**，
未进头文件（`CgalLittletilesBuilder.cpp:53,273`），只在同一 TU 内可见。

### 2.2 裸 NBT 路径（NBT → 模型）

`ChunkTileEntities::readChunkNbt(const nbt::tag_compound&)`（`LittleTiles.h:176`，
实现 `ChunkTileEntities.cpp:67`）不经过 Anvil / 文件系统：

- 优先取根下 `Level`，不存在则把根自身当作 level（兼容 1.18+ 扁平结构）
- 若 level 内有 `xPos`/`zPos` 则同步区块坐标
- 与 `readChunk` 共用 `readTileEntities_`（`ChunkTileEntities.cpp:97`），两条入口行为一致

块级入口早已存在：`BlockTileEntities::readBlockTileNBT(const tag_compound&, size_type*)`
（`LittleTiles.h:129`），可直接吃单个 block tile entity 的 compound。

## 3. 类型与 ownership

| 类型 | 说明 |
|---|---|
| `AnvilReader::ChunkDataReference` | POD，持有**非拥有裸指针** `tag_compound* p_chunk_root/p_chunk_level`，指向缓存内由 `unique_ptr` 持有的对象 → 缓存清空即悬垂（`Anvil.h:180-184`） |
| `ChunkData::ChunkNbtRoot` | `std::unique_ptr<nbt::tag_compound>`，唯一真正拥有者（`Anvil.h:189`） |
| `ChunkData::p_chunk_level` | **未初始化且从未被赋值**（`Anvil.h:190`），目前无人读取，属埋雷 |
| `ChunkTileEntities` / `BlockTileEntities` | 值语义容器（vector / map），解析后持有数据，可移动 |
| `ChunkMesh::tiles_in_world_` | `vector<LtSurfaceMesh>`，每个 tile 一份 CGAL `Surface_mesh`（值拷贝） |
| `LtSurfaceMesh::getMeshWithOffset` | 按值返回整张 mesh，代价高（`CgalTypeDef.cpp:60`） |

保持现代 C++ 风格：`unique_ptr`、move、`swap`、delete copy assignment；不要引入裸 `new/delete`。

## 4. 缓存

两层，均在 `AnvilReader` 实例内、**无同步、无上限**：

| 缓存 | key | value |
|---|---|---|
| `mca_cache_` | `RegionCoordinate` | 整份 `.mca` 字节数组（40 KB – 数百 KB） |
| `chunk_cache_` | `RegionCoordinate` | `CacheManagerBase<ChunkData,int32>`，固定 32×32 槽，每槽一份已解析 NBT 树 |

`CacheManagerBase`（`Anvil.h:33`）是固定尺寸二维 `unique_ptr` 槽，只支持 0..31；
`GetCachePointer`（`Anvil.h:61,70`）只读查表，`GetNewCachePointer` 是"未命中则创建"。

服务器化前必须处理：缓存 key 不含数据源标识、`setRegionFolder()` 不清缓存、
无容量上限、无并发保护。详见 `known-issues.md`。

## 5. 线程安全现状

**当前没有任何一处是线程安全的：**

- `GalibExceptionBasic<...>::last_exception_` 是模板静态可变对象，每次构造异常都写入
  （`Galib/include/Exception/GalibExceptionBasic.h`），多线程下是数据竞争与全局共享状态。
- `AnvilReader` 的缓存是可变成员且无同步；`setRegionFolder()` 只改路径不清缓存
  → 复用同一实例处理不同来源会串数据。
- `createIntersectionCube()` 用函数内 `new` 出的静态单例且从不释放
  （`CgalLtSupport.cpp:141`，当前无调用者）。

## 6. 异常体系

- `GalibExceptionBasic<ErrorCodeType> : std::exception`，自带格式化的 `what()`。
- `MinecraftException`（`MinecraftErrorCode` 1..7）、`LittleTilesException`（`LittleTilesErrorCode`）。
- 解析层大量 `try/catch(...)` 静默吞异常，只在 `GALIB_DEBUG` 下打印；
  而 `GALIB_DEBUG` 被**无条件** `#define`（`GalibNamespaceDef.h:27`）→ 服务器端日志噪音，
  且该分支下 `catch(std::exception&)` 比 `catch(...)` 更窄，非 std 异常会穿透。
- **库边界没有错误转换**：`main()` 无 try/catch，异常直接终止进程。
  V2 需要在库/服务边界映射为可返回的错误（invalid input / unsupported version / invalid NBT /
  invalid LittleTiles data / geometry failed / export failed）。

## 7. 构建

### 7.1 依赖来源优先级（`CMakeLists.txt` 顶部）

1. 显式传入的 `CMAKE_TOOLCHAIN_FILE`
2. 设置了环境变量 `VCPKG_ROOT` → 所有平台统一用 vcpkg
3. macOS 回退 Homebrew

### 7.2 已验证的依赖版本

| 依赖 | 版本 |
|---|---|
| Boost (iostreams) | 1.92.0（vcpkg） |
| zlib | 1.3.2（vcpkg） |
| CGAL | 6.2.1（vcpkg） |
| libnbtplusplus | commit `687e43031df0dc641984b4256bcca50d5b3f7de3`（FetchContent，已钉） |

注意 `CMakeLists.txt:35` 目前声明 `CMAKE_CXX_STANDARD 14`，但代码使用了 C++17 的 if-init
（`BlockTileEntities.cpp:102`）；clang 按扩展接受并有告警，MSVC `/std:c++14` 会直接失败。

### 7.3 命令与产物

```sh
cmake -S . -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug
# 产物：cmake-build-debug/Galib/libgalib.a、cmake-build-debug/LittleTilesReader
```

### 7.4 环境注意事项

- **构建缓存陈旧会让 vcpkg toolchain 静默失效**：若 `cmake-build-debug` 残留旧
  `CMakeCache.txt`，会出现"记录了 toolchain 但依赖前缀没注入"，`find_package(Boost)`
  落到 CMake 4.3 已移除的 `FindBoost` 模块并报错。解决：删掉 `CMakeCache.txt` 重新配置
  （CLion：Tools → CMake → Reset Cache and Reload Project）。
- 配置阶段需要网络（`FetchContent` 会做一次 git 更新）。离线时用
  `-DFETCHCONTENT_FULLY_DISCONNECTED=ON`，或
  `-DFETCHCONTENT_SOURCE_DIR_LIBNBTPLUSPLUS=<本地源码目录>`。
- 用 vcpkg 构建 CGAL 需要 `gmp`/`mpfr`，而它们依赖 autotools：
  `brew install autoconf autoconf-archive automake libtool`。
  若 `xcode-select -p` 指向**含空格**的路径（例如 `Xcode Beta.app`），autotools 生成的
  Makefile 会把 `MAKE` 截断导致 gmp 构建失败；可用 `MAKE=/usr/bin/make vcpkg install cgal` 绕过，
  根治方式是 `sudo xcode-select -s /Applications/Xcode.app/Contents/Developer`。
- `main.cpp` 的输出路径 `../out_file/...` 是相对于**当前工作目录**的（`main.cpp:21,24`），
  不是可执行文件所在目录。
