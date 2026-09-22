# LittleTilesReader 0.3.0-beta

把 Minecraft 存档（Anvil `.mca`）或 LittleTiles 的 SNBT 结构，在**游戏外**导出成标准 **OBJ + MTL + 纹理**，
可以直接丢进 Blender 等建模软件。面向大型结构的分享与专业渲染流程 —— 不用在游戏里等着导出。

> 这是**测试版**（beta）：接口与输出格式已稳定，但仍在跟着社区反馈改。
> 0.3.0 起同时支持 **Minecraft 1.12.2**（Forge + LittleTiles 1.5.66）与
> **1.18+/1.19/1.20**（含 1.20.1 Forge + LittleTiles）两代存档与结构格式。

> ⚠️ **本文件是待发草稿，还不能直接发**：macOS arm64 包尚未构建（见下面「下载」一节的说明）。
> 发布前请补齐 macOS 行、按磁盘重算 `SHA256SUMS.txt`，并确认 tag 指向的提交。

## 下载

| 平台 | 文件 | 大小 | sha256 |
|---|---|---|---|
| Linux（x86_64） | `LittleTilesReader-0.3.0-beta-linux-x86_64.tar.gz` | 463,099 B | `9b88e6aba991d85477357baaa24aff206487f416fe93d7867d0377b5cddcf58d` |
| Windows（x64） | `LittleTilesReader-0.3.0-beta-windows-x64.zip` | 709,024 B | `9f9ec95743d01030f2ad15020acae05e132622cbc12951b7ad0a660f38110324` |
| macOS（Apple Silicon） | 待构建 | — | — |

> macOS 包**本轮未构建**：Mac 侧构建一直由 Mac 本机手工完成（`docs/build-guide.md` §5），
> 本轮没有可用的 Mac 构建环境。补上之后这一行要按**实测**的字节数与哈希填写，
> 并把 macOS 的冒烟数据补进 `docs/benchmark.md` 的 0.3.0 一节。

附件里还有 `SHA256SUMS.txt`（含包内每个文件的哈希），校验：

```sh
shasum -a 256 -c SHA256SUMS.txt          # macOS / Linux
Get-FileHash .\LittleTilesReader-0.3.0-beta-windows-x64.zip -Algorithm SHA256   # Windows
```

## 包里有什么

解压即用，两个平台都不需要另装运行库：

| 平台 | 可执行文件 | 附带运行库 | 说明 |
|---|---|---|---|
| Linux | `LittleTilesReader` | `libnbt++.so` | x86_64，OpenCloudOS 9.6 构建。**Boost / zlib / C++ 运行库是动态链接的系统库**，需要同代发行版（见下） |
| Windows | `LittleTilesReader.exe` | `nbt++.dll` | x64，**静态 CRT（/MT）**，`dumpbin` 确认只依赖 `nbt++.dll` 与系统 DLL，**不需要 VC++ 运行库** |

> **Linux 运行环境要求（这一条 0.2.0 写错过，别照抄旧版）**：
> 二进制链接的是系统共享库，`ldd` 闭包为
> `libboost_iostreams.so.1.82.0`、`libboost_json.so.1.82.0`、`libboost_container.so.1.82.0`、
> `libz.so.1`、`libbz2.so.1`、`liblzma.so.5`、`libstdc++.so.6`、`libm.so.6`、`libgcc_s.so.1`、
> `libc.so.6`，符号版本下限 **GLIBC_2.38 / GLIBCXX_3.4.29**。
> 即 **OpenCloudOS 9 / Rocky Linux 9 / AlmaLinux 9** 同代发行版可用；
> RHEL 8、Debian 12、Ubuntu 22.04 **不行**。准确清单与验证方法见包内 `THIRD-PARTY-NOTICES.txt`。

两个包都带 `LICENSE`、`LICENSE-GPL-3.0`、`THIRD-PARTY-NOTICES.txt`、`README.md`。

> ⚠️ 许可证：本程序链接了 **CGAL**（GPL-3.0-or-later），因此**二进制以 GPL-3.0-or-later 分发**；
> Boost / zlib / libnbt++ 等其它依赖的许可见 `THIRD-PARTY-NOTICES.txt`。

## 0.3.0 新增了什么

- **两种 SNBT 方言都能读**：1.12.2 的 `tiles` 列表，以及 1.16+/1.19+/1.20 的 `t` 映射
  （扁平化方块名 + block state）。
- **方言互转**：`--convert 1.20 --input House.txt`（或 `--convert 1.12.2`，或 job 文件的
  `"mode": "snbt_convert"` + `"options": {"target": "1.20"}`）可以把结构改写成另一代的格式。
  方块名走的是 LittleTiles 自己加载旧结构时用的那张对照表；找不到对应的名字会在警告里逐条列出。
- **1.18+ / 1.20 存档布局**：支持扁平化的区块格式（`sections[].block_states` 调色板与
  `block_entities`）以及更高的世界高度（y −64..319），与 1.12.2 的旧布局并存。
- **贴图名跨版本兜底**：只差一次 1.12/1.13 改名的方块
  （`minecraft:polished_granite` ↔ `minecraft:stone:2`、`light_gray_*` ↔ `silver_*`、
  `*_terracotta` ↔ `*_stained_hardened_clay` 等）现在会解析到同一张贴图，
  所以 1.20 的模型配旧命名素材包不会再是白模。

## 几何结果（实测）

同一份测试数据下 **Windows 与 Linux 逐项一致**（`docs/benchmark.md` 的 0.3.0 一节有完整表格）：

| 测试 | 结果 | 耗时 |
|---|---|---|
| 存档导出（`base` 存档 chunk 0,0 半径 1） | 9 区块 / 236 tile / **12,975 顶点 / 4,940 面** / 6 材质 / 0 缺贴图 | Linux 0.048 s（峰值 15.6 MB）、Windows 0.230 s |
| SNBT 导出（CoronaSign） | **1,114 meshes** | Linux 0.029 s（峰值 8.1 MB）、Windows 0.148 s |
| SNBT 导出（大结构 SHB） | **18,044 meshes** | Linux 0.338 s（峰值 54.5 MB）、Windows 1.144 s |
| SNBT 互转（大结构 SHB，1.12.2 → 1.20） | 18,044 box / 142 tile / 47 level | Linux 0.044 s（峰值 38.6 MB）、Windows 0.388 s |

这几个数字与 0.2.0 **完全相同**，所以 0.3.0 对核心几何代码的改动没有造成回退；
两个平台的 region OBJ 文件逐字节相同，两个平台转出的 1.20 SNBT 也逐字节相同。

另外专门验证过 0.3.0 的新能力：同一座建筑分别放在 1.12.2 与 1.20.1 两个存档的同一区块里，
只导 LittleTiles 时两边**逐字节一致**（164 tile / 1,355 顶点 / 999 面 / 3 材质 / 0 缺贴图）；
转出的 1.20 结构被同一个二进制读回来仍是 18,044 meshes。

## 已知限制

- 支持 1.12.2 与 1.18+/1.20 的存档布局；其它版本没有验证过。
- LittleTiles 侧验证过的版本是 1.5.66（1.12.2）与配套的 1.20 结构方言。
  区域文件仍要求 zlib 压缩，并且不做 `DataVersion` 检查。
- 结构互转时**组的顺序可能变**、显式 `color:-1` 会归一化成「无颜色」、
  1.12.2 的 `tID` 类私有键在 1.20 无处安放 —— 都会在转换时逐条报告，
  详见仓库 `docs/snbt-format.md` §5.1。**跨代转换的结果需要自己看一眼再用。**
- 高版本转低版本时，部分方块在低版本不存在，会被替换成替代方块。
- `FloatType` 在 Linux / macOS 上是 `double`、在 Windows 上是 `float`，
  所以几何可能在末几位小数上跨平台不同；顶点数、面数、材质与包围盒是一致的。
- Windows 可执行文件里仍含有**开发机路径字符串**（MSVC 断言用的 `__FILE__`，
  以及 vcpkg 预编译库里打包的路径）。它们只是调试信息，程序运行时不读这些路径。
- 没有图形界面，只有命令行（`--job job.json` / `--progress json` / `--convert`）。

## 反馈

- 问题与建议：<https://www.inception.work/feedback?module=littletiles>
- 使用说明与下载页：<https://www.inception.work/littletiles>

## 源码与许可

二进制按 GPL-3.0-or-later 分发，对应源码随时可取：

- 核心库（本仓库）：<https://github.com/Gaksy/Minecraft-Littletiles-Reader>（分支 `v2ForLLM`，tag `v0.3.0-beta`）
- 桌面客户端：<https://github.com/Gaksy/Minecraft-Littletiles-Reader-App>
- 第三方依赖清单与许可全文：包内 `THIRD-PARTY-NOTICES.txt`、`LICENSE-GPL-3.0`

> 源码基线：Windows 与 Linux 两个包都从 `v2ForLLM` 的同一份 0.3.0 源码构建，
> **没有** 0.2.0 那种 Linux 平台补丁分支。本 tag `v0.3.0-beta` 指向该提交。
> （草稿待办：提交 0.3.0 工作区改动后，把这里和 tag 一起改成本次的真实提交号。）
