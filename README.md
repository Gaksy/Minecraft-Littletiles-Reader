# 简介
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
这只是最初的版本，存在各种 BUG 与性能上的问题，作者正在构建第二版。本版本只能够导出基本的白模与其每个 tiles 对应的方块id。暂时没有去合并其法线、UV贴图等资源。

# 使用方式
启动后会显示
```
LITTLE TILES OBJ EXPORTER
Region Folder:
```
在此键入您存档中 region 文件夹的路径，例如 ```D:\Minecraft\PLCII\.minecraft\versions\Lt\saves\test\region```。
```
LITTLE TILES OBJ EXPORTER
Region Folder: D:\Minecraft\PLCII\.minecraft\versions\Lt\saves\test\region
start chunk x: -1
start chunk z: 0
end chunk x: -1
end chunk z: 0
```
随后键入您要导出模型的区块的起始坐标和终点坐标，查看方式如下：
![image](https://github.com/user-attachments/assets/034008b4-f20e-424f-8a9d-377b32a4b70a)

之后等待导出成功即可
```
Reade chunk { -1, 0 }:Reading chunk successfully.
 Read chunk lt nbt:Reading chunk lt nbt successfully.

[1 / 1] Build chunk lt face { -1, 0 } offset { 0, 0 }:Building chunk lt face successfully.

Build obj to file "D:/Development/Project/Minecraft//test": Export successfully!
Elapsed time: 0:0:33
```

![image](https://github.com/user-attachments/assets/23f98b62-a88a-4360-82e0-9f0e506f7876)

# 开发计划（画大饼）
- [x] 白模导出
- [ ] 重构并使用 CMake 进行项目管理
- [ ] UV、法线、材质等数据的构建
- [ ] CGAL 分支 - 加速计算支持
- [ ] ...

# 依赖
超级抱歉：一开始直接用的 Visual Studio 进行开发，正在重构 CMake 管理的版本

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


# Introduction
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
Upon startup, the program will prompt:
```
LITTLE TILES OBJ EXPORTER
Region Folder:
```

At this point, input the path to your Minecraft save’s region folder, for example, ```D:\Minecraft\PLCII\.minecraft\versions\Lt\saves\test\region```.

```
LITTLE TILES OBJ EXPORTER
Region Folder: D:\Minecraft\PLCII\.minecraft\versions\Lt\saves\test\region
start chunk x: -1
start chunk z: 0
end chunk x: -1
end chunk z: 0
```

Then input the starting and ending chunk coordinates for the model export. The coordinates can be viewed a-s follows:

![image](https://github.com/user-attachments/assets/034008b4-f20e-424f-8a9d-377b32a4b70a)

After that, wait for the export to complete successfully:

```
Reade chunk { -1, 0 }:Reading chunk successfully.
 Read chunk lt nbt:Reading chunk lt nbt successfully.

[1 / 1] Build chunk lt face { -1, 0 } offset { 0, 0 }:Building chunk lt face successfully.

Build obj to file "D:/Development/Project/Minecraft//test": Export successfully!
Elapsed time: 0:0:33
```

![image](https://github.com/user-attachments/assets/23f98b62-a88a-4360-82e0-9f0e506f7876)

# Development Plan (Feature Roadmap)

- [x] White model export
- [ ] Refactor and use CMake for project management
- [ ] UV, normals, and texture data construction
- [ ] CGAL Branch - Acceleration support
- [ ] ...

# Dependencies

Apologies: I initially used Visual Studio for development and am currently refactoring it to use CMake for project management.

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
