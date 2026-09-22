# 构建与发版操作手册（三平台）

> 本文是**唯一的构建口径**：从改版本号到产出三平台安装包，照着做即可。
> 许可证与合规依据不在本文，见 [release-and-licensing.md](release-and-licensing.md)。
> 项目现状与交接信息见 [HANDOFF.md](HANDOFF.md)。
>
> 最后验证的版本：**0.2.0-beta**（三平台已发布，tag `v0.2.0-beta` → `f2de641`）。
> 当前工作区已升到 **0.3.0**，尚未构建发布。

---

## 0. 一分钟速查

| 平台 | 在哪编 | 工具链 | 产物 |
|---|---|---|---|
| **macOS arm64** | Mac 本机 | CLion 自带 cmake + ninja，依赖走 Homebrew | `LittleTilesReader-<版本>-macos-arm64.tar.gz` |
| **Linux x86_64** | OpenCloudOS 9.6 虚拟机（`10.0.0.249`） | CMake **3.29.6** + Ninja + dnf | `LittleTilesReader-<版本>-linux-x86_64.tar.gz` |
| **Windows x64** | Windows 本机 | MSVC **v143**（VS2022）+ Ninja + vcpkg **静态** triplet | `LittleTilesReader-<版本>-windows-x64.zip` |

三平台**必须都到位**才发版，缺一个就别发。

```sh
# 三平台的共同核心（各自再叠加平台参数）
cmake -S . -B cmake-build-release -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DGALIB_VERSION_CHANNEL=beta
cmake --build cmake-build-release
```

> 构建目录统一用 **`cmake-build-release/`** —— 它已在 `.gitignore` 里。
> 别用 `build-release/`，那个名字**没被忽略**，会往 `git status` 里塞上百个未跟踪文件。

---

## 1. 改版本号 —— 有两处，必须同步

### 1.1 单一事实来源

`Galib/include/Version.h`：

```cpp
#define GALIB_VERSION_MAJOR 0
#define GALIB_VERSION_MINOR 3
#define GALIB_VERSION_PATCH 0
```

`--version`、job/progress 事件、导出记录都读这里，没有第三处需要手工同步。

### 1.2 ⚠️ 但 `CMakeLists.txt` 也必须一起改

```cmake
project(LittleTilesReader VERSION 0.3.0 LANGUAGES CXX)
```

这个 `VERSION` 只喂给 CMake 与打包（exe 文件属性、包名），**和头文件是两套**。
两处不一致时，`--version` 打印的是一套、Windows 文件属性里是另一套，发版页面上对不上。
**改版本号时同时改这两处**，这是唯一容易漏的地方。

### 1.3 发布通道

默认 **`beta`**，这是**故意的** —— 随手编出来的构建不该自称稳定版。

```sh
-DGALIB_VERSION_CHANNEL=beta      # 测试版，输出 "0.3.0-beta (test build)"
-DGALIB_VERSION_CHANNEL=stable    # 正式版，输出 "0.3.0-stable (stable release)"
```

不发 stable 就不要传 `stable`。`IsStableRelease()` 只在通道恰好等于 `stable` 时为真。

---

## 2. 依赖版本（各平台实测，别照抄）

| 组件 | Windows（vcpkg） | Linux（dnf） | macOS（brew） |
|---|---|---|---|
| Boost | **1.89.0** | **1.82.0** | 以 configure 日志为准 |
| CGAL | **6.1** | **5.6.1** | 以 configure 日志为准 |
| zlib | **1.3.1** | **1.2.13** | 以 configure 日志为准 |
| libnbt++ | commit `687e4303`（三平台一致，FetchContent 钉死） | 同 | 同 |
| 编译器 | MSVC 14.38（v143） | gcc 12.3.1.8 | Apple Clang |
| CMake | 3.30.2 | **3.29.6** | CLion 自带 |

**三平台版本本来就不一样，这是正常的**（vcpkg / dnf / brew 各发各的）。
需要一致的是**行为**，不是版本号 —— 见 §7 的冒烟基准。

代码侧硬约束：
- **C++17**（if-init、嵌套命名空间、`std::filesystem`）
- **Boost ≥ 1.78**（`boost::json::string_view` 的接口变更）—— 这就是 Linux 不能用 Rocky 9 的原因
- CGAL 6.x / 5.6 均可，但 `Surface_mesh` 等包的 GPL 传染性不变

---

## 3. Windows x64

### 3.1 工具链

| 项 | 路径 / 值 |
|---|---|
| 编译器 | `A:\Application\VisualStudio\2022\VC\Tools\MSVC\14.38.33130\bin\Hostx64\x64\cl.exe` |
| vcvars（**别直接用**） | `A:\Application\VisualStudio\2022\VC\Auxiliary\Build\vcvars64.bat` |
| **环境入口（用这个）** | `C:\tmp\env_v143.bat` —— 包了一层，见 §3.3(a) |
| Ninja | `A:\Application\VisualStudio\2022\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe` |
| vcpkg | `D:\Development\DevLib\CorCpp\vcpkg` |
| Overlay triplet | `C:\tmp\vcpkg-triplets\x64-windows-static-env.cmake` |
| 0.2.0 用的构建脚本 | `C:\ltr-win\build.bat`（可直接复用） |

> 两个 VS 都在机器上：`...\VisualStudio\2022`（**v143，用这个**）与 `...\2026`（vc145）。
> 2022 的 v143 是实测通过的那个。

> ⚠️ `C:\tmp\` 与 `C:\ltr-win\` 都是**临时位置**，随时可能被清理。
> §3.6 把两个关键文件的内容完整抄了一份，丢了照着重建即可。

### 3.2 构建命令

从 Git Bash 直接跑 `cl.exe` 会因缺 `INCLUDE`/`LIB` 报 `C1083: Cannot open include file: 'cstdint'`，
**必须先套一层环境**。注意入口是 `env_v143.bat` 而**不是** `vcvars64.bat`（原因见 §3.3(a)）：

```bat
@echo off
call "C:\tmp\env_v143.bat"

cmake -S <源码目录> -B <源码目录>\cmake-build-release -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_TOOLCHAIN_FILE=D:/Development/DevLib/CorCpp/vcpkg/scripts/buildsystems/vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-windows-static-env ^
  -DVCPKG_OVERLAY_TRIPLETS=C:/tmp/vcpkg-triplets ^
  -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded ^
  -DGALIB_VERSION_CHANNEL=beta ^
  -DFETCHCONTENT_SOURCE_DIR_LIBNBTPLUSPLUS=C:/ltr-win/deps/libnbtplusplus-src
if errorlevel 1 ( echo CONFIGURE_FAILED & exit /b 1 )

cmake --build <源码目录>\cmake-build-release -j
if errorlevel 1 ( echo BUILD_FAILED & exit /b 1 )
echo BUILD_OK
```

期望结果：`[41/41] Linking CXX executable LittleTilesReader.exe`，**0 warning**。

- `<源码目录>` 用你要发的那个版本的工作副本。0.2.0 当时用的是 `C:\ltr-src`（从 `v2ForLLM` 单独 clone 的），
  与 `D:\DevelopmentProject\minecraft-littletiles-reader` 是两个不同的副本。
- `-DFETCHCONTENT_SOURCE_DIR_LIBNBTPLUSPLUS=...` **只在构建机没有外网时需要**；
  有网时删掉这行，让 `FetchContent` 正常 clone。

### 3.3 三个必须显式处理的坑

**(a) 安全策略拉黑 `reg.exe` → vcvars 找不到 Windows SDK → 每次 `try_compile` 都失败**

`vcvars64.bat` 是靠 shell 调用 `reg.exe` 去注册表里查 Windows SDK 的。
这台机器的命令安全策略**拉黑了 `reg.exe`**，于是 SDK 路径查不到：

- 没有 `rc.exe` / `mt.exe`
- 没有 ucrt 头与库

后果是 configure 阶段**每一次 `try_compile` 都失败**，报错看起来像编译器坏了 —— 其实不是。

解法：`C:\tmp\env_v143.bat` —— 先 `call vcvars64.bat`（拿到 `cl.exe` 与 MSVC 的头/库），
**再手工把 SDK 的 `PATH` / `INCLUDE` / `LIB` 补上**。所以 Windows 构建的环境入口必须是这个文件，
**不能直接 `call vcvars64.bat`**。完整内容见 §3.6。

**(b) 静态 CRT 不会自动传给项目 → 必须手加 `-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded`**

不传会链接失败：

```
libcpmt.lib(locale0.obj) : error LNK2038: mismatch detected for 'RuntimeLibrary':
value 'MT_StaticRelease' doesn't match value 'MD_DynamicRelease' in main.cpp.obj
fatal error LNK1169: one or more multiply defined symbols found
```

原因：依赖是 `/MT` 编的（triplet 里 `VCPKG_CRT_LINKAGE=static`），但 **vcpkg 的 toolchain 脚本
完全没有把 `CMAKE_MSVC_RUNTIME_LIBRARY` / `VCPKG_CRT_LINKAGE` 传给消费方项目**，
项目就退回了 CMake 自己的 MSVC 默认值 `/MD`。
→ 只影响构建命令行，**仓库里不需要改任何东西**。

**(c) 构建机没有外网 → 用 `3rdparty/CMakeLists.txt` 自带的离线钩子**

`FetchContent` 会在 configure 阶段去 clone `PrismLauncher/libnbtplusplus`。
外网不通时用仓库里已写好的替代入口（注释里就有）：

```sh
-DFETCHCONTENT_FULLY_DISCONNECTED=ON
# 或
-DFETCHCONTENT_SOURCE_DIR_LIBNBTPLUSPLUS=<已有的本地源码树>
```

本地源码树的 HEAD 必须是 `687e43031df0dc641984b4256bcca50d5b3f7de3`，
否则离线构建和联网构建用的不是同一个 revision。

### 3.4 核对依赖（别跳过）

```bat
call A:\Application\VisualStudio\2022\VC\Auxiliary\Build\vcvars64.bat
dumpbin /dependents cmake-build-release\LittleTilesReader.exe
```

**0.2.0 的实测结果**（这是目标形态）：

```
nbt++.dll
KERNEL32.dll
```

只有 `nbt++.dll` 一个非系统 DLL，**没有 `vcruntime140.dll` / `msvcp140.dll`**
→ 证明 `/MT` 生效 → **目标机不需要 VC++ 运行库**。

Boost / zlib / bzip2 / liblzma / zstd / GMP / MPFR / CGAL 全部静态编进 exe，
既不出现在导入表里，也不作为文件出现在包里。

反向验证（推荐做一次）：把 `.exe` **单独**拷进空目录运行 → 应该是 exit 127（找不到 DLL），
证明 `nbt++.dll` 确实必需、且确实是唯一缺的东西。

### 3.5 打包

包内 **6 个文件**：

```
LittleTilesReader-0.3.0-beta-windows-x64/
├── LittleTilesReader.exe
├── nbt++.dll
├── LICENSE
├── LICENSE-GPL-3.0
├── README.md
└── THIRD-PARTY-NOTICES.txt
```

### 3.6 两个临时文件的内容（丢了照抄重建）

这两个文件都在临时目录里，是 0.2.0 构建的关键前提，内容原样抄在这里。

**`C:\tmp\vcpkg-triplets\x64-windows-static-env.cmake`**

```cmake
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)

# The machine's command security policy blocks reg.exe, so vcvars64.bat cannot
# discover the Windows SDK by itself and vcpkg rebuilds PATH/INCLUDE/LIB without
# any SDK paths. These variables are injected by C:\tmp\env_v143.bat and are
# forwarded to the port builds so that rc.exe / mt.exe / ucrt are reachable.
set(VCPKG_ENV_PASSTHROUGH INCLUDE LIB PATH WindowsSdkDir WindowsSDKVersion WindowsSdkBinPath WindowsSdkVerBinPath)
```

`VCPKG_ENV_PASSTHROUGH` 是关键：vcpkg 构建端口时会**重建** `PATH`/`INCLUDE`/`LIB`，
不 passthrough 的话手工补的 SDK 路径会被丢掉，依赖照样编不过。

**`C:\tmp\env_v143.bat`**

```bat
@echo off
rem ---------------------------------------------------------------------------
rem MSVC v143 (Visual Studio 2022) + Windows SDK 10.0.22621.0 build environment.
rem
rem Why this file exists: vcvars64.bat locates the Windows SDK by shelling out to
rem reg.exe, which is blocked by the machine's command security policy. Without
rem the SDK the toolchain has no rc.exe/mt.exe and no ucrt headers, so every
rem try_compile fails. This wrapper calls vcvars64.bat first (to get cl.exe and
rem the MSVC headers/libs) and then appends the SDK paths by hand.
rem ---------------------------------------------------------------------------

call "A:\Application\VisualStudio\2022\VC\Auxiliary\Build\vcvars64.bat" >nul

set "WindowsSdkDir=C:\Program Files (x86)\Windows Kits\10\"
set "WindowsSDKVersion=10.0.22621.0\"
set "WindowsSdkBinPath=C:\Program Files (x86)\Windows Kits\10\bin\"
set "WindowsSdkVerBinPath=C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\"

set "PATH=%WindowsSdkVerBinPath%x64;%WindowsSdkBinPath%x64;%PATH%"
set "INCLUDE=%INCLUDE%;%WindowsSdkDir%Include\%WindowsSDKVersion%ucrt;%WindowsSdkDir%Include\%WindowsSDKVersion%shared;%WindowsSdkDir%Include\%WindowsSDKVersion%um;%WindowsSdkDir%Include\%WindowsSDKVersion%winrt"
set "LIB=%LIB%;%WindowsSdkDir%Lib\%WindowsSDKVersion%ucrt\x64;%WindowsSdkDir%Lib\%WindowsSDKVersion%um\x64"

set "VCPKG_ROOT=D:\Development\DevLib\CorCpp\vcpkg"
set "VCPKG_VISUAL_STUDIO_PATH=A:\Application\VisualStudio\2022"
set "VCPKG_PLATFORM_TOOLSET=v143"
```

> 若哪天安全策略不再拦 `reg.exe`，这个 wrapper 就可以退休，直接 `call vcvars64.bat` 即可。
> 但在那之前，**不要**删它 —— 删了 Windows 构建会以"编译器坏了"的形式失败。

---

## 4. Linux x86_64

### 4.1 构建机

| 项 | 值 |
|---|---|
| 虚拟机 | `D:\ApplicationData\VirtualMachine\LTR-Linux\LTR-Linux-OpenCloudOS9.vmx` |
| `vmrun` | `A:\Application\VMware Workstation\vmrun.exe`（**不在默认 PATH**） |
| OS | **OpenCloudOS 9.6**（`6.6.119-51.5.oc9.x86_64`） |
| 网络 | MAC `00:50:56:2f:a1:07` / IP `10.0.0.249/24` / 网关 `10.0.0.1` / 主机名 `ltr-build` |
| 登录 | `ltr` / `ltrbuild`，密钥 `_ltr-stage\cloudinit\ltr_ed25519` |
| 规格 | 4 vCPU / 8 GB / 60 GB，桥接 |

### 4.2 为什么必须是 OpenCloudOS 9.6

| | Rocky Linux 9.8 | **OpenCloudOS 9.6** |
|---|---|---|
| Boost | **1.75.0** → 编译失败 | **1.82.0** ✓ |
| glibc | 2.34 | **2.38** ✓ |
| CGAL | CRB 里 5.6.x | EPOL 里 5.6.1 ✓ |

`main.cpp` 需要 Boost ≥ 1.78（`boost::json::string_view` 在 1.78 才变成 `std::string_view` 的别名），
Rocky 9.8 的 1.75 编不过。**"RHEL 9 家族都有 Boost 1.82" 这个假设不成立，要实测。**

### 4.3 依赖安装

```sh
# CGAL 在独立的 EPOL 仓库
sudo dnf install -y \
    https://mirrors.opencloudos.tech/epol/9/Everything/x86_64/os/Packages/.../epol-release-*.rpm
# 或手工加 repo 文件指向 https://mirrors.opencloudos.tech/epol/9/Everything/x86_64/os/

sudo dnf install -y gcc-c++ ninja-build \
    boost-devel boost-iostreams boost-json boost-container \
    zlib-devel bzip2-devel xz-devel \
    CGAL-devel          # ⚠️ 大写！小写的 cgal-devel 找不到
```

Boost 被拆包，`boost-devel` 之外还要 `boost-json` / `boost-iostreams` / `boost-container`。

### 4.4 ⚠️ CMake 必须用 3.29.x

`CMakeLists.txt` 里有 `cmake_policy(SET CMP0167 NEW)`，
**CMake ≥ 3.30 会移除内置 `FindBoost` 模块**，转而走 config 模式要 `BoostConfig.cmake`，
而 OpenCloudOS 的 `boost-devel` **不带**它 → 报 `Could not find BoostConfig.cmake`。

3.29.6 仍带 `FindBoost.cmake`，且满足 `cmake_minimum_required(3.28)`：

```sh
curl -LO https://cmake.org/files/v3.29/cmake-3.29.6-linux-x86_64.tar.gz
tar xzf cmake-3.29.6-linux-x86_64.tar.gz -C ~/tools/
export PATH=~/tools/cmake-3.29.6-linux-x86_64/bin:$PATH
```

### 4.5 构建命令

```sh
cmake -S ~/ltr-src -B ~/ltr-src/build-release -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DGALIB_VERSION_CHANNEL=beta \
      -DCMAKE_INSTALL_RPATH='$ORIGIN' \
      -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON \
      -DCMAKE_EXE_LINKER_FLAGS='-Wl,--as-needed' \
      -DCMAKE_SHARED_LINKER_FLAGS='-Wl,--as-needed'
cmake --build ~/ltr-src/build-release -j4
```

`CMAKE_BUILD_WITH_INSTALL_RPATH=ON` 让 rpath 直接烘进构建树的二进制，
**不需要额外的 `patchelf` 步骤**。

### 4.6 `-Wl,--as-needed` 是标准配置，别省

不加这个 flag，`NEEDED` 里会挂着 **GMP / MPFR / Boost.Regex / ICU**，而它们**符号引用为 0**
（是 CGAL / Boost 的 CMake 链接行没用 `--as-needed` 造成的硬性运行时依赖），
让包在别的发行版上更难起。

加了之后 **`NEEDED` 13 → 8**，去掉 `libboost_regex` / `libgmpxx.so.4` / `libmpfr.so.6` /
`libgmp.so.10` 与 ICU，`libboost_container` 从直接依赖降为间接依赖（经 Boost.JSON）。
**实测输出逐字节不变**，峰值内存反而降约 2.3 MB。

保留的 8 条：

```
libboost_iostreams.so.1.82.0
libboost_json.so.1.82.0
libz.so.1
libnbt++.so                     (包内自带，靠 $ORIGIN 加载)
libstdc++.so.6
libm.so.6
libgcc_s.so.1
libc.so.6
```

> ⚠️ **Linux 包是动态链接系统库的**，`README` 与 `THIRD-PARTY-NOTICES.txt` 必须按
> **实际的 `ldd` 闭包**写。0.2.0 的 release notes 里那句"其余依赖静态链接"是**错的**，
> 发 0.3.0 时别照抄。

### 4.7 运行时自检

```sh
cd <解包后的包目录>
ldd ./LittleTilesReader                                   # not found 必须 0
ldd -r ./LittleTilesReader 2>&1 | grep -c 'undefined symbol'   # 必须 0
readelf -d ./LittleTilesReader | grep -i 'runpath\|rpath' # 必须 [$ORIGIN]
strings ./LittleTilesReader | grep -c "$HOME"             # 必须 0
strings ./LittleTilesReader | grep -c '/home/'            # 必须 0
strings ./LittleTilesReader | grep -c 'ltr-src'           # 必须 0
objdump -T ./LittleTilesReader | grep -o 'GLIBC_[0-9.]*' | sort -uV | tail -1
mkdir -p /tmp/clean && cp -r . /tmp/clean/ && cd /tmp/clean
env -i ./LittleTilesReader --version                      # rc=0，输出 0.3.0-beta
```

运行时下限（0.2.0 实测）：`GLIBC_2.38` / `GLIBCXX_3.4.29` / Boost 1.82（soname 锁死）
→ OpenCloudOS 9 / Rocky 9 / Alma 9 同代可用；RHEL 8 / Debian 12 / Ubuntu 22.04 **不行**。

### 4.8 打包

包内 **6 个文件**，与 macOS 严格对齐：

```
LittleTilesReader-0.3.0-beta-linux-x86_64/
├── LittleTilesReader         (可执行)
├── libnbt++.so               ← SONAME 就是不带版本号的 libnbt++.so，只有一个文件
├── LICENSE
├── LICENSE-GPL-3.0
├── README.md
└── THIRD-PARTY-NOTICES.txt
```

> ⚠️ 不要试图造 `libnbt++.so.N` + 同名软链 —— libnbt++ 的 SONAME 就是 `libnbt++.so`，
> macOS 包同理只有一个 `libnbt++.dylib`。

---

## 5. macOS arm64

> 本节是**已验证要点**的汇总，不是逐步教程 —— Mac 侧构建一直由本机完成，
> 没有留下逐命令记录。第一次在新 Mac 上重建时，把实际用的命令补回这一节。

### 5.1 工具链

- CLion 自带的 cmake + ninja（不必另装）
- 依赖走 **Homebrew**（`CMakeLists.txt` 在 `APPLE` 且无 `VCPKG_ROOT` 时自动 `brew --prefix`）
- 也可以设 `VCPKG_ROOT` 强制走 vcpkg —— 逻辑在 `CMakeLists.txt` 顶部，vcpkg 优先级高于 brew

### 5.2 构建

```sh
cmake -S . -B cmake-build-release -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DGALIB_VERSION_CHANNEL=beta
cmake --build cmake-build-release -j8
```

### 5.3 ⚠️ 打包时的三个动作，顺序不能反

```sh
install_name_tool -change <原依赖路径> @rpath/libnbt++.dylib <包内 LittleTilesReader>
install_name_tool -add_rpath @executable_path <包内 LittleTilesReader>
install_name_tool -delete_rpath <开发机绝对路径> <包内 LittleTilesReader>
codesign --force --sign - <包内 LittleTilesReader> <包内 libnbt++.dylib>
```

要点：

1. **`install_name_tool` 会破坏签名 → 必须重新 `codesign`，反过来（先签后改）签名失效。**
2. **必须 `-delete_rpath`**，否则包里带着开发机的绝对路径。
3. ad-hoc 签名（`--sign -`）就够；用户首次打开被 Gatekeeper 拦时右键「打开」即可。

### 5.4 打包

```
LittleTilesReader-0.3.0-beta-macos-arm64/
├── LittleTilesReader
├── libnbt++.dylib
├── LICENSE
├── LICENSE-GPL-3.0
├── README.md
└── THIRD-PARTY-NOTICES.txt
```

---

## 6. 打包规范（三平台共同）

### 6.1 包内文件

固定 **6 个文件**（见各平台小节）。三个包**结构一致**，只有可执行文件与运行库的名字不同。

### 6.2 ⚠️ `THIRD-PARTY-NOTICES.txt` 必须从**实际链接**生成，不要跨平台照抄

同一个代码在三个平台的依赖形态**完全不同**：

| | Windows | Linux | macOS |
|---|---|---|---|
| Boost / zlib | **静态**编进 exe | **动态**链系统库 | **静态**编进 exe |
| GMP / MPFR | 静态编进 exe | 动态（加了 `--as-needed` 后**不链接**） | **不链接** |
| bzip2 / liblzma / zstd | 静态编进 exe | 动态（间接依赖） | 视情况 |
| Expat | **未链接**（CGAL 6.1 此配置不用） | — | notices 里提到过 |

生成依据：`readelf -d | grep NEEDED` + `ldd` + `rpm -qf`（Linux）、
`dumpbin /dependents`（Windows）、`otool -L`（macOS）。
并写明 **glibc / GLIBCXX 下限**与**构建发行版**。

如果做了 `-Wl,--as-needed`，NOTICES 里加一节
**"Components deliberately not linked"**，说明去掉了哪几个、为什么、以及它们的许可不再适用 ——
免得后来的人又把它们加回去。

### 6.3 包内的 `README.md` 不是仓库的 README

包内的 `README.md` 是**给最终用户看的**：怎么解压、怎么运行、需要哪些系统库
（Linux 要给准确的 `dnf install` 行）、以及本平台的许可说明。

⚠️ **写包内文档一律用纯 ASCII**，不要用长破折号 / 弯引号。
0.2.0 的 Linux 包就中过招：README 标题里的 em dash 经 SSH 管道变成了 `???`，成了
`# LittleTilesReader 0.2.0-beta ??? Linux x86_64`。自查：

```sh
LC_ALL=C grep -c '[^ -~]' README.md      # 期望 0
```

### 6.4 许可文件

每个包都要有 `LICENSE`（MIT，项目自身）与 `LICENSE-GPL-3.0`（GPL-3.0 全文）。
二进制按 **GPL-3.0-or-later** 分发（被 CGAL 的 GPL 包传染），依据见
[release-and-licensing.md](release-and-licensing.md) §3。

---

## 7. 冒烟测试与基准值

### 7.1 三个用例

| # | 用例 | 输入 |
|---|---|---|
| 1 | `--version` | — |
| 2 | 存档导出，`base` chunk `0,0` 半径 1 | `.minecraft/saves/base`（overworld），3×3 = 9 区块 |
| 3a | SNBT 导出 | `CoronaSign.txt` |
| 3b | SNBT 导出（大结构） | `SHB_05_Contemporary_Style_House_v1.1.txt` |

跑法（机读，从 NDJSON 的 `done` 事件取数）：

```sh
./LittleTilesReader --job <job.json> --progress json
```

### 7.2 ⚠️ 0.2.0 的基准值只能作参考，**0.3.0 必须重建基线**

0.2.0 实测（三平台一致）：

| 用例 | 指标 | 值 |
|---|---|---|
| region 0,0 r1 | chunks / tiles / vertices / faces | 9（缺 0）/ 236 / **12975** / **4940** |
| CoronaSign | meshes | **1114** |
| SHB house | meshes | **18044** |

**但 0.3.0 动了核心几何代码**（`BlockTileEntities.cpp` +164、`LtStructure.cpp` +455、
`ChunkBlocks.cpp` +190、`SnbtParser.cpp` +133），顶点/面数**是否变化必须实测** ——
既不要假设"只是加功能所以数字不变"，也不要假设它一定变了。
0.3.0 构建完必须：

1. 三平台各跑一遍，记录**新的**实测值；
2. 三平台互相对比（这是真正要保证的一致性）；
3. 用新值更新 `docs/benchmark.md` 与发布说明；
4. **不要把 0.2.0 的数字直接抄进 0.3.0 的发布说明。**

峰值内存与耗时也一并记录（`/usr/bin/time -v` on Linux，`GetProcessMemoryInfo` / 任务管理器 on Windows）。
同用例重复跑有几十 KB 的抖动，属正常。

### 7.3 额外回归项（0.3.0 新增功能，别漏测）

0.3.0 新增了这些能力，**冒烟时至少各覆盖一次**：

| 新功能 | 怎么测 |
|---|---|
| **1.18+/1.20 存档布局** | 用同一份建筑在 1.12.2 与 1.20.1 两个存档里各导一次，对比面数与材质集合 |
| **SNBT → SNBT 互转** | `examples/snbt_convert.json`，检查输出的 1.20 SNBT 能被自己再读回来 |
| **跨版本贴图兜底** | 用旧命名的素材包（`silver_concrete` 等）跑 1.20 存档，确认面数与材质集合对齐 |

已知的**有意差异**（不是 bug，别当回归报）：结构互转时组的顺序可能变、显式 `color:-1`
跨代会归一化为"无颜色"、1.12.2 的 `tID` 类私有键在 1.20 无处安放 ——
逐条见 [snbt-format.md](snbt-format.md) §5.1。

---

## 8. 产物汇总与校验

### 8.1 汇总目录

```
D:\DevelopmentProject\ltr-release\
```
（Mac 侧同一份在 `\\10.0.0.11\HDD` → 挂载后 `/tmp/hddmnt/DevelopmentProject/ltr-release/`；
各仓库自己的 `dist/` 是 **gitignore** 的。）

### 8.2 `SHA256SUMS.txt` 的三条规则

1. **覆盖包内每一个文件**，不只可执行文件。
2. **路径用正斜杠**。PowerShell 的 `Join-Path` 产出 `\`，在 Linux 上
   `sha256sum -c` 会直接报 "no such file"。
3. **LF 换行、无 BOM**。`Set-Content` 会写 CRLF 且可能加 BOM；用
   `[System.IO.File]::WriteAllText($p, $t, (New-Object System.Text.UTF8Encoding($false)))`。

写完**必须实测**：

```sh
cd D:\DevelopmentProject\ltr-release
sha256sum -c SHA256SUMS.txt          # 期望：每个文件一行 OK
```

### 8.3 追加而不是重写

新平台的哈希**追加**到已有清单，**不要动其他平台的行**。
追加前先备份，追加后 `diff` 确认旧行逐字节未变，再用第二个实现
（Linux `sha256sum` ↔ Windows `Get-FileHash`）交叉验证一遍。

---

## 9. 发版

三平台的包与 `SHA256SUMS.txt` 齐了之后：

```sh
git tag -a v0.3.0-beta -m "…"
gh release create v0.3.0-beta \
  --title "LittleTilesReader 0.3.0-beta" \
  --prerelease \
  --notes-file docs/release-notes-v0.3.0-beta.md \
  dist/<mac>.tar.gz dist/<linux>.tar.gz dist/<win>.zip dist/SHA256SUMS.txt
```

要点：

- **默认分支是 `v2ForLLM`**，tag 打在哪个 commit 上要想清楚。
- beta 一律 **`--prerelease`**（不占 "Latest"）。
- 发布说明模板见 [release-and-licensing.md](release-and-licensing.md) §6，
  上一版实例见 [release-notes-v0.2.0-beta.md](release-notes-v0.2.0-beta.md)。
- 发版前必须：三平台包都到位、`SHA256SUMS.txt` 按磁盘重算、
  附件 URL 实测 HTTP 200 且字节数一致、发布说明里的数字来自**实测**。
- 若同时在**自己的网站**提供下载：GPL-3.0 §6(d) 要求源码在**同一个地方**免费可得，
  不能只放一个跳去 GitHub 的链接。详见 release-and-licensing.md §3.6.1。

---

## 10. 坑速查表

| # | 坑 | 症状 | 解法 |
|---|---|---|---|
| 1 | 版本号两处不同步 | `--version` 与 exe 文件属性对不上 | 同时改 `Version.h` 与 `CMakeLists.txt` |
| 2 | 安全策略拦 `reg.exe` | vcvars 查不到 Windows SDK，每次 `try_compile` 都失败（像编译器坏了） | 用 `C:\tmp\env_v143.bat` 作环境入口，别直接 call `vcvars64.bat` |
| 3 | Windows 静态 CRT 不传递 | `LNK2038 RuntimeLibrary mismatch` | 加 `-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded` |
| 4 | vcpkg 端口里 SDK 路径丢失 | 依赖编译报缺 `rc.exe` / ucrt | triplet 里加 `VCPKG_ENV_PASSTHROUGH INCLUDE LIB PATH ...` |
| 5 | Windows 无外网 | FetchContent clone 失败 | `-DFETCHCONTENT_SOURCE_DIR_LIBNBTPLUSPLUS=<本地源码>` |
| 6 | Git Bash 跑 `cl.exe` | `C1083: cstdint` | 先走 `env_v143.bat` |
| 7 | CMake ≥ 3.30 | `Could not find BoostConfig.cmake` | 用 CMake **3.29.x** |
| 8 | Rocky 9 的 Boost 太旧 | `string_view` 编译错误 | 用 OpenCloudOS 9.6 |
| 9 | CGAL 包名大小写 | `cgal-devel` 找不到 | **`CGAL-devel`**，且在 EPOL 仓库 |
| 10 | Linux 硬链无用库 | `NEEDED` 里挂 GMP/MPFR/Regex/ICU | 加 `-Wl,--as-needed`（已为标准配置） |
| 11 | 包内文档非 ASCII | README 里出现 `???` | 写纯 ASCII；`LC_ALL=C grep -c '[^ -~]'` |
| 12 | mac 打包顺序反了 | 签名失效 | `install_name_tool` **之后**才 `codesign`；并 `-delete_rpath` |
| 13 | `SHA256SUMS` 用反斜杠 | Linux 上 `sha256sum -c` 失败 | 正斜杠 + LF + 无 BOM |
| 14 | 跨平台照抄 NOTICES | 声明了实际没链接的库 | 按各平台**实际** `ldd` / `dumpbin` / `otool -L` 写 |
| 15 | 抄旧版发布说明的数字 | 与实测不符 | 每版重建基线（§7.2） |
| 16 | Windows 二进制里有开发机路径 | `grep -a` 能看到 `C:\ltr-src\...` | 已知项；`/pathmap` 可清第一种，vcpkg 预编译库那种要重建依赖 |
| 17 | 构建目录名用了 `build-release/` | `git status` 里冒出上百个未跟踪文件 | `.gitignore` 只忽略了 **`cmake-build-release/`** —— 统一用这个名字，别用 `build-release/` |

---

## 11. 边界（不要做的事）

- **不要 push `minecraft-littletiles-reader-app`** —— 那是站长的线，验收后他自己推。
- **动网站仓库（`inception-work`）前先问** —— 那条线由站长自己/另一个任务在推。
- **二进制按 GPL-3.0-or-later 分发**，不要加任何 GPL 禁止的附加限制
  （禁商用 / 禁再分发 / 强制署名 / EULA 门禁都不行）。可以对二进制收费。
- **不要把贴图或示例输出打进发布包** —— 模组贴图的版权属于各模组作者，不随本项目授权。
- **App 仓库绝不能改成 `ctypes` / `pybind11` / 静态链接 `galib`** ——
  一旦链接，MIT 的 app 立刻变成 GPL 衍生作品。进程边界 + 命令行才是安全的调用方式。
