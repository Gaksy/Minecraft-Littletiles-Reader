- [English](#EGUS)
- [中文](#ZHCN)

### EGUS
# V2 Introduction
This project is used to parse Minecraft save files and extract model structures from the Little Tiles mod, converting them into Obj format files.

The project is tested with Minecraft 1.12.2 and Little Tiles 1.5.66. The general processing flow after obtaining the block coordinates to be parsed is as follows:
1. Calculate the region coordinates;
2. Read the `mca` binary files to get the index of the 8KiB region;
3. Calculate the corresponding chunk index and read its compressed binary data;
4. Decompress the data using [boost](https://archives.boost.io) and [zlib](https://zlib.net/);
5. Parse the data using [libnbt++](https://github.com/ljfa-ag/libnbtplusplus);
6. Pass the chunk root NBT to the Little Tiles parser for processing;
7. Pass the parsed Little Tiles data to the obj builder for model construction, where we use the [CGAL](https://www.cgal.org) library to handle the "cutting" parts of the Little Tiles;
8. Use the obj file builder to export the model to an obj file.

# Notes
This is the initial version, which has various bugs and performance issues. The author is working on the second version. This version can only export basic white models with the corresponding block IDs for each tile. The merging of normals, UV mapping, and other resources has not been implemented yet.

# Usage
1. Use vcpkg to manage dependencies such as zlib, boost, and cgal.

2. Configure the VCPKG_ROOT system environment variable.

3. Since VCPKG_ROOT uses MSVC for compilation, the project should also be compiled with MSVC.

4. Clone the project and configure the toolchain and CMake.

5. Execute CMakeLists, then build nbt++ to fulfill the dependency (this project relies on this library for NBT parsing).

6. Finally, compile and run LittleTilesReader.

# Development Plan (Feature Roadmap)

- [x] Use CMake for project management
- [ ] Refactoring
- [ ] UV, normals, and texture data construction
- [ ] CGAL Branch - Acceleration support
- [ ] ...

# Language
C++ Standard: ISO C++14

Platform Toolset: Visual Studio 2022 (v143)

Windows SDK Version: 10.0

# Library Dependencies

| Library Name | Version |
|-------|------|
|boost (https://archives.boost.io/release/1.85.0/source/)|1.85.0|
|zlib (https://zlib.net/)|1.3.1|
|libnbt++ (https://github.com/ljfa-ag/libnbtplusplus)|2|
|CGAL (https://www.cgal.org/2023/07/28/cgal56/)|5.6|

### ZHCN
# V2 简介
本项目用于解析《Minecraft》存档从中获取 [Little Tiles](https://github.com/CreativeMD/LittleTiles) 模组中的模型结构将其转换为 Obj 格式的文件。

本项目基于《Minecraft 1.12.2》，Little Tiles 1.5.66 进行测试，当得到需要解析的区块坐标后的大致处理流程：
1. 计算其区域坐标；
2. 读取 ```mca``` 二进制文件获取 8KiB 区的索引；
3. 计算对应区块索引并读取其压缩二进制数据；
4. 通过 [boost](https://archives.boost.io) 和 [zlib](https://zlib.net/) 进行解压；
5. 通过 [libnbt++](https://github.com/ljfa-ag/libnbtplusplus) 进行解析；
6. 将区块根 NBT 交由 Little Tiles 解析器进行解析；
7. 将解析后的 Little Tiles 数据交由 obj 构建器进行构建模型，其中我们使用 [CGAL](https://www.cgal.org) 库对； Little tiles 的“切割”部分进行处理；
8. 使用 obj 文件构建器将 obj 模型构建到 obj 文件中。

# 注意
作者正在构建第二版（也就是该分支），还未能够正常运行，请勿使用！

仅在 Windows 平台开发测试

# 使用方式
1. 使用 vcpkg 管理项目需要的 zlib、boost、cgal 依赖。
2. 配置 VCPKG_ROOT 系统环境变量。
3. 由于 VCPKG_ROOT 使用 msvc 进行编译，所以该项目的编译也应使用 MSVC。
4. 克隆该项目，并配置 toolchain、cmake。
5. 执行 CMakeLists，随后执行 nbt++ 以构建 nbt++ 依赖。（该项目的 nbt 解析由该库提供）
6. 最后对 LittleTilesReader 进行编译运行

# 开发计划（画大饼）
- [x] 使用 CMake 进行项目管理
- [ ] 重构
- [ ] UV、法线、材质等数据的构建
- [ ] CGAL 分支 - 加速计算支持
- [ ] ...

## 语言
C++ 语言标准: ISO C++14 标准

平台工具集：Visual Studio 2022 (v143)

Windows SDK 版本：10.0

## 库依赖
| 库名称 | 版本 |
|-------|------|
|boost (https://archives.boost.io/release/1.85.0/source/)|1.85.0|
|zlib (https://zlib.net/)|1.3.1|
|libnbt++ (https://github.com/ljfa-ag/libnbtplusplus)|2|
|CGAL (https://www.cgal.org/2023/07/28/cgal56/)|5.6|
