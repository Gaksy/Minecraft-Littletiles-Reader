# 交接文档 · Minecraft LittleTiles Reader（主线）

最后更新：2026-09-17 · 交接人：Codex
面向对象：接手这个项目的下一个 AI / 开发者。**先读完这一页再动手**，里面有环境、命令、已完成的事、待办和踩过的坑。

---

## 一、一句话现状

**0.2.0-beta 已经发布**（macOS arm64 / Linux x86_64 / Windows x64 三平台附件，GitHub Pre-release），
核心库的仓库、标签、发布说明都齐了；**服务器上的白模生成链路也已跑通**（
`inception-work` 的模型共享用它把 SNBT 生成 OBJ）。

> Release：<https://github.com/Gaksy/Minecraft-Littletiles-Reader/releases/tag/v0.2.0-beta>
> tag `v0.2.0-beta` → `f2de641`；分支 `v2ForLLM`；发布说明见 [release-notes-v0.2.0-beta.md](release-notes-v0.2.0-beta.md)

---

## 二、仓库、目录与账号

| 项 | 位置 / 值 |
|---|---|
| 核心库（本仓库） | `/Users/external_elliott/Development/minecraft-littletiles-reader`，分支 **`v2ForLLM`**（可以 push） |
| 桌面客户端 | `/Users/external_elliott/Development/minecraft-littletiles-reader-app`，分支 `main` —— **不要 push**（站长验收后自己推） |
| 网站 | `/Users/external_elliott/Development/inception-work`，分支 `main`（另一条线，见第七节） |
| 发布产物汇总 | Windows 机 `D:\DevelopmentProject\ltr-release\`；Mac 侧同一份在 `\\10.0.0.11\HDD` → 挂载后 `/tmp/hddmnt/DevelopmentProject/ltr-release/`；本仓库 `dist/`（**gitignore**） |
| 测试数据 | `/Users/external_elliott/DevelopmentTestFolder/MinecraftLittleTIlesReader/`（`data/snbt/test_snbt/*.txt`、`data/regions/{base,escalator,subway}.zip`、`InceptionGN/.minecraft/saves/{base,escalator,subway}`） |
| 站点服务器 | `ssh -i <网站仓库>/SSH/43.138.2.99_id_ed25519 -p 27755 root@www.inception.work` |
| 服务器上的 CLI | `/opt/littletiles-reader/LittleTilesReader-0.2.0-beta-linux-x86_64/LittleTilesReader`（`--version` → `0.2.0-beta`） |
| 服务器环境变量 | `/www/wwwroot/inception-work-server/app.env` 里 `WORK_READER_BINARY=<上面的路径>`（**名字必须是 `WORK_READER_BINARY`**，`APP_` 前缀绑不上 `work.reader-binary`） |

---

## 三、这轮做完的事（都有实测数字）

### 1. 0.2.0-beta 三平台发布

| 平台 | 附件 | 大小 | sha256 |
|---|---|---|---|
| macOS arm64 | `LittleTilesReader-0.2.0-beta-macos-arm64.tar.gz` | 455,650 B | `f3bff53d9b049379c84d33534654cd081d11f2e2bf39f2ec5b4de324a5f226bb` |
| Linux x86_64 | `LittleTilesReader-0.2.0-beta-linux-x86_64.tar.gz` | 384,739 B | `d036673165c34c5db33b77bc4fe831577526801995253aa6b09e9dea0e891d43` |
| Windows x64 | `LittleTilesReader-0.2.0-beta-windows-x64.zip` | 641,876 B | `e087ff83046e4bfb53ee971fba08bac2fd11fa8071ded7b6b663a5909fd0fdd1` |

外加 `SHA256SUMS.txt`（含包内每个文件的哈希）。四个附件的下载 URL 都实测过 HTTP 200 且字节数一致。
Release 已标记 **Pre-release**（beta 不占 “Latest”）。

包内容：可执行文件 + 一个运行库（mac `libnbt++.dylib` / Linux `libnbt++.so` / Win `nbt++.dll`）
+ `LICENSE` + `LICENSE-GPL-3.0` + `THIRD-PARTY-NOTICES.txt` + `README.md`。
Windows 是**静态 CRT（/MT）**，`dumpbin` 确认只依赖 `nbt++.dll` 与系统 DLL，目标机不需要 VC++ 运行库。

### 2. 三平台结果一致（同一份测试数据，各自机器实测）

| 测试 | 结果 |
|---|---|
| 存档导出（`base` chunk 0,0 半径 1） | 12,975 顶点 / 4,940 面 / 0 缺贴图 —— macOS 0.03 s、Windows 0.083 s（峰值 14.9 MB） |
| SNBT（CoronaSign） | 1,114 meshes —— macOS 0.026 s、Linux 0.026 s、Windows 0.110 s（峰值 10.5 MB） |
| SNBT（大结构 SHB） | 18,044 meshes —— Linux 0.374 s（峰值 52 MB）、Windows 1.095 s（峰值 76.3 MB） |

### 3. 服务器白模生成跑通（网站那边在用）

- 服务器实测：CoronaSign 输出 OBJ **417,571 字节 / 8,568 顶点 / 6,106 面**，落在
  `/www/wwwroot/inception-work-uploads/work/mesh/<workId>/item-<itemId>.obj`
- 队列相关配置：`sys_config` 的 `work.generation-enabled=true`、`work.generation-concurrency=1`、`work.max-snbt-bytes=10485760`

---

## 四、怎么用（本地）

```bash
# 1) 导出存档
cd /Users/external_elliott/Development/minecraft-littletiles-reader
./dist/LittleTilesReader-0.2.0-beta-macos-arm64/LittleTilesReader --job examples/region.json

# 2) 导出 SNBT（examples/snbt.json 就是一份现成 job）
./dist/LittleTilesReader-0.2.0-beta-macos-arm64/LittleTilesReader --job examples/snbt.json

# 3) 看版本 / 进度事件（机读）
… --version
… --job <job.json> --progress json        # 逐行 NDJSON，模型共享的进度条就读它
```

job 文件的字段含义见 [job.md](job.md)；SNBT 格式见 [snbt-format.md](snbt-format.md)；
几何构建与"为什么不用布尔运算"见 [architecture.md](architecture.md)。

---

## 五、怎么重新构建三平台

**逐命令的详版见 [build-guide.md](build-guide.md)**（唯一的构建口径）。许可证依据见
[release-and-licensing.md](release-and-licensing.md)，另有两份构建报告在发布目录：
`FINAL-REPORT.md`（Linux 虚拟机）与 `SMOKE-TEST-windows-x64.txt`（Windows 本机）。要点：

| 平台 | 关键点 |
|---|---|
| macOS | CLion 自带 cmake+ninja；`-DGALIB_VERSION_CHANNEL=beta`；打完包要 `install_name_tool -change/-add_rpath/-delete_rpath` 再 `codesign --force --sign -`（**顺序不能反**） |
| Windows | MSVC v143 + Ninja + vcpkg **静态** triplet（`x64-windows-static` 派生，`/MT`）；libnbt++ 走 `3rdparty/CMakeLists.txt` 已有的离线钩子 `-DFETCHCONTENT_SOURCE_DIR_LIBNBTPLUSPLUS=<本地源码>`（构建机当时没有外网） |
| Linux | OpenCloudOS 9.6 虚拟机；需要仓库里的 Linux 平台分支（见下） |

跨平台一致性靠两处：`BlockTileEntities.cpp` 的平台判断（`#ifdef __APPLE__` 必须同时包含 `__linux__`，nbt++ 的
`tag_array<int32_t>` 取法在 Linux 与 macOS 相同）与 `CgalTypeDef.h` 的 `FloatType = double`（Linux 也走 double）。
这两处就是 `f2de641` 的内容。

---

## 六、怎么发下一次版

```bash
cd /Users/external_elliott/Development/minecraft-littletiles-reader
git tag -a v0.2.1-beta -m "…"            # 或让 gh 自动在默认分支 HEAD 建 tag
gh release create v0.2.1-beta \
  --title "LittleTilesReader 0.2.1-beta" \
  --prerelease \
  --notes-file docs/release-notes-v0.2.1-beta.md \
  dist/<mac>.tar.gz dist/<linux>.tar.gz dist/<win>.zip dist/SHA256SUMS.txt
```

发版前必须做：三平台的包都到位（缺一个就别发）、`SHA256SUMS.txt` 按磁盘重算、
附件 URL 实测 200 且字节数一致、发布说明里的数字来自**实测**而不是估。

---

## 七、待办（按优先级）

### P0 — 等你拍板的两件事

1. **Windows 可执行文件里的开发机路径**：`grep -a` 能找到 `C:\ltr-src\main.cpp` 等 MSVC 断言用的 `__FILE__`，
   以及 vcpkg 预编译 Boost.JSON 里打包的绝对路径。运行时不读它们，但确实在二进制里。
   清掉第一种要加 `/pathmap`（只影响 Windows 构建）；第二种得重建 vcpkg 依赖。
2. **许可证不一致**：仓库根 `LICENSE` 是 MIT，而每个源文件头写 LGPL-3.0，README 没有 License 段。
   二进制按 GPL-3.0-or-later 分发（被 CGAL 的 GPL 传染）是对的，但**源码侧的口径要你定**。

### P1

3. **网站下载页挂上这个 Release**：`inception.work/littletiles` 的下载区目前读的是网站自己的
   `lt_read_download` 数据，不是 GitHub。要不要把三个附件链接 + 校验和接进去由你决定（**网站线目前暂停**）。
4. **客户端找 CLI 的路径**：`minecraft-littletiles-reader-app` 现在按 `<库仓库>/cmake-build-debug/LittleTilesReader`
   找可执行文件（开发路径）；发行包布局是 `LittleTilesReader-0.2.0-beta-<平台>/`。App 自己打包时会带上 CLI 副本，
   所以只影响"用仓库构建去跑"的场景。

### P2

5. 模组方块的支持（现在以原版方块 + LittleTiles 结构为主）；1.12.2 之外版本。
6. 三平台构建目前是**三台机器手工跑**（mac 本机 / Windows 本机 / Linux 虚拟机）。要做 CI 得先解决
   vcpkg 资产缓存与 macOS 签名。

---

## 八、踩过的坑（省得再踩一遍）

1. **跨平台分支**：`#ifdef __APPLE__` 漏了 Linux，Linux 上 nbt++ 的数组取法会走错分支；`FloatType` 不一致会让
   三平台几何结果对不上。
2. **mac 打包顺序**：`install_name_tool` 改完必须重新 `codesign`，反过来签名失效；改完还要
   `-delete_rpath`，否则包里带着开发机绝对路径。
3. **Windows 免运行库**：只有 vcpkg 静态 triplet（`/MT`）才不需要 VC++ 运行库；`dumpbin /dependents` 是唯一可信的验证。
4. **Windows 构建机无外网**：libnbt++ 用仓库 `3rdparty/CMakeLists.txt` 里已写好的
   `FETCHCONTENT_SOURCE_DIR_LIBNBTPLUSPLUS` 离线钩子。
5. **服务器上的生成器路径**：默认值是错的，且环境变量名必须是 `WORK_READER_BINARY`；写错会静默不生效，
   表现是每一条都 `生成失败`。
6. **（如果之后要动网站）** 两件事会让人怀疑"我改了没用"：`index.html` 被浏览器缓存（现在由 nginx
   `no-cache` + `/version.json` 自愈解决），以及路由 **alias**（老路径能匹配组件但不在菜单树里，会被
   菜单鉴权拦下）——网站侧的排障记录在 `inception-work/docs/ai/服务器环境记录.md`。

---

## 九、工作约定（站长明确要求过）

- **提交信息用中文**，写清"改了什么 + 实测数字"；不要"应该可以"这种话。
- 汇报要给**实测数值**（顶点/面数/耗时/字节数/哈希），给不出就说给不出。
- 动 **App 仓库不要 push**；动 **网站仓库**前先问（那条线由站长自己/另一个任务在推）。
- 只在自己项目目录、给定的测试目录、以及允许的下载目录里操作；服务器上别碰其它站点。
- 干完活收尾前跑一次 `/Users/external_elliott/DevelopmentTestFolder/tools/codex-notify.sh` 让机器出声提醒站长。

---

## 十、文档索引

| 文档 | 内容 |
|---|---|
| [README.md](../README.md) / [README.zh-CN.md](../README.zh-CN.md) | 项目介绍与上手 |
| [job.md](job.md) | job JSON 全字段 |
| [snbt-format.md](snbt-format.md) · [nbt-format.md](nbt-format.md) | SNBT / NBT 结构 |
| [architecture.md](architecture.md) | 几何管线（半空间裁剪、不做布尔） |
| [assets-package.md](assets-package.md) · [texture-mapping.md](texture-mapping.md) | 素材包与贴图映射 |
| [benchmark.md](benchmark.md) | 各测试存档的基准数字 |
| [known-issues.md](known-issues.md) | 已知问题 |
| **[build-guide.md](build-guide.md)** | **三平台构建与发版操作手册（唯一的构建口径）** |
| [release-and-licensing.md](release-and-licensing.md) | 许可证与合规依据、发布说明模板 |
| [release-notes-v0.2.0-beta.md](release-notes-v0.2.0-beta.md) | 本次发布的对外说明 |
| `D:\DevelopmentProject\ltr-release\{FINAL-REPORT.md, SMOKE-TEST-windows-x64.txt}` | Linux 与 Windows 的构建＋冒烟实测报告 |
| `inception-work/docs/ai/模型共享-需求清单.md` | 网站侧模型共享的需求与验收口径（那条线的唯一口径） |
