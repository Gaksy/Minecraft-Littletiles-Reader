# 许可证与发布：结论摘要

> 生成时间：2026-09-14
> 详细版见 [`docs/release-and-licensing.md`](release-and-licensing.md)
> ⚠️ 本文是**事实陈述，不构成法律意见**。涉及商业分发请咨询律师。

---

## 一、一句话结论

**项目可以正常开源发布，不必因许可证问题中断。**

二进制整体按 **GPL-3.0+** 分发（即「出路 A」），义务已被公开仓库天然满足大半，
剩下的是**清单式执行工作**（补许可文本、写声明、提供源码包）。

---

## 二、为什么必须接受 GPL

CGAL 的许可是**逐包不同**的，而本项目用到了其中的 GPL 包：

| CGAL 包 | 用途 | 许可 | 能否移除 |
|---|---|---|---|
| `Kernel_23`（EPICK / `Vector_3`） | 基础 kernel | LGPL-3.0+ | — |
| **`Surface Mesh`** | **核心网格结构 `SurfaceMeshType`** | **GPL-3.0+** | ❌ **不可行** |
| `3D Point Set`（`Point_set_3`） | 从未使用 | GPL-3.0+ | ✅ 删多余 include |
| `PMP – Geometric Repair` | `CleanupMesh()` 里 3 处调用 | GPL-3.0+ | ✅ 可手写替代 |

**关键点**：

- `Surface_mesh` 是贯穿整个几何层的核心数据结构，**换不掉**。
- 唯一的替代品 `CGAL::Polyhedron_3` **也是 GPL** → **CGAL 里没有 LGPL 的网格数据结构**。
- 所以「移除 GPL 包」这条路走不通（出路 C 不成立）。
- CGAL 官方 FAQ 原话：*"If you distribute your software based on **GPLed CGAL data
  structures**, you are obliged to distribute ... the source code of your own software
  under the GPL."* —— **触发点是"你用了它"，与链接方式无关。**

**三条出路的选择**：A 接受 GPL ✅（已选）／B 买商业许可（闭源场景才需要）／C 移除 GPL 包 ❌。

> 选 A 的理由：本项目**长期开源、公开**。A 的代价（挡住闭源二次开发）不构成代价，
> 而 A 的义务（提供完整对应源码）已被公开仓库天然满足。

---

## 三、走 A 路线必须做的 6 件事

| # | 动作 | 依据 |
|---|---|---|
| 1 | 仓库根加 **`LICENSE-GPL-3.0`**（GPL-3.0 全文） | GPL-3.0 §4/§5 |
| 2 | 加 **`LICENSE-LGPL-3.0`**（LGPL-3.0 全文 **+ GPL-3.0 全文**） | LGPL-3.0 §3(b)/§4(b) |
| 3 | 显著声明「本程序使用 CGAL，整体按 GPL-3.0+ 分发」 | GPL-3.0 §5 |
| 4 | 提供 **Corresponding Source**（含 `CMakeLists.txt` 等构建脚本） | GPL-3.0 §6 |
| 5 | 说明「源码 MIT / 二进制 GPL-3.0+」的关系 | — |
| 6 | **不得添加与 GPL 冲突的附加限制** | GPL-3.0 §10 |

**第 2 条最易漏**：LGPL-3.0 **不是一份完整许可**，它靠引用把 GPL-3.0 纳入进来，
所以两份文本都得给。

**第 5 条不是"冲突"**：**MIT → GPL 是单向兼容的**，MIT 代码可以放进 GPL 作品再分发，
反过来不行。所以「源码 MIT / 二进制 GPL」合法且自洽，唯一要求是**说清楚**，
别让根目录那份只写 MIT 的 `LICENSE` 误导人。

**第 6 条的落地黑名单**（GPL-3.0 §10 禁止对下游施加额外限制）：

| ❌ 不能写 | 为什么 |
|---|---|
| 禁止商业使用 | GPL **允许**商业分发 |
| 禁止再分发 | 再分发是 GPL 的核心权利 |
| 必须署名 / 加 LOGO | 超出 GPL 要求的额外条件 |
| 下载前必须接受 EULA | 构成附加限制 |
| 源码仅供阅读、禁止修改 | GPL 明确允许修改 |

✅ **可以做的**：对二进制**收费**（GPL 允许卖钱）、提供付费支持/担保、要求保留版权声明。

---

## 四、依赖许可总表（实测版本）

| 组件 | 版本 / 来源 | 许可 | 分发义务 | 风险 |
|---|---|---|---|---|
| **CGAL `Surface Mesh`** | 6.1 | **GPL-3.0+** | 整个程序 GPL | 🔴 高（不可移除） |
| CGAL `PMP – Geometric Repair` | 6.1 | GPL-3.0+ | 整个程序 GPL | 🟡 可移除 |
| CGAL `3D Point Set` | 6.1 | GPL-3.0+ | 整个程序 GPL | 🟡 未使用，可移除 |
| CGAL `Kernel_23` | 6.1 | LGPL-3.0+ | 附文本 + 允许替换 + 指向源码 | 中 |
| **libnbt++** | commit `687e4303`（**PrismLauncher fork**） | **LGPL-3.0+** | 同上；已是 `nbt++.dll` | 低 |
| LittleTiles `1.12.2.txt`（方块映射表，内嵌源码） | 分支 `1.20`，commit `c5694e1` | **LGPL-3.0+**（与本库相同） | 保留声明与署名 | 无 |
| GMP | 6.3.0 | LGPL-3.0+ / GPL-2.0+ 双许可 | 选 LGPL 分支 + 动态链接 | 低 |
| MPFR | 4.2.2 | LGPL-3.0+ | **当前未被链接** | 低 |
| Boost | 1.89.0 | BSL-1.0 | 保留版权声明 | 低 |
| zlib | 1.3.1 | Zlib | 保留声明 | 低 |
| bzip2 | 1.0.8 | BSD 类 | 保留声明 | 低 |
| liblzma | 5.8.1 | **0BSD** | **零义务** | 无 |
| zstd | 1.5.7 | BSD-3（双许可选 BSD） | 保留声明 | 低 |
| expat | 2.7.3 | MIT | 保留声明 | 低 |

**✅ 全树零 AGPL**（已实测，见 §五.4）。

**判定方法备忘**：
- CGAL 逐包许可 → 读 `include/CGAL/license/<Package>.h` **正文**那句
  「under the terms of the GPLv3+」。⚠️ 该文件**自身**的 SPDX 行写 LGPL，
  不代表它描述的包。
- 依赖版本一律以 `<vcpkg>/installed/vcpkg/status` 为准，**README 的版本表不准**
  （boost 写 1.92.0 实为 1.89.0、zlib 写 1.3.2 实为 1.3.1、CGAL 写 6.2.1 实为 6.1）。

---

## 五、常见误解澄清

### 1. 「换成动态链接库就能躲开 GPL」——❌ 不能

**GPL 里没有"动态链接豁免"这一条。** 那条豁免是 **LGPL 独有**的
（LGPL-3.0 §4(d)(1)）。函数调用进一个 GPL 的 DLL **仍然构成合并作品**。

| 许可类型 | 动态链接的效力 |
|---|---|
| 宽松（MIT/BSD/Zlib/BSL/0BSD） | 无所谓，本来就不传染 |
| LGPL | ✅ **有效豁免**，你的代码可闭源 |
| **GPL** | ❌ **无效**，仍然传染整个程序 |

而且对 CGAL 来说还有个更前置的障碍：**CGAL 5.0+ 是 header-only**，
模板在调用点实例化进你的 `.obj`，**根本没有 DLL 可动态链接**。

> 唯一结构性隔离是**独立进程 + 通用接口**（管道/CLI/socket），
> 但 `Surface_mesh` 是贯穿几何层的核心数据结构 → 拆进程 = 重写架构 +
> 把最值钱的部分开源，只保住外层 CLI 壳，**收益为负**。

**话术**：**动态链接是"打包维度"的选择，不是"许可维度"的解药。**

### 2. 「header-only 库不用管许可」——❌ 错

header-only 模板编进 exe **≈ 静态链接**。CGAL / nlohmann-json / Eigen 都属此类。

两个值得知道的**灰区**（在出路 A 下全部失效）：
- **LGPL-3.0 §3**（目标代码含库头文件材料）允许"按你选择的条款"分发，
  但要求显著标注 + 附 GPL/LGPL 全文；它**是否同时免除 §4(d) 的重链接义务，
  法律界有争议**（FSF 倾向未免除）。
- **LGPL-3.0 §4(d)(1)** 要求共享库是"用户机器上**已有**的副本"，
  严格读**不包括你自己塞进 zip 的那个 DLL**（libstdc++ 争议源头）。

### 3. 「GPL 等于非商业」——❌ 错

**GPL 完全允许商业分发，你可以对二进制收费。** 它只管你不能给下游加绳子。
「非商业」从来不是合规的判定标准。

### 4. 「做网站服务要开源整个后端」——⚠️ 结论对，但理由要说准

| 场景 | 性质 | GPL 义务 |
|---|---|---|
| 下载页提供 exe 下载 | **分发** | ✅ 全触发 |
| 用户上传存档 → 服务器跑 reader → 返回 OBJ | **使用** | ❌ **不触发** |

**原理**：**GPL-3.0 没有网络条款**。有网络条款的是 **AGPL-3.0 §13** ——
只要用户通过网络与它交互，就必须向这些用户提供源码。

**✅ 已实测：整个依赖树零 AGPL**

```bash
grep -rn "SPDX-License-Identifier.*AGPL\|License: AGPL" \
  <vcpkg>/installed/x64-windows/share/
# → 零命中
```

⚠️ **误报陷阱**：直接搜 `affero` 会命中 `cgal` / `gmp` / `mpfr` 的 `copyright` ——
那是 **GPL-3.0 全文第 13 条的标题**（"Use with the GNU Affero General Public
License"），**是误报**。判据必须用 `SPDX-License-Identifier.*AGPL`。

**边界会破的三种情况**：
1. 把二进制交给客户 / 外包去部署 → 属于再分发
2. 网站同时提供 exe 下载 → 那一部分按分发处理
3. **网站自己的后端栈引入 AGPL 组件** → 另一个项目的合规问题，但同样致命
   （常见 AGPL：旧版 MongoDB、部分 Grafana / MinIO）。上线前单独查一遍。

**输出物不是问题**：OBJ / MTL / PNG **是数据不是程序**，不构成 GPL 的"对应源码"义务。

### 5. 「同一个 zip ≠ 同一个作品」——✅ 允许

把 GPL 的 exe 和 MIT 的 app 打包进同一个 zip 分发是**允许**的，
这叫 **"单纯聚合"（mere aggregation）**，前提是**两者仍是独立程序**。
只是 GPL 那部分（exe）的许可文本、声明、源码照旧要齐备。

---

## 六、与 app 仓库的 GPL 边界

**结论：边界干净，app 可以继续是 MIT。**

已核实 `minecraft-littletiles-reader-app`：

| 检查项 | 结果 |
|---|---|
| 许可 | **MIT**（`Copyright (c) 2026 FU Hongren`） |
| 技术栈 | Python + Qt（PySide） |
| 调用方式 | **`subprocess` / `QProcess` 启动 `LittleTilesReader.exe`** |
| `ctypes` / `cffi` / `CDLL` / `LoadLibrary` | **零命中** |
| `pybind11` / 链接 `galib` | **零命中** |

**为什么安全**：GPL 的传染边界是"**是否构成一个作品**"。app 通过
**进程边界 + 命令行参数**调用 reader，两者是**独立程序**在 arm's length 地通信
→ **不构成合并作品** → app 的 MIT 不受影响。

> 把库设计成「别的程序调用命令行就能跑」，**恰好选对了架构**。
> 这不是运气，是 CLI 化带来的真实法律收益。

**⚠️ 必须守住的线：绝对不要为了"方便"改成链接。**

| 做法 | 后果 |
|---|---|
| `subprocess` / `QProcess` 调用 exe | ✅ 独立程序，app 保持 MIT |
| `ctypes.CDLL(...)` 加载 DLL | ❌ 变成链接 → app 成为 GPL 衍生作品 |
| `pybind11` / 静态链接 `galib` | ❌ 同上，app 必须整体 GPL |
| 两者打包进同一个 zip 分发 | ✅ 允许（单纯聚合） |

**实操要改两处**：
1. `ltgen/paths.py` 的 `reader_executable()` 现在硬找 `<library_root>/cmake-build-debug/`，
   是**开发期路径**。对外分发要改成：先找 app 自身目录旁的 exe，再回落到
   `LTR_LIBRARY` 环境变量，最后才回落到开发期路径。
2. 建立 **reader 版本 ↔ app 版本**的对应关系（app 启动时读一次
   `LittleTilesReader --version` 做校验，避免参数错配静默失败）。

---

## 七、双渠道分发（自己的网站 + GitHub Releases）

**这是走 A 路线最容易踩的坑。** GPL-3.0 §6(d) 要求：提供对象代码时，
必须**"通过同一个地方（through the same place）、以同样的方式、不再额外收费"**
地提供对应源码。

- **网站下载页上必须有显著、等价的源码入口** —— 不能藏在 README 角落，
  也不能只放 GitHub 链接就完事（严格读法不认"跳去另一个地方"）。
- **最稳妥：在自己站点上也镜像一份源码包**。理由不只是合规读法 ——
  哪天 GitHub 仓库改名/私有化/删除，而你的网站还在发 exe，源码链接就断了，
  那是实打实的违规。
- **源码不能收费、不能要求注册**。exe 免费，源码也必须同等免费可得。
- **两个渠道的义务各自独立**：网站发的 exe 要满足 §6(d)，GitHub Release 也要满足。
  同一版本、同一份源码，两个地方都要挂。
- **建议 Release 同时挂两个附件**：
  `LittleTilesReader-<版本>-win-x64.zip`（二进制）+
  `LittleTilesReader-<版本>-source.zip`（对应源码快照）。
  这样两个渠道都能"就地"满足义务，不依赖任何外部链接。

---

## 八、附带风险：贴图与模型输出的再分发

reader 会把模组贴图提取成 `*_textures/` 目录，**这些贴图的版权属于各模组作者**，
**不随本项目的 MIT/GPL 授权一起给你**。

**好消息**：`.gitignore` 同时忽略了 `/outputs/` 和 `/texture`，
**仓库本身不含贴图**，所以**发布的 exe 是干净的**（贴图运行时由
`LITTLETILES_ASSETS` 提供）。

**注意**：别把示例输出或贴图原图放进网站 / Release 的下载包。
想在网站展示效果就用**自己渲染的截图**。

---

## 九、待办与下一步

**已定**
- [x] 三选一定案：**出路 A**（接受二进制 GPL-3.0+）

**待用户决定**
- [ ] 网站上的源码怎么放：只链接 GitHub，还是也在自己站点镜像 `source.zip`？
      （**建议镜像**，理由是防断链）
- [ ] app 仓库要不要也对外发布？若发，两个仓库的版本号怎么对齐？

**待执行（决策定了就能开工）**
- [ ] `CMakeLists.txt` 的 `project()` 补 `VERSION 0.1.0`（当前没有 VERSION 字段）
- [ ] `.gitignore` 补 `cmake-build-release/`（当前没有，构建产物会污染 `git status`）
- [ ] 做 Release 构建（当前 `cmake-build-debug/` 里全是 Debug 产物，**不能对外发布**）
- [ ] 生成 `LICENSE-GPL-3.0` / `LICENSE-LGPL-3.0` / `THIRD-PARTY-NOTICES.txt`
- [ ] 准备 `source.zip`
- [ ] 打 tag `v0.1.0` + 建 GitHub Release（默认分支是 `v2ForLLM`，注意挂对分支）

**发布环境备忘**
- Visual Studio：`A:\Application\VisualStudio\2026`（vc145 工具集）
- vcpkg：`D:\Development\DevLib\CorCpp\vcpkg`（triplet `x64-windows`，动态链接）
- 运行时依赖 **8 个 DLL**：`boost_iostreams-*`、`boost_json-*`、`zlib1`、
  `nbt++`、`gmp-10`、`bz2d`、`liblzma`、`zstd`
- ⚠️ VS 2026 / vc145 太新，产物依赖新版 MSVC 运行库，用户机器大概率没有
  → 要么在发布说明里要求装对应 Redistributable，要么改 `/MT` 静态链接 CRT
  （注：`/MT` 只解决 CRT，**不影响 GPL 结论**）
- 本机**未安装 `gh` CLI** → 建 Release 需先装 gh 或走网页 UI

---

## 十、完整发布流程

见 [`docs/release-and-licensing.md`](release-and-licensing.md)：
§4 十步打包清单、§5 `THIRD-PARTY-NOTICES.txt` 模板、§6 Release notes 模板。
