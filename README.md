- [English](#EGUS)
- [中文](#ZHCN)

### EGUS
# V2 Introduction

This project is designed to parse **Minecraft** save files, extract model structures from the [Little Tiles](https://github.com/CreativeMD/LittleTiles) mod, and convert them into **OBJ** format files.

## Current Status

At the current stage, **Version 2** has implemented the core functionality:

- Export each tile as an **OFF** model file  
- Convert OFF files to **OBJ** via Python for editing in third-party modeling software  
- Cross-platform compilation support for **Windows** and **macOS**  

⚠️ Due to platform differences, model computation results may vary slightly between Windows and macOS.  

Therefore, **Version 2** officially replaces the first version and will serve as the foundation for future feature updates and technical support.  

## Tested Environment

- **Minecraft 1.12.2**  
- **Little Tiles 1.5.66**  

## Processing Workflow

Once the target chunk coordinates are provided, the approximate processing steps are:

1. Compute the corresponding region coordinates  
2. Read the `mca` binary file to obtain the 8KiB region index  
3. Calculate the corresponding chunk index and read its compressed binary data  
4. Decompress using [boost](https://archives.boost.io) and [zlib](https://zlib.net/)  
5. Parse the data with [libnbt++](https://github.com/ljfa-ag/libnbtplusplus)  
6. Pass the chunk’s root NBT data to the **Little Tiles parser** for interpretation  
7. Use [CGAL](https://www.cgal.org/2023/07/28/cgal56/) to construct the geometric structure of the Little Tiles data  
8. Finally, use the **OBJ file builder** to generate an OBJ file from the constructed geometry  

# Usage 
## For Windows
1. Use vcpkg to manage dependencies such as zlib, boost, and cgal.
2. Configure the `VCPKG_ROOT` system environment variable.
3. Since `VCPKG_ROOT` uses MSVC for compilation, the project should also be compiled with MSVC.
4. Clone the project and configure the toolchain and CMake.
5. Execute `CMakeLists.txt`, then build `libnbt++` to fulfill the dependency (this project relies on this library for NBT parsing).
6. Finally, compile and run LittleTilesReader.

## For Mac
1. Use Homebrew to manage dependencies such as zlib, boost, and cgal:
    ```
    brew install boost
    brew install zlib
    brew install cgal
    ```
2. Clone the project.
3. Run CMake to configure the project; it will automatically check for required dependencies.
4. Compile and run LittleTilesReader.

# Development Plan (Feature Roadmap)

- [x] Use CMake for project management
- [x] Refactoring
- [ ] UV, normals, and texture data construction
- [ ] ...

# Language
C++ Standard: ISO C++14

Platform Toolset: Visual Studio 2022 (v143) (msvc)

Windows SDK Version: 10.0

⚠️ Do not use MingW toolchain

# Library Dependencies

| Library Name | Version |
|-------|------|
|boost (https://archives.boost.io/release/1.85.0/source/)|1.85.0|
|zlib (https://zlib.net/)|1.3.1|
|libnbt++ (https://github.com/ljfa-ag/libnbtplusplus)|2|
|CGAL (https://www.cgal.org/2023/07/28/cgal56/)|5.6|

# Matlab Support
The geometric structure data exported through the LittleTile mod can be converted using ```IntArrayInterpreter.py``` in the Python directory to display its geometric structure in MatLab.

For example, the SNBT ```[I;0,0,0,2,1,2,-2135499923,-65537,65537,-65537,65537,-1,131073,-1,131073]``` From:
```
{tiles:[{bBox:[I;0,0,0,2,1,2,-2135499923,-65537,65537,-65537,65537,-1,131073,-1,131073],tile:{block:"minecraft:stone"}}],min:[I;0,0,0],size:[I;2,1,2],grid:2,count:1}
```

When provided to this script, it outputs the corresponding MatLab code:

```
% Define grid size
grid_type = 2;

% Define the eight vertices of the cube
block_aabb = LtBlock(0,0,0,2,2,2);
block = LtBlock(0,0,0,2,1,2);
block.applyOffset(AngleID.WDS, LtPoint(2, 0, 1));
block.applyOffset(AngleID.WDN, LtPoint(-1, 0, -1));
block.applyOffset(AngleID.WUS, LtPoint(2, 0, 1));
block.applyOffset(AngleID.WUN, LtPoint(-1, 0, -1));
block.applyOffset(AngleID.EDS, LtPoint(1, 0, 1));
block.applyOffset(AngleID.EDN, LtPoint(-2, 0, -1));
block.applyOffset(AngleID.EUS, LtPoint(1, 0, 1));
block.applyOffset(AngleID.EUN, LtPoint(-2, 0, -1));
% Plot
showGrid(grid_type, -2, 2);
patchLtBlock(block, 'green', 0.7);
patchLtBlock(block_aabb, 'blue', 0.1);

%cyan
```
<img width="628" height="567" alt="Screenshot 2025-09-01 at 13 40 34" src="https://github.com/user-attachments/assets/89748dc6-6f5f-43a5-949d-14306b0356d5" />

# Export Example
![image](https://github.com/user-attachments/assets/034008b4-f20e-424f-8a9d-377b32a4b70a)
![image](https://github.com/user-attachments/assets/23f98b62-a88a-4360-82e0-9f0e506f7876)

### ZHCN
# V2 简介

本项目用于解析 **Minecraft** 存档，从 [Little Tiles](https://github.com/CreativeMD/LittleTiles) 模组中提取模型结构，并将其转换为 **OBJ** 格式文件。

## 当前状态

在现阶段，**Version 2** 已经实现了基础功能：

- 将每个 tile 导出为 **OFF** 模型文件  
- 通过 Python 将 OFF 文件转换为 **OBJ**，以便在第三方建模软件中进行编辑  
- 支持 **Windows** 与 **macOS** 的跨平台编译  

⚠️ 由于平台差异，模型计算结果在 Windows 和 macOS 上可能会略有不同。  

因此，**Version 2** 将正式取代第一版，并作为后续功能更新与技术支持的基础。  

## 测试环境

- **Minecraft 1.12.2**  
- **Little Tiles 1.5.66**  

## 处理流程

当获得目标区块坐标后，大致的处理步骤如下：

1. 计算对应的区域坐标  
2. 读取 `mca` 二进制文件以获取 8KiB 区域索引  
3. 计算对应区块索引并读取其压缩的二进制数据  
4. 使用 [boost](https://archives.boost.io) 和 [zlib](https://zlib.net/) 进行解压  
5. 使用 [libnbt++](https://github.com/ljfa-ag/libnbtplusplus) 进行解析  
6. 将区块的根 NBT 数据交由 **Little Tiles 解析器** 进行解析  
7. 使用 [CGAL](https://www.cgal.org/2023/07/28/cgal56/) 对 Little Tiles 的几何结构数据进行构建  
8. 最后使用 **OBJ 文件构建器** 将几何结构生成 OBJ 文件  

# 使用方式
## Windows 平台
1. 使用 vcpkg 管理项目需要的 zlib、boost、cgal 依赖。
2. 配置 VCPKG_ROOT 系统环境变量。
3. 由于 VCPKG_ROOT 使用 msvc 进行编译，所以该项目的编译也应使用 MSVC。
4. 克隆该项目，并配置 toolchain、cmake。
5. 执行 CMakeLists，随后执行（自动） libnbt++ 以构建 libnbt++ 依赖。（该项目的 nbt 解析由该库提供）
6. 最后对 LittleTilesReader 进行编译运行。

## Mac 平台
1. 使用 Homebrew 管理项目需要的 zlib、boost、cgal 依赖。
    ```
    brew install boost
    brew install zlib
    brew install cgal
    ```
2. 克隆该项目。
3. 执行CMakeList，CMake 会自动检查所需依赖。
4. 最后对 LittleTilesReader 进行编译运行。

# 开发计划（画大饼）
- [x] 使用 CMake 进行项目管理
- [x] 重构
- [ ] UV、法线、材质等数据的构建
- [ ] ...

# 语言
C++ 语言标准: ISO C++14 标准

平台工具集：Visual Studio 2022 (v143) (msvc)

Windows SDK 版本：10.0

⚠️ 请勿使用 MingW 工具链 (Toolchain)

# 库依赖
| 库名称 | 版本 |
|-------|------|
|boost (https://archives.boost.io/release/1.85.0/source/)|1.85.0|
|zlib (https://zlib.net/)|1.3.1|
|libnbt++ (https://github.com/ljfa-ag/libnbtplusplus)|2|
|CGAL (https://www.cgal.org/2023/07/28/cgal56/)|5.6|

# Matlab 支持
通过 LittleTile 模组的到出台导出的几何结构数据可以通过Python目录中的 ```IntArrayInterpreter.py``` 进行转换以在 MatLab 中展示其几何结构
如 SNBT 中的 ```[I;0,0,0,2,1,2,-2135499923,-65537,65537,-65537,65537,-1,131073,-1,131073]```:
```
{tiles:[{bBox:[I;0,0,0,2,1,2,-2135499923,-65537,65537,-65537,65537,-1,131073,-1,131073],tile:{block:"minecraft:stone"}}],min:[I;0,0,0],size:[I;2,1,2],grid:2,count:1}
```
将其提供给该脚本后会输出对应的 matlab 代码：
```
% 定义网格大小
grid_type = 2;

% 定义立方体的八个顶点
block_aabb = LtBlock(0,0,0,2,2,2);
block = LtBlock(0,0,0,2,1,2);
block.applyOffset(AngleID.WDS, LtPoint(2, 0, 1));
block.applyOffset(AngleID.WDN, LtPoint(-1, 0, -1));
block.applyOffset(AngleID.WUS, LtPoint(2, 0, 1));
block.applyOffset(AngleID.WUN, LtPoint(-1, 0, -1));
block.applyOffset(AngleID.EDS, LtPoint(1, 0, 1));
block.applyOffset(AngleID.EDN, LtPoint(-2, 0, -1));
block.applyOffset(AngleID.EUS, LtPoint(1, 0, 1));
block.applyOffset(AngleID.EUN, LtPoint(-2, 0, -1));
% 绘制
showGrid(grid_type, -2, 2);
patchLtBlock(block, 'green', 0.7);
patchLtBlock(block_aabb, 'blue', 0.1);

%cyan
```
<img width="628" height="567" alt="Screenshot 2025-09-01 at 13 40 34" src="https://github.com/user-attachments/assets/89748dc6-6f5f-43a5-949d-14306b0356d5" />

# 导出示例
![image](https://github.com/user-attachments/assets/034008b4-f20e-424f-8a9d-377b32a4b70a)
![image](https://github.com/user-attachments/assets/23f98b62-a88a-4360-82e0-9f0e506f7876)

# Engineering Documentation

Detailed engineering notes (verified against the code, with `file:line` references) live in [`docs/`](docs/):

- [`docs/architecture.md`](docs/architecture.md) — module layout, the real call chain, ownership/thread-safety status, build
- [`docs/nbt-format.md`](docs/nbt-format.md) — `.mca` / chunk NBT / LittleTiles tile format, offset bit layout, coordinate system
- [`docs/known-issues.md`](docs/known-issues.md) — verified defects, output determinism, test-data baselines, README/code mismatches
- [`docs/reference-lt3d-importer.md`](docs/reference-lt3d-importer.md) — study of the third-party LT 3D Importer & Exporter mod (UV/texture strategies)

# 工程文档

详细的工程笔记放在 [`docs/`](docs/) 目录（结论均对照代码核实，附 `文件:行` 引用）：

- [`docs/architecture.md`](docs/architecture.md) —— 模块划分、真实调用链、ownership 与线程安全现状、构建方式
- [`docs/nbt-format.md`](docs/nbt-format.md) —— `.mca` / chunk NBT / LittleTiles tile 格式、角度偏移位域、坐标系统
- [`docs/known-issues.md`](docs/known-issues.md) —— 已核实缺陷、输出确定性、测试基线与 README/代码不一致清单
- [`docs/reference-lt3d-importer.md`](docs/reference-lt3d-importer.md) —— 第三方模组 LT 3D Importer & Exporter 的研究（UV/纹理策略）
