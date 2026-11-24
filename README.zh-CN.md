# Minecraft LittleTiles Reader

[English](README.md) | **简体中文**

解析 **Minecraft** 存档（Anvil `.mca`），读取
[Little Tiles](https://github.com/CreativeMD/LittleTiles) 模组的 tile 数据，
重建为带贴图的 **OBJ** 模型（OBJ + MTL + PNG），或可编辑的
**Blockbench Java Block/Item 工程**（`.bbmodel`）。

> **关于后续开发**  
> 由于现在 AI 已经足够强大，在本项目已有的架构与技术文档基础上，完全可以接手接下来的
> 任务。因此后续的代码更新，大部分将由我与 AI 协作完成。

## 版本与范围

本仓库提供 **C++ 核心库和 CLI，版本 0.3.0-beta**。配套的 **App 0.2.0** 是独立的
Windows x64 测试版，使用 Flutter 界面；App 与核心库的版本号、发行周期分别管理。
macOS App 暂缓更新。下载入口：[LittleTiles Reader](https://www.inception.work/littletiles)。

| 输入 | 当前范围 |
|---|---|
| 原生 Forge 1.12.2 存档 | `DataVersion` 1343；旧数字 ID 与 metadata；可从存档 Forge 注册表读取模组方块 ID |
| 原生 Forge 1.20.1 存档 | `DataVersion` 3465；完整命名空间方块状态与属性；带材质导出要求匹配的 v2 素材 |
| LittleTiles 结构 SNBT | 旧版 `tiles` 列表与新版 `t` 映射，含子集合和各层独立 grid |

当前 App 工作流**只开放主世界导出**。核心仍保留其他维度参数，这不代表下界或末地
已通过本版验收。

仅支持从创建起就使用对应游戏版本和模组环境的存档。**不支持升级、降级或跨版本迁移的存档**；
Reader 不升级或修改存档。区块元数据不能证明创建历史，`source_history: "native"` 是调用者
提供的来源声明，不是绕过迁移限制的开关。

新版 SNBT 格式家族也见于 1.20 / 1.21 蓝图。识别该格式不等于识别精确的 Minecraft／LT
版本，更不代表支持原生 1.21 存档。历史实物检查和合成数据测试范围见
[1.20.1 适配记录](docs/native-world-export-1.20.1-progress.md)。

## 导出能力

- region 文件（`.mca`）→ 区块 NBT → LT 几何；读取整体结构子集合内的小方块，
  也提供不经过 MCA 的 `ChunkTileEntities::ReadChunkNbt` 入口。
- SNBT → OBJ：在几何构建器支持范围内保留逐角偏移；输出静态模型，不迁移门动画或交互。
- SNBT → `java_block` `.bbmodel`：整体工程或自动拆成单方块工程，限制见下文。
- 普通方块采用立方体几何；素材包 `cross_states.txt` 中的植物状态采用交叉平面。
  这不是楼梯、multipart、自定义 loader 或方块实体的通用模型渲染器。
- UV 保留源结构跨格的贴图相位，不受输出居中／缩放影响；tint 与 tile 颜色烘焙进 PNG，
  动态贴图取第一帧。默认染色是近似值，不是读取原存档每个位置的生物群系颜色。
- OBJ 输出 MTL 和 PNG；含透明像素的材质增加 `map_d`。缺失映射会提示面数，并在存在缺失项时
  输出 `<name>_missing_materials.tsv`，受影响的面使用白色回退。BB 缺少材质映射时拒绝导出，
  不静默删除几何。
- 核心只读取一个准备好的素材目录；原版、模组和资源包的分版本导入、排序与组合由 App／生成端负责。
- CLI 提示**仅英文**；桌面 App 的七种界面语言是独立功能。

## Blockbench 导出

CLI 输出 BB 工程格式 **5.0**、工作区 **`java_block`**，需要使用可打开该工程版本的
Blockbench。核心不直接输出 Java 方块 JSON；在 BB 中检查、编辑后再导出。

- 仅支持标准轴对齐长方体。任何带角点偏移的斜面盒子都会导致整次导出失败；
  不导出门动画、开关或交互行为。
- **整体与拆分两种模式，都要求整份输入至多占用 3×3×3 个世界轴向的方块格。**
  按源坐标实际跨越的格子计算，不只看几何宽度；自动拆分不能绕过整体上限。
- `output.split: "none"`（默认）输出一个工程；`"blocks"` 输出
  `<name>_blocks/block_<x>_<y>_<z>.bbmodel` 与 `blocks.bbmanifest.json`。
  坐标沿游戏 X/Y/Z 轴，各局部分量为 0..2，保留空隙。
- 一个游戏方块对应 16 模型单位，**不是精度上限**；LT 高精度 grid 保留为小数坐标。
  `normalize_scale` 必须为 `false`。整体居中把占用格范围中心平移到 `[8,8,8]`；
  拆分工程保留格内位置。
- CLI 将染色后的第一帧 PNG 写入工程旁的 `<project>_textures/`，工程使用相对路径引用。
  迁移时需一起保留工程与贴图目录；跨贴图单元的盒子可能拆成多个可编辑元素，以保留重复采样。
- 输出静态模型资源，不自动生成多方块放置、碰撞或 MCreator 行为定义。

几何预检查在解析材质前返回 JSON 的 `valid` 与 `issues`。应检查 `valid`，不能只判断进程退出码；
它不验证材质是否齐全。

```sh
./LittleTilesReader --version
./LittleTilesReader --capabilities
./LittleTilesReader --inspect-snbt House.txt --inspect-format bbmodel
./LittleTilesReader --job examples/bbmodel.json --progress json
```

运行前修改 [examples/bbmodel.json](examples/bbmodel.json) 的输入和素材路径。
在 `output` 内增加 `"split": "blocks"` 即切换为拆分模式。完整参数见 [docs/job.md](docs/job.md)。
[BB 说明](docs/bbmodel-v1.md) 和[拆分说明](docs/bbmodel-blocks.md) 保留历史实现记录；
当前 CLI 使用上述外部 PNG 引用方式。

## 测试存档

历史验证使用的 region 存档（均为 Minecraft 1.12.2 + Little Tiles 1.5.66）**不随本库分发**：
它们和生成端脚本一起放在**测试数据包**里。下表路径相对测试数据根
（例如 `…/minecraft-littletiles-reader-data/data`），用时把绝对路径填进去即可。
运行时依次输入存档目录、区块坐标、扫描半径：

| 目录 | 区块坐标 (x, z) | 推荐范围 | 扫描规模 | 基线（Debug 构建，开启完整方块、剔除相邻面） |
|---|---|---|---|---|
| `data/regions/test_region/` | **0, 0** | **1** | 3×3 区块 | 236 个 tile → 4881 面 → 0.3 MB OBJ，约 0.1 s |
| `data/regions/test_region_medim/` | **-136, 49** | **5** | 11×11 区块 | 55561 个 tile → 388744 面 → 34 MB OBJ，约 9 s |
| `data/regions/test_region_large/` | **-7, -26** | **5** | 11×11 区块 | 324427 个 tile + 194 万普通方块 → 2036139 面 → 178 MB OBJ，约 50 s |

这些是历史核心库基线，不代表 Windows App 0.2.0 的性能验收结果。
测试数据侧的 `tools/benchmark.py` 会用上面这套参数把三个存档各跑一遍并记录结果，
历史记录见 [`docs/benchmark.md`](docs/benchmark.md)（脚本不在本仓库内）。

```sh
# 依次为：存档目录、区块 x、区块 z、半径、
# 完整方块、剔除相邻面、居中、单位化、进度与耗时
# （给的是结构文件 .txt/.struct 时，跳过坐标与半径那三问）
# <region 根> = 测试数据包里的 data/ 目录
printf "<region 根>/regions/test_region_large\n-7\n-26\n5\ny\ny\ny\nn\ny\n" | ./LittleTilesReader
```

结构跨版本互转（不给 `--out` 时输出写在输入文件旁边；job 文件写法见
[`examples/snbt_convert.json`](examples/snbt_convert.json)）：

```sh
# 1.12.2 结构 -> 1.20
./LittleTilesReader --convert 1.20 --input House.txt

# 1.20 蓝图 -> 1.12.2
./LittleTilesReader --convert 1.12.2 --input House.struct --out House_112.txt
```

界面的提示、进度与结果行**只输出英文**（避免终端编码导致的乱码）。所有可见文案都走
`galib::Tr()` 这一个入口，它现在是单参数的直通函数，源码里已不含中文——
要恢复多语言，只需改这一个函数。
最后一项同时控制**进度提示**（每处理一个区块打印 `[progress] chunk (x, z) - i/n`，
以及库里逐区块、逐方块的详细信息）和结尾的**总耗时**一行。
想要干净的、方便脚本处理的输出就答 `n`。

产物写在**当前工作目录**下的 `outputs/`（OBJ + MTL + 同名 `<obj 名>_textures/` 贴图目录）。

## 素材与资源包

核心通过 `assets_root`／job 的 `assets.package` 读取**准备好的素材目录**，不直接导入客户端 JAR、
模组或资源包；这些来源由配套 App／生成端解析并组合。App 先导入对应的原版客户端 JAR，
再添加可选模组和资源包，两个游戏版本的素材分别维护。

目录包含 `block_textures.tsv`、只读 `textures/`；现代素材还包含带版本的 `manifest.json`。
1.20.1 存档的带材质导出要求匹配 **1.20.1 的格式 v2** 素材，包含完整状态和逐面 UV 映射；
旧格式继续用于 1.12.2 流程。模组贴图需要有效映射，导入模组不会自动实现它的自定义模型渲染器。

可通过 job、环境变量 `LITTLETILES_ASSETS=<素材目录>`，或交互式 `assets root` 提示指定目录。
相对路径依次按当前工作目录、可执行文件目录及其上一级解析。
参见[素材契约](docs/assets-package.md)、[贴图与 UV](docs/texture-mapping.md)和
[动态贴图静态帧处理](docs/texture-animation-static-frame.md)。

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
ctest --test-dir cmake-build-debug --output-on-failure
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

- 存档配置仅限 **1.12.2 / 1.20.1**。不支持的 `DataVersion`、显式版本冲突及混合版本区块会失败。
  缺失 `DataVersion` 时只允许带警告的字段布局推断，不能验证精确版本或原生历史。
  现代高度从实际存储的 section 推导；MCA 区块压缩目前只接受 zlib（类型 2）。
- Windows 使用 `float`，macOS／Linux 使用 `double`（`CgalTypeDef.h`），跨平台几何可能略有差异。
- 立方体／交叉平面路径以外的普通方块属于近似，不还原任意模组模型、CTM 连接纹理、动态渲染、
  光影或游戏中的精确光照。
- 贴图保留透明度，实际显示取决于目标软件的材质／导入设置；植被和水的默认染色不等于
  原存档各生物群系颜色。
- 结构 SNBT 互转与存档迁移是两回事。SNBT 互转可能透传门动画／开关参数，但模型导出不会重现
  这些行为，详见 [SNBT 说明](docs/snbt-format.md)。
- BB 继续遵循标准盒子、3³ 格与禁止单位化的限制。几何预检查、工程生成和游戏内验收是不同步骤，
  自动测试样本不能覆盖所有模组组合。

## Matlab 支持

通过 LittleTile 模组导出的几何结构数据可以用生成端/测试数据包里的
`python/IntArrayInterpreter.py` 转换，以在 MatLab 中展示其几何结构。
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

工程笔记放在 [`docs/`](docs/)。部分文档保留带日期的计划或测试记录，
不能把历史设计全部视为已实现；当前能力以代码和本 README 为准。常用入口：

- [`docs/architecture.md`](docs/architecture.md) —— 模块划分、真实调用链、ownership 与线程安全现状、构建方式
- [`docs/nbt-format.md`](docs/nbt-format.md) —— `.mca` / chunk NBT / LittleTiles tile 格式、角度偏移位域、坐标系统
- [`docs/snbt-format.md`](docs/snbt-format.md) —— LittleTiles 结构 SNBT（两种方言）、盒子编码、子结构、模组贴图
- [`docs/known-issues.md`](docs/known-issues.md) —— 已核实缺陷、输出确定性、测试基线与 README/代码不一致清单
- [`docs/texture-mapping.md`](docs/texture-mapping.md) —— UV 约定、tint 烘焙、材质包叠加
- [`docs/benchmark.md`](docs/benchmark.md) —— 导出基线与实测数据（由测试数据侧的 `tools/benchmark.py` 生成）
- [`docs/reference-lt3d-importer.md`](docs/reference-lt3d-importer.md) —— 第三方模组 LT 3D Importer & Exporter 的研究（UV/纹理策略）

- [`docs/job.md`](docs/job.md) —— 非交互任务与 JSON 进度事件
- [`docs/build-guide.md`](docs/build-guide.md) —— 各平台构建
- [`docs/release-and-licensing.md`](docs/release-and-licensing.md) —— 发行与第三方许可要求
