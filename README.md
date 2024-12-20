# 简介
本项目用于读取并解析 Minecraft region 文件夹中的存档文件，即 mca 文件。在解析后读取并构建 LittleTiles 模组中的模型，然后将其转换为 Obj 模型。

# 依赖
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

# 使用方式
启动后会显示
```cmd
LITTLE TILES OBJ EXPORTER
Region Folder:
```
在此键入您存档中 region 文件夹的路径，例如 ```D:\Minecraft\PLCII\.minecraft\versions\Lt\saves\test\region```。
```cmd
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
```cmd
Reade chunk { -1, 0 }:Reading chunk successfully.
 Read chunk lt nbt:Reading chunk lt nbt successfully.

[1 / 1] Build chunk lt face { -1, 0 } offset { 0, 0 }:Building chunk lt face successfully.

Build obj to file "D:/Development/Project/Minecraft//test": Export successfully!
Elapsed time: 0:0:33
```

![image](https://github.com/user-attachments/assets/23f98b62-a88a-4360-82e0-9f0e506f7876)

# 注意
这只是最初的版本，存在各种BUG与性能上的问题，作者正在构建第二版

# Introduction
This project is designed to read and parse the saved files in the Minecraft region folder, specifically the mca files. After parsing, it reads and constructs models from the LittleTiles mod and then converts them into Obj models.

# Dependencies
## Language
C++ Language Standard: ISO C++14 Standard

Platform Toolset: Visual Studio 2022 (v143)

Windows SDK Version: 10.0

## Library Dependencies
| library Name | Version |
|-------|------|
|boost (https://archives.boost.io/release/1.85.0/source/)|1.85.0|
|zlib (https://zlib.net/)|1.3.1|
|libnbt++ (https://github.com/ljfa-ag/libnbtplusplus)|2|
|CGAL (https://www.cgal.org/2023/07/28/cgal56/)|5.6|

# Usage
After starting, the following will be displayed:
```cmd
LITTLE TILES OBJ EXPORTER  
Region Folder:  
```
Enter the path to the region folder in your save, for example, ```D:\Minecraft\PLCII\.minecraft\versions\Lt\saves\test\region```.

```cmd
LITTLE TILES OBJ EXPORTER  
Region Folder: D:\Minecraft\PLCII\.minecraft\versions\Lt\saves\test\region  
start chunk x: -1  
start chunk z: 0  
end chunk x: -1  
end chunk z: 0  
```
Next, enter the starting and ending coordinates of the chunk you want to export the model for. The coordinates can be viewed as shown below:
![image](https://github.com/user-attachments/assets/034008b4-f20e-424f-8a9d-377b32a4b70a)

After that, wait for the export to succeed:
```cmd
Read chunk { -1, 0 }: Reading chunk successfully.  
Read chunk lt nbt: Reading chunk lt nbt successfully.  

[1 / 1] Build chunk lt face { -1, 0 } offset { 0, 0 }: Building chunk lt face successfully.  

Build obj to file "D:/Development/Project/Minecraft//test": Export successfully!  
Elapsed time: 0:0:33  
```

# Notes
# Usage  
After starting, the following will be displayed:  
```cmd  
LITTLE TILES OBJ EXPORTER  
Region Folder:  
```  
Enter the path to the region folder in your save, for example, ```D:\Minecraft\PLCII\.minecraft\versions\Lt\saves\test\region```.  
```cmd  
LITTLE TILES OBJ EXPORTER  
Region Folder: D:\Minecraft\PLCII\.minecraft\versions\Lt\saves\test\region  
start chunk x: -1  
start chunk z: 0  
end chunk x: -1  
end chunk z: 0  
```  
Next, enter the starting and ending coordinates of the chunk you want to export the model for. The coordinates can be viewed as shown below:  
![image](https://github.com/user-attachments/assets/034008b4-f20e-424f-8a9d-377b32a4b70a)  

After that, wait for the export to succeed:  
```cmd  
Read chunk { -1, 0 }: Reading chunk successfully.  
Read chunk lt nbt: Reading chunk lt nbt successfully.  

[1 / 1] Build chunk lt face { -1, 0 } offset { 0, 0 }: Building chunk lt face successfully.  

Build obj to file "D:/Development/Project/Minecraft//test": Export successfully!  
Elapsed time: 0:0:33  
```  
![image](https://github.com/user-attachments/assets/b278d0c3-778e-49ac-adfe-7c7d2c65d192)

# Notes  
This is just the initial version, and there are various bugs and performance issues. The author is currently working on the second version.
