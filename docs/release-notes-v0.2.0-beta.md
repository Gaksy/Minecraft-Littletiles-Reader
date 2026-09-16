# LittleTilesReader 0.2.0-beta

把 Minecraft 存档（Anvil `.mca`）或 LittleTiles 的 SNBT 结构，在**游戏外**导出成标准 **OBJ + MTL + 纹理**，
可以直接丢进 Blender 等建模软件。面向大型结构的分享与专业渲染流程 —— 不用在游戏里等着导出。

> 这是**测试版**（beta）：接口与输出格式已稳定，但仍在跟着社区反馈改。
> 当前只支持 **Minecraft 1.12.2（Forge + LittleTiles 1.5.66）**。

## 下载

| 平台 | 文件 | 大小 | sha256 |
|---|---|---|---|
| macOS（Apple Silicon） | `LittleTilesReader-0.2.0-beta-macos-arm64.tar.gz` | 455,650 B | `f3bff53d9b049379c84d33534654cd081d11f2e2bf39f2ec5b4de324a5f226bb` |
| Linux（x86_64） | `LittleTilesReader-0.2.0-beta-linux-x86_64.tar.gz` | 384,739 B | `d036673165c34c5db33b77bc4fe831577526801995253aa6b09e9dea0e891d43` |
| Windows（x64） | `LittleTilesReader-0.2.0-beta-windows-x64.zip` | 641,876 B | `e087ff83046e4bfb53ee971fba08bac2fd11fa8071ded7b6b663a5909fd0fdd1` |

附件里还有 `SHA256SUMS.txt`（含包内每个文件的哈希），校验：

```sh
shasum -a 256 -c SHA256SUMS.txt          # macOS / Linux
Get-FileHash .\LittleTilesReader-0.2.0-beta-windows-x64.zip -Algorithm SHA256   # Windows
```

## 包里有什么

解压即用，不需要装运行库：

| 平台 | 可执行文件 | 附带运行库 | 说明 |
|---|---|---|---|
| macOS | `LittleTilesReader` | `libnbt++.dylib` | arm64，ad-hoc 签名；首次打开被 Gatekeeper 拦时右键「打开」，或自己 `codesign -s -` |
| Linux | `LittleTilesReader` | `libnbt++.so` | x86_64，OpenCloudOS 9.6 构建，其余依赖静态链接 |
| Windows | `LittleTilesReader.exe` | `nbt++.dll` | x64，**静态 CRT（/MT）**，`dumpbin` 确认只依赖 `nbt++.dll` 与系统 DLL，**不需要 VC++ 运行库** |

三个包都带 `LICENSE`、`LICENSE-GPL-3.0`、`THIRD-PARTY-NOTICES.txt`、`README.md`。

> ⚠️ 许可证：本程序链接了 **CGAL**（GPL-3.0-or-later），因此**二进制以 GPL-3.0-or-later 分发**；
> Boost / zlib / libnbt++ 等其它依赖的许可见 `THIRD-PARTY-NOTICES.txt`。

## 这个版本能做什么

- 读 `.mca` 区域文件，按区块坐标 + 半径导出；也能直接读 LittleTiles 导出的 **SNBT**
- 原版方块与 LittleTiles 结构一起导出，几何保留 **n 边形**（不做三角扇）
- 支持导入资源包（材质包）取纹理，材质文件名不冲突时同名复用
- 输出 OBJ + MTL + 纹理目录，可直接在 Blender 打开；可选居中 / 归一化 / 隐藏面剔除

三平台的几何结果**逐项一致**（同一份测试数据）：

| 测试 | 结果 |
|---|---|
| 存档导出（`base` 存档 chunk 0,0 半径 1） | 12,975 顶点 / 4,940 面 / 0 缺贴图 —— macOS 0.03 s、Windows 0.083 s（峰值 14.9 MB） |
| SNBT 导出（CoronaSign） | 1,114 meshes —— macOS 0.026 s、Linux 0.026 s、Windows 0.110 s（峰值 10.5 MB） |
| SNBT 导出（大结构 SHB） | 18,044 meshes —— Linux 0.374 s（峰值 52 MB）、Windows 1.095 s（峰值 76.3 MB） |

## 已知限制

- 只支持 **1.12.2**；模组方块的支持还在做，目前以原版方块 + LittleTiles 结构为主
- 没有图形界面，只有命令行（`--job job.json` / `--progress json`）
- Windows 可执行文件里含有**开发机路径字符串**（MSVC 断言用的 `__FILE__`，以及 vcpkg 预编译库里打包的路径）。
  它们只是调试信息，程序运行时不读这些路径；会在后续版本用 `/pathmap` 清掉。
- 仓库根 `LICENSE`（MIT）与源文件头（LGPL-3.0）的历史不一致仍在处理，二进制分发按 GPL-3.0-or-later 执行。

## 反馈

- 问题与建议：<https://www.inception.work/feedback?module=littletiles>
- 使用说明与下载页：<https://www.inception.work/littletiles>
