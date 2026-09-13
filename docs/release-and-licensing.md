# 发布指南：第一版可执行程序（含依赖许可证分析）

> 生成时间：2026-09-14
> 适用仓库：`Gaksy/Minecraft-Littletiles-Reader`（默认分支 `v2ForLLM`）
> ⚠️ 本文的许可证部分是基于各依赖**自带的版权文件与源码 SPDX 头**整理的事实陈述，
> **不构成法律意见**。涉及商业分发请咨询律师。

---

## 0. 结论先行

1. **GitHub Releases 就是你要的功能；GitHub Packages 用不上。**（见 §1）
2. **但必须先解决许可证问题。** CGAL 的 `Surface_mesh` 包是 **GPL-3.0-or-later**，
   而且它是本项目的核心网格数据结构、**无法简单移除**。
   一旦对外分发二进制，**整个程序（包括你自己的 MIT 代码）必须按 GPL-3.0+ 分发**。（见 §3）
3. 所以建议的顺序是：**先决定 §3 走哪条路 → 再按 §4 打包 → 最后才点发布。**

---

## 1. Releases 与 Packages 是什么

| | **GitHub Releases** | **GitHub Packages** |
|---|---|---|
| 面向对象 | **人**：下载即用 | **机器**：包管理器自动拉取 |
| 载体 | Release 附件（任意二进制文件） | 包注册表条目 |
| 绑定 | Git tag（如 `v0.1.0`） | 包名 + 版本号 |
| 典型生态 | 所有语言通用 | npm / NuGet / Maven / Gradle / RubyGems / Docker(ghcr.io) |
| 下载门槛 | 公开仓库**免登录** | 通常需要 token 认证 |
| 容量 | 单个附件 ≤ 2 GiB，数量不限 | 视包类型而定 |
| **C++ 适用性** | ✅ 直接可用 | ⚠️ **没有官方 registry**；vcpkg / Conan 各有独立仓库，不接 GitHub Packages |
| **本项目结论** | **✅ 第一版 exe 走这里** | **❌ 不需要** |

### 还有一个容易混淆的第三个概念

**Actions Artifacts** —— CI 跑完后留在 workflow 运行记录里的中间产物。
它**不对外**、有过期时间、下载要登录。**不要**把它当 Releases 用。

> 顺带一提：`ghcr.io`（容器镜像仓库）属于 Packages 体系，但那是发 Docker 镜像的，
> 和桌面 CLI 的发布无关。

---

## 2. 依赖许可证清单（实测）

**数据来源**
- 版本：vcpkg 的 `installed/vcpkg/status`（本机 `D:\Development\DevLib\CorCpp\vcpkg`）
- 许可证：各包自带的 `share/<port>/copyright` 文件 + 源码文件头的 `SPDX-License-Identifier`

| 组件 | 实测版本 | 许可证 | 分发二进制时的义务 | 风险 |
|---|---|---|---|---|
| Boost (iostreams, json) | **1.89.0** | BSL-1.0 | 保留版权声明与许可文本 | 无 |
| zlib | **1.3.1** | Zlib | 保留声明；不得冒名 | 无 |
| libnbt++ | commit `687e4303`（**PrismLauncher fork**） | **LGPL-3.0-or-later** | 动态链接即可；提供许可文本 + 允许替换该 DLL + 指向源码 | 低（已是 DLL） |
| GMP | 6.3.0 | **LGPL-3.0+ / GPL-2.0+ 双许可** | 选 LGPL 分支，动态链接 | 低（已是 DLL） |
| MPFR | 4.2.2 | LGPL-3.0+ | 同上（**当前未被链接**，见 §4.3） | 低 |
| liblzma (xz) | 5.8.1 | **0BSD** | **无任何义务** | 无 |
| bzip2 | 1.0.8 | bzip2-1.0.6（BSD 类） | 保留声明 | 无 |
| zstd | 1.5.7 | **BSD-3-Clause / GPL-2.0 双许可** | 选 BSD 分支 | 无 |
| expat | 2.7.3 | MIT | 保留声明 | 无 |
| **CGAL `Kernel_23`**（EPICK / `Vector_3`） | 6.1 | **LGPL-3.0-or-later** | 同 LGPL；header-only 模板编译进二进制属静态链接，需提供可重链接方式 | 中 |
| **CGAL `Surface Mesh`** | 6.1 | **GPL-3.0-or-later** | **整个程序 GPL** | **🔴 高（无法移除）** |
| **CGAL `3D Point Set`**（`Point_set_3`） | 6.1 | **GPL-3.0-or-later** | 整个程序 GPL | 🟡 高但**可移除**（见 §3.3） |
| **CGAL `Polygon Mesh Processing` – Geometric Repair** | 6.1 | **GPL-3.0-or-later** | 整个程序 GPL | 🟡 高但**可移除**（见 §3.3） |
| MSVC 运行库 | VS 2026 / vc145 | Microsoft 可再发行条款 | 随包分发 DLL，或要求用户装 Redistributable | 中 |

### 2.1 怎么确认的（可复现）

CGAL 的许可是**逐包不同**的，官方原文（[doc.cgal.org/latest/Manual/license.html](https://doc.cgal.org/latest/Manual/license.html)）：

> *"the packages forming a foundation layer are distributed under the LGPL, and the
> higher level packages under the GPL."*
>
> *"The GNU GPL is an Open Source license that, if you distribute your software based on
> GPLed CGAL data structures, **obliges you to distribute the source code of your
> software under the GPL**."*

判定某个包是 LGPL 还是 GPL 的方法（本项目实测用过）：

1. 读包对应的 `include/CGAL/license/<Package>.h`，正文会明写
   `"You use the CGAL <X> package under the terms of the GPLv3+."`
2. 或者直接读头文件顶部的 `SPDX-License-Identifier:`。
3. **注意**：`CGAL/license/*.h` 这些文件自身的 SPDX 行写的是 LGPL，那是**这个生成文件自己的许可**，
   不代表它所描述的包。**以正文那句话为准。**

本项目实测结果：

```
CGAL/Exact_predicates_inexact_constructions_kernel.h  → LGPL-3.0-or-later   ✅
CGAL/Vector_3.h                                       → LGPL-3.0-or-later   ✅
CGAL/Surface_mesh/Surface_mesh.h                      → GPL-3.0-or-later    🔴
CGAL/Point_set_3.h                                    → GPL-3.0-or-later    🟡
CGAL/Polygon_mesh_processing/repair.h                 → GPL-3.0-or-later    🟡
```

---

## 3. GPL 问题的三条出路

**问题的本质**：你用的是 header-only 的 CGAL 模板。
模板在编译期被实例化、**直接编进你的 exe**——在许可证意义上等同于**静态链接**。
GPL-3.0 的传染性作用于"基于该程序的作品"，所以你的 exe 整体落入 GPL-3.0+。

（MIT 与 GPL 是兼容的，所以"MIT 源码 + GPL 二进制"本身合法；
但**对外分发的那个二进制，其许可就是 GPL-3.0+**，且你必须提供完整对应源码。）

### 出路 A：接受 GPL-3.0+（最省事，推荐）

- 仓库源码**可以继续是 MIT**（你拥有版权，可对下游授予额外许可），
  但**发布的二进制**按 GPL-3.0+ 分发。
- 你要做的事：
  1. 在仓库根加 `LICENSE-GPL-3.0`（GPL-3.0 全文）。
  2. README 的 License 段说明：**源码 MIT，分发的二进制因含 CGAL GPL 包而以 GPL-3.0+ 提供**。
  3. 发布页/`THIRD-PARTY-NOTICES.txt` 写明完整对应源码的位置
     （就是你自己的公开仓库 + 依赖的版本与来源）。因为仓库本来就公开且含全部源码，
     **这一条几乎零成本**。
- 代价：想在你的程序基础上做闭源二次开发的商业用户会被 GPL 挡住。

### 出路 B：买 CGAL 商业许可

- 从 [GeometryFactory](https://www.geometryfactory.com/) 购买对应包的商业许可，
  即可按 MIT（或任何你想要的许可）分发二进制。
- 适合：你打算把工具闭源卖钱，或要给别人提供非 GPL 的再许可。
- 代价：要花钱，且需按包购买。

### 出路 C：移除 GPL 包，只留 LGPL 层

**先说结论：走不通。** 原因见下。

| 包 | 实际使用情况 | 能否移除 |
|---|---|---|
| `3D Point Set` | `CgalTypeDef.h:21` 有 `#include <CGAL/Point_set_3.h>`，**但代码里一次都没用到** | ✅ **删掉这行 include 即可**，零功能影响 |
| `Polygon Mesh Processing` | 只在 `CgalLtSupport.cpp` 的 `CleanupMesh()` 里用了 3 次：<br>`remove_isolated_vertices()` ×2、`remove_degenerate_faces()` ×1 | ✅ **可手写替代**；而且**紧邻的下方就已经有等价的手写回退实现**（处理非三角网格的那段），照抄即可 |
| `Surface Mesh` | `SurfaceMeshType = CGAL::Surface_mesh<CGAL::Point_3<LtKernel>>`，是几何核心的数据结构，**全项目都在用** | ❌ **不可行** |

而且——`CGAL::Polyhedron_3`（唯一可能的替代网格结构）实测也是
`SPDX-License-Identifier: GPL-3.0-or-later`。**CGAL 里没有 LGPL 的网格数据结构。**

所以：即使把前两个包清干净，`Surface_mesh` 仍会把 GPL 传染性留在项目里。
**出路 C 只能降低 GPL 代码的"面积"，不能改变结论。**

> 不过删掉那两个多余的 GPL 包本身仍是值得做的清理（减少依赖、编译更快），
> 可以作为独立的小改动。

### 3.4 还有一条"绕开"的路（如果你只想自用）

**只发源码、不发二进制**，就完全不触发 GPL 的分发义务。
但你的场景是"别的程序通过命令行调用"，对方需要可执行文件——
除非你让每个使用者自己按 README 编译，否则这条路不适用。

---

## 4. 打包操作清单

### 4.0 前置：先补版本号

`CMakeLists.txt` 当前是：

```cmake
project(LittleTilesReader)
```

改成：

```cmake
project(LittleTilesReader VERSION 0.1.0 LANGUAGES CXX)
```

> 不补的话，exe 的"属性 → 详细信息"里看不到版本，发布页也不好对齐。

### 4.1 把 Release 构建目录加进 `.gitignore`

`.gitignore` 目前只忽略了 `cmake-build-debug/`、`cmake-build-visual-studio/`、
`cmake-build-msbuild/`，**没有 `cmake-build-release/`**。先补上，否则构建产物会污染 `git status`。

### 4.2 做 Release 构建

本机工具链（实测）：
- VS 2026：`A:\Application\VisualStudio\2026`（vc145 工具集）
- vcpkg：`D:\Development\DevLib\CorCpp\vcpkg`（triplet `x64-windows`，动态链接）

从 Git Bash 直接跑 `cl.exe` 会因缺 `INCLUDE`/`LIB` 报 `C1083: cstdint`，
必须**先套一层 vcvars**：

```bash
cmd /c "call A:\Application\VisualStudio\2026\VC\Auxiliary\Build\vcvars64.bat && \
        cmake -S . -B cmake-build-release -G Ninja -DCMAKE_BUILD_TYPE=Release && \
        cmake --build cmake-build-release"
```

### 4.3 核对真实依赖（重要，别跳过）

vcpkg 的 applocal 机制会把用到的 DLL 自动拷到 exe 旁边，但**要确认它拷全了**：

```bash
cmd /c "call A:\Application\VisualStudio\2026\VC\Auxiliary\Build\vcvars64.bat && \
        dumpbin /dependents cmake-build-release\LittleTilesReader.exe"
```

逐条对照 `cmake-build-release\*.dll`，缺哪个从
`D:\Development\DevLib\CorCpp\vcpkg\installed\x64-windows\bin\` 补。

> Debug 版实测部署了 8 个 DLL：
> `boost_iostreams-*.dll`、`boost_json-*.dll`、`zlibd1.dll`、`nbt++.dll`、
> `gmp-10.dll`、`bz2d.dll`、`liblzma.dll`、`zstd.dll`。
> Release 版名字会变（去掉 `d` 后缀，如 `zlib1.dll`），**以 dumpbin 结果为准**。
> 顺便也能确认 MPFR 到底有没有被链接（Debug 版里没见到 `mpfr-*.dll`）。

### 4.4 组装发布目录

```
dist/LittleTilesReader-0.1.0-win-x64/
├── LittleTilesReader.exe
├── <全部运行时 DLL>
├── LICENSE                    ← 你原有的 MIT
├── LICENSE-GPL-3.0            ← 若走出路 A
├── THIRD-PARTY-NOTICES.txt    ← 见 §5
└── README.md                  ← 至少要有"怎么运行"
```

### 4.5 压缩

```bash
powershell -NoProfile -Command "Compress-Archive -Path 'dist/LittleTilesReader-0.1.0-win-x64' -DestinationPath 'dist/LittleTilesReader-0.1.0-win-x64.zip' -Force"
```

### 4.6 冒烟测试（发布前必做）

**在一台没装 vcpkg、没装 Visual Studio 的机器**（或干净的 Windows 沙箱 / 虚拟机）上：
1. 解压 zip 到任意目录
2. 双击 `LittleTilesReader.exe`
3. 用 `data/regions/test_region` 跑一遍，确认输出与
   `docs/benchmark.md` 里的基线一致

这一步能一次性抓出：漏拷 DLL、缺 VC++ 运行库、路径假设写死在开发机上 三类问题。

### 4.7 打 tag

```bash
git tag -a v0.1.0 -m "LittleTilesReader 0.1.0 - first public release"
git push origin v0.1.0
```

> 注意：**默认分支是 `v2ForLLM`**。tag 打在哪个 commit 上要想清楚。

### 4.8 创建 GitHub Release

**方式一：网页（第一次发布建议用这个，最直观）**

打开 <https://github.com/Gaksy/Minecraft-Littletiles-Reader/releases/new>
- *Choose a tag* → 选 `v0.1.0`
- *Release title* → `v0.1.0 - first public release`
- 描述区粘贴 §6 的模板
- 把 `LittleTilesReader-0.1.0-win-x64.zip` 拖到 *Attach binaries* 区
- 确认勾选 **Set as the latest release**

**方式二：命令行（本机当前没装 `gh`）**

```bash
winget install --id GitHub.cli
gh auth login
gh release create v0.1.0 \
  dist/LittleTilesReader-0.1.0-win-x64.zip \
  --title "v0.1.0 - first public release" \
  --notes-file RELEASE_NOTES.md
```

### 4.9 后续：自动化（第二版再做）

可以加 `.github/workflows/release.yml`，`push tag` 时自动在 `windows-latest` 上
构建 + 打包 + 建 Release。**但有个坑**：CI 上要重新装 vcpkg 依赖，
**CGAL 编译非常慢**（十几到几十分钟），必须配 vcpkg 二进制缓存，否则每次跑都等到崩。

---

## 5. `THIRD-PARTY-NOTICES.txt` 模板

```
LittleTilesReader
Copyright (c) 2024-2026 Gaksy

This distribution includes the following third-party components.
Each is provided under its own license, reproduced or referenced below.

------------------------------------------------------------------------
Boost 1.89.0 (Boost.Iostreams, Boost.JSON)
License: Boost Software License 1.0
https://www.boost.org/LICENSE_1_0.txt
------------------------------------------------------------------------
zlib 1.3.1
License: zlib License
https://zlib.net/zlib_license.html
------------------------------------------------------------------------
libnbt++ (commit 687e43031df0dc641984b4256bcca50d5b3f7de3)
Source: https://github.com/PrismLauncher/libnbtplusplus
License: GNU Lesser General Public License v3.0 or later

  This program is distributed with libnbt++ as a separate shared library
  (nbt++.dll). In accordance with LGPL-3.0, you may replace that DLL with
  a modified version. The complete corresponding source of libnbt++ is
  available at the URL above.
------------------------------------------------------------------------
GMP 6.3.0
License: GNU Lesser General Public License v3.0 or later
         (dual-licensed with GPL-2.0+; LGPL is used here)
https://gmplib.org/manual/Copying
------------------------------------------------------------------------
CGAL 6.1
License: GNU General Public License v3.0 or later, or a commercial
         license from GeometryFactory.

  CGAL packages used by this program, and their licenses:
    - Kernel_23 .......................... LGPL-3.0-or-later
    - Surface Mesh ....................... GPL-3.0-or-later
    - 3D Point Set ....................... GPL-3.0-or-later
    - Polygon Mesh Processing (Repair) ... GPL-3.0-or-later

  Because GPL-licensed CGAL packages are compiled into this binary,
  this binary as a whole is distributed under the GNU GPL v3.0 or later.
  The complete corresponding source code is available at:
  https://github.com/Gaksy/Minecraft-Littletiles-Reader
------------------------------------------------------------------------
bzip2 1.0.8
License: bzip2-1.0.6 (BSD-like)
https://sourceware.org/bzip2/
------------------------------------------------------------------------
liblzma / XZ Utils 5.8.1
License: BSD Zero Clause License (0BSD)
------------------------------------------------------------------------
Zstandard 1.5.7
License: BSD 3-Clause (dual-licensed with GPL-2.0; BSD is used here)
https://github.com/facebook/zstd/blob/dev/LICENSE
------------------------------------------------------------------------
Expat 2.7.3
License: MIT
------------------------------------------------------------------------
Microsoft Visual C++ Runtime
Distributed under the Microsoft Visual C++ Redistributable terms.
https://learn.microsoft.com/cpp/windows/latest-supported-vc-redist
------------------------------------------------------------------------
```

> 若走**出路 A**，再补一个 `LICENSE-GPL-3.0`（GPL-3.0 全文）。
> 若走**出路 B**（买了商业许可），把 CGAL 那一段换成商业许可说明。

---

## 6. 发布说明（Release notes）模板

```markdown
## LittleTilesReader 0.1.0

First public release. Parses Minecraft Anvil (`.mca`) save files and
LittleTiles structure files (SNBT), and rebuilds the tile geometry as an
OBJ model (OBJ + MTL + baked textures) that opens directly in Blender.

### Downloads

- `LittleTilesReader-0.1.0-win-x64.zip` — Windows 64-bit CLI

### Requirements

- Windows 10/11 x64
- Microsoft Visual C++ Redistributable (x64) — see the link below
- No vcpkg, no compiler, and no Python needed

### Quick start

1. Unzip anywhere.
2. Run `LittleTilesReader.exe`.
3. Answer the prompts: region folder (or a `.txt` / `.struct` file),
   chunk x, chunk z, scan radius.
4. The result is written to `outputs/` next to your working directory.

Point the `assets root` prompt at an assets package to get textures;
otherwise geometry is exported without textures.

### Verified against

- Minecraft 1.12.2 + Little Tiles 1.5.66

### Known limitations

- Only the 1.12 save layout is supported (Sections under Level, zlib).
- Connected textures (MCPatcher/CTM) are not supported yet.
- `FloatType` differs between Windows (float) and macOS (double), so
  geometry may differ slightly across platforms.

### Licensing

Source code is MIT. **The binary in this release is distributed under
GPL-3.0-or-later**, because it links GPL-licensed CGAL packages
(Surface Mesh, 3D Point Set, Polygon Mesh Processing).
See `THIRD-PARTY-NOTICES.txt` inside the zip for details.
```

---

## 7. 顺带发现的问题（建议一并修掉）

### 7.1 README 的依赖版本表与实际不符

`README.md` 的 *Library Dependencies* 表：

| 库 | README 写的 | vcpkg 实测 |
|---|---|---|
| boost | 1.92.0（要求 ≥ 1.74） | **1.89.0** |
| zlib | 1.3.2 | **1.3.1** |
| CGAL | 6.2.1 | **6.1** |
| libnbt++ | commit `687e4303` | ✅ 一致 |

另外 `docs/known-issues.md` 的定位是"记录 README 与代码不符"，
这条可以记进去。

### 7.2 两个可以删掉的 GPL 依赖

即使不解决 `Surface_mesh`，下面两处清理也是独立有价值的：

1. `Galib/include/Minecraft/CgalSupport/CgalTypeDef.h:21` 的
   `#include <CGAL/Point_set_3.h>` —— **纯多余，代码里没用过**，直接删。
2. `Galib/Minecraft/CgalSupport/CgalLtSupport.cpp` 的 `CleanupMesh()` 里
   3 处 `CGAL::Polygon_mesh_processing::*` 调用 —— 用紧邻的手写实现替代。

收益：少两个 GPL 包、编译更快、依赖面更小。**但不会改变 §3 的结论。**

### 7.3 MSVC 运行库是个真实风险

VS 2026 / vc145 工具集很新。用户机器上装的 Redistributable 很可能**版本不够**，
导致双击报"找不到 VCRUNTIME140_1.dll"之类。

两个选择：
- **简单**：在发布说明里明确给出 Redistributable 下载链接，让用户自己装。
- **稳妥**：给 exe 加 `/MT`（静态链接 CRT），这样只依赖第三方 DLL，不依赖 VC++ 运行库。
  代价是 exe 变大，且需要改 CMake 配置。

> 注意：`/MT` 只解决 **CRT** 的问题，**不影响 §3 的 GPL 结论**。

---

## 8. 执行顺序速查

| 步骤 | 内容 | 关键点 |
|---|---|---|
| 1 | **决定 §3 走哪条路** | 不决定就别往下走 |
| 2 | 补 `project(... VERSION 0.1.0)` | §4.0 |
| 3 | `.gitignore` 加 `cmake-build-release/` | §4.1 |
| 4 | vcvars + Release 构建 | §4.2，必须套 vcvars |
| 5 | `dumpbin /dependents` 核对 DLL | §4.3，别跳过 |
| 6 | 组装目录 + 写 THIRD-PARTY-NOTICES | §4.4 / §5 |
| 7 | 压缩 zip | §4.5 |
| 8 | **在干净机器上冒烟测试** | §4.6，最容易被跳过也最该做 |
| 9 | `git tag -a v0.1.0` + push | §4.7 |
| 10 | 建 GitHub Release + 上传 zip | §4.8 |
