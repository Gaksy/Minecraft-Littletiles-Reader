# Minecraft LittleTiles Reader

[English](README.md) | **简体中文**

解析 **Minecraft** 存档（Anvil `.mca`），读取
[Little Tiles](https://github.com/CreativeMD/LittleTiles) 模组的 tile 数据，
重建为标准 **OBJ** 模型（OBJ + MTL + 贴图），可直接导入 Blender 等建模软件。

## 当前状态

- 读取 region 存档（`.mca`）→ 区块 NBT → LittleTiles tile entity
- 提供裸 NBT 入口（`ChunkTileEntities::ReadChunkNbt`），不经过 mca 文件也能解析
- 重建 tile 几何，含 LittleTiles 的逐角角度偏移与越界 tile（按方块盒裁剪）
- 普通方块（非 LittleTiles）导出为完整立方体，可选剔除被邻居挡住的面
- 六面 UV；生物群系 tint 与 tile 自带颜色逐像素烘焙进导出的 PNG
- 可选用自定义材质包（目录 / zip / rar）叠加在原版素材之上
- 界面支持中文与英文

## 测试环境

- **Minecraft 1.12.2**
- **Little Tiles 1.5.66**

## 测试存档

用于验证的 region 存档都在仓库里（均为 Minecraft 1.12.2 + Little Tiles 1.5.66）。
运行时依次输入存档目录、区块坐标、扫描半径：

| 目录 | 区块坐标 (x, z) | 推荐范围 | 扫描规模 | 基线（Debug 构建，开启完整方块、剔除相邻面） |
|---|---|---|---|---|
| `test_region/` | **0, 0** | **1** | 3×3 区块 | 236 个 tile → 4881 面 → 0.3 MB OBJ，约 0.1 s |
| `test_region_medim/` | **-136, 49** | **5** | 11×11 区块 | 55561 个 tile → 388744 面 → 34 MB OBJ，约 9 s |
| `test_region_large/` | **-7, -26** | **5** | 11×11 区块 | 324427 个 tile + 194 万普通方块 → 2036139 面 → 178 MB OBJ，约 50 s |

`python3 tools/benchmark.py` 会用上面这套参数把三个存档各跑一遍并记录结果，
历史记录见 [`docs/benchmark.md`](docs/benchmark.md)。

```sh
# 依次为：语言（1 = 中文，2 = English）、存档目录、区块 x、区块 z、半径、
# 完整方块、剔除相邻面、居中、单位化、进度与耗时
printf "1\ntest_region_large\n-7\n-26\n5\ny\ny\ny\nn\ny\n" | ./LittleTilesReader
```

**第一个问题选界面语言**（直接回车 = 简体中文）：所有提示、进度、结果行都有中英两版，
切一次语言全程生效。最后一项同时控制**进度提示**（每处理一个区块打印 `[进度] 区块 (x, z) —— i/n`，
以及库里逐区块、逐方块的详细信息）和结尾的**总耗时**一行。
想要干净的、方便脚本处理的输出就答 `n`。

产物写在**当前工作目录**下的 `out_file/`（OBJ + MTL + 同名 `<obj 名>_textures/` 贴图目录）。

## 使用自定义材质包

贴图来自一个素材根目录（默认 `assets/1.12.2`）。想用别的材质包（例如工作室内部包，
不入库），先把材质包与原版素材合并：

```sh
python3 tools/build_assets_from_pack.py --pack "texture/MyPack.zip" --out assets/pack
```

合并出的目录就是标准素材根：材质包里有的贴图用它自己的，没有的回退原版。指定方式两种——
环境变量 `LITTLETILES_ASSETS=assets/pack`，或运行时在 `assets root (blank = auto-detect):`
那一问里直接填路径（可留空走自动探测）。相对路径依次按「当前工作目录 → 可执行文件目录 →
可执行文件上一级」解析（CLion 的工作目录是构建目录，所以这样在哪儿都能填 `assets/pack`），
选中后会打印成绝对路径，方便确认到底用了哪一份素材。细节与限制见
[`docs/texture-mapping.md`](docs/texture-mapping.md)。

## 处理流程

当获得目标区块坐标后，大致的处理步骤如下：

1. 计算对应的区域坐标
2. 读取 `mca` 二进制文件以获取 8KiB 区域索引
3. 计算对应区块索引并读取其压缩的二进制数据
4. 使用 [boost](https://archives.boost.io) 和 [zlib](https://zlib.net/) 进行解压
5. 使用 [libnbt++](https://github.com/ljfa-ag/libnbtplusplus) 进行解析
6. 将区块的根 NBT 数据交由 **Little Tiles 解析器** 进行解析
7. 使用 [CGAL](https://www.cgal.org/) 构建 Little Tiles 的几何结构
8. 最后使用 **OBJ 文件构建器** 导出 OBJ（含 MTL 与贴图）

## 使用方式

依赖统一交给 **vcpkg** 管理（macOS 上没有 vcpkg 时回退 Homebrew）。

```sh
cmake -S . -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug
```

### Windows

1. 使用 vcpkg 管理 zlib、boost、cgal、libnbt++ 依赖。
2. 配置 `VCPKG_ROOT` 系统环境变量。
3. `VCPKG_ROOT` 使用 MSVC 编译，因此本项目也应使用 MSVC。
4. 克隆项目并配置 toolchain、CMake。
5. 执行 CMakeLists，随后构建 libnbt++ 依赖（本项目的 NBT 解析由该库提供）。
6. 编译运行 LittleTilesReader。

### macOS

1. 用 Homebrew 安装依赖：
    ```
    brew install boost
    brew install zlib
    brew install cgal
    ```
2. 克隆项目。
3. 执行 CMake，它会自动检查所需依赖。
4. 编译运行 LittleTilesReader。

## 开发计划

- [x] 使用 CMake 进行项目管理
- [x] 重构
- [x] UV、材质与贴图烘焙
- [ ] 法线导出
- [ ] glTF / GLB 导出
- [ ] 服务器化库接口

## 语言与工具链

- C++ 语言标准：**ISO C++17**（代码用到嵌套命名空间定义、if-init 与 `std::filesystem`，
  CGAL 6.x 也要求 C++17；此前文档里写的 C++14 与代码不符）
- 平台工具集：Visual Studio 2022 (v143) / MSVC；macOS 上为 Apple Clang
- Windows SDK 版本：10.0
- ⚠️ 请勿使用 MinGW 工具链

## 库依赖

由 vcpkg 管理（macOS 回退 Homebrew）；libnbt++ 钉在固定 commit 上，保证构建可复现。

| 库名称 | 版本 | 用途 |
|---|---|---|
| boost | 1.92.0（CMake 要求 ≥ 1.74） | iostreams 的 zlib 过滤器 |
| zlib | 1.3.2 | 区块解压 |
| libnbt++ | commit `687e4303` | NBT 解析 |
| CGAL | 6.2.1 | 网格与几何运算 |

## 已知限制

- **跨平台浮点精度**：`FloatType` 目前在 Windows 上是 `float`、macOS 上是 `double`
  （`CgalTypeDef.h`），因此两个平台的几何结果可能存在细微差异。
  服务器化前应统一为 `double`，保证"同一输入 → 同一输出"。
- 仅支持 1.12 的存档结构（`Level` 下的 `Sections`、zlib 压缩类型）。
- 材质包里的 MCPatcher/CTM 连接纹理暂不支持，用的是连接纹理的基础贴图。

## Matlab 支持

通过 LittleTile 模组导出的几何结构数据可以通过 Python 目录中的
`IntArrayInterpreter.py` 进行转换，以在 MatLab 中展示其几何结构。
如 SNBT 中的 `[I;0,0,0,2,1,2,-2135499923,-65537,65537,-65537,65537,-1,131073,-1,131073]`：
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
```
<img width="628" height="567" alt="Screenshot 2025-09-01 at 13 40 34" src="https://github.com/user-attachments/assets/89748dc6-6f5f-43a5-949d-14306b0356d5" />

## 导出示例

![image](https://github.com/user-attachments/assets/034008b4-f20e-424f-8a9d-377b32a4b70a)
![image](https://github.com/user-attachments/assets/23f98b62-a88a-4360-82e0-9f0e506f7876)

## 工程文档

详细的工程笔记放在 [`docs/`](docs/) 目录（结论均对照代码核实，引用符号名而非行号）：

- [`docs/architecture.md`](docs/architecture.md) —— 模块划分、真实调用链、ownership 与线程安全现状、构建方式
- [`docs/nbt-format.md`](docs/nbt-format.md) —— `.mca` / chunk NBT / LittleTiles tile 格式、角度偏移位域、坐标系统
- [`docs/known-issues.md`](docs/known-issues.md) —— 已核实缺陷、输出确定性、测试基线与 README/代码不一致清单
- [`docs/texture-mapping.md`](docs/texture-mapping.md) —— UV 约定、tint 烘焙、材质包叠加
- [`docs/benchmark.md`](docs/benchmark.md) —— 导出基线与实测数据（由 `tools/benchmark.py` 生成）
- [`docs/reference-lt3d-importer.md`](docs/reference-lt3d-importer.md) —— 第三方模组 LT 3D Importer & Exporter 的研究（UV/纹理策略）
