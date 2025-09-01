- [English](#EGUS)
- [中文](#ZHCN)

### EGUS
# V2 Introduction
This project is used to parse Minecraft save files and extract model structures from the Little Tiles mod, converting them into Obj format files.

At this stage, the V2 version has implemented basic functionalities, enabling the export of OFF model files for each tile and supporting their conversion to OBJ files one by one via Python. This allows for subsequent editing in third-party modeling software. The project now supports cross-platform compilation for both Windows and macOS. However, due to platform differences, the model calculation results on macOS differ somewhat from those on the Windows platform.

Therefore, the V2 version will serve as the official release, replacing the first version. Subsequent feature updates and technical support will be based on this version.

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
The author is currently working on version 2 of this project, which is not yet fully operational. Please do not use this version for production.

This project is developed and tested only on the Windows platform.

# Usage (Windows)
1. Use vcpkg to manage dependencies such as zlib, boost, and cgal.
2. Configure the `VCPKG_ROOT` system environment variable.
3. Since `VCPKG_ROOT` uses MSVC for compilation, the project should also be compiled with MSVC.
4. Clone the project and configure the toolchain and CMake.
5. Execute `CMakeLists.txt`, then build `libnbt++` to fulfill the dependency (this project relies on this library for NBT parsing).
6. Finally, compile and run LittleTilesReader.

# Usage (Mac)
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
本项目用于解析《Minecraft》存档从中获取 [Little Tiles](https://github.com/CreativeMD/LittleTiles) 模组中的模型结构将其转换为 Obj 格式的文件。

现阶段，V2版本已实现基础功能，能够导出每个 tile 的 OFF 模型文件，并支持通过 Python 将其逐一转换为 OBJ 文件，以便在第三方模型编辑软件中进行后续编辑。该项目现已支持 Windows 与 macOS 的跨平台编译，但由于平台差异，macOS 上的模型计算结果与 Windows 平台存在一定区别。

因此，V2 版本将作为正式版本替代第一版，并基于该版本提供后续功能更新与技术支持。

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

# 使用方式 (Windows 平台)
1. 使用 vcpkg 管理项目需要的 zlib、boost、cgal 依赖。
2. 配置 VCPKG_ROOT 系统环境变量。
3. 由于 VCPKG_ROOT 使用 msvc 进行编译，所以该项目的编译也应使用 MSVC。
4. 克隆该项目，并配置 toolchain、cmake。
5. 执行 CMakeLists，随后执行（自动） libnbt++ 以构建 libnbt++ 依赖。（该项目的 nbt 解析由该库提供）
6. 最后对 LittleTilesReader 进行编译运行。

# 使用方式 (Mac 平台)
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
- [ ] CGAL 分支 - 加速计算支持
- [ ] ...

# 语言
C++ 语言标准: ISO C++14 标准

平台工具集：Visual Studio 2022 (v143)

Windows SDK 版本：10.0

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
