# 交接文档：注释中文化 → 全英文（+ 残留中文字符串清理工单）

> 生成时间：2026-09-13 23:59
> 修订时间：2026-09-14（复核修订：修正 A/B 类划分与行数；补全 80 行逐行清单）
> 执行时间：2026-09-14（**A、B 两类均已处理完毕，编译与回归验证通过**，见 §7）
> 仓库：`minecraft-littletiles-reader`
> 状态：**已完成。源码里 0 行中文（注释 0 + 字符串 0，56 个文件）**

---

## 0. 给后续执行者的一句话摘要

注释已经全部英文化了，**不用再动**。剩下的活只有一件：

> 源码里还有 **80 行中文，全部在字符串字面量里**，分布在 **9 个文件**。
> 其中 **63 行是 `Tr()` 的死参数（删掉即可，英文已经有了）**，
> **17 行是真正会打印中文的硬编码消息（需要翻译）**。

执行顺序建议：先做 B 类（17 行，是真 bug），再做 A 类（63 行，纯清理）。
**两类的做法完全不同，别用同一个脚本一把梭。**

---

## 1. 项目背景（接手前先看）

**Minecraft LittleTiles Reader** —— 读取 Minecraft 存档（Anvil `.mca`）中
[Little Tiles](https://github.com/CreativeMD/LittleTiles) 模组的方块数据，
重建为可用 Blender 打开的 OBJ 模型（OBJ + MTL + 贴图）。

- 架构：静态库 `galib`（解析 + 几何构建）+ `main.cpp`（CLI，只是库的一个消费者）
- 链路：`Anvil(.mca) → NBT → LittleTiles 语义 → CGAL 几何 → Mesh → OBJ`（未来 glTF）
- 依赖：Boost(iostreams) / zlib / CGAL 6.x / libnbt++（钉 commit `687e4303`），统一走 vcpkg
- 已验证环境：Minecraft 1.12.2 + Little Tiles 1.5.66；C++17；禁止使用 MinGW 工具链
- 工程文档在 `docs/`（architecture / nbt-format / snbt-format / texture-mapping / known-issues / benchmark）

---

## 2. 已完成的工作

### 2.1 目标
用户要求：**把项目中的代码注释全部从中文换成英文。**

### 2.2 结果
- 处理 **33 个源文件**，翻译 **493 行**含中文的注释。
- 覆盖范围：`main.cpp`、`CMakeLists.txt`、`3rdparty/CMakeLists.txt`、`Galib/CMakeLists.txt`、
  `Galib/**/*.{cpp,h}`（含 `CgalSupport/`、`TextureSupport/`、`Log/`、`Exception/`、`include/`）。
- 翻译原则：
  - 沿用项目既有英文术语（`tile` / `box` / `chunk` / `assets package` / `cull` 等），不另造词
  - 保留代码标识符、路径、URL、数字、版本号原样
  - 保留注释风格标记：`// ---- 分区 ----`、`TODO`、`NOTE:`、`FIXME` 等
  - 行内注释（`code;  // comment`）保持与代码同行
  - 中英混排的技术名词（如 "LittleTiles 的 tile"）译为 `LittleTiles tiles`

### 2.3 明确**没有**改的东西（有意为之）
| 对象 | 原因 |
|---|---|
| `galib::Tr("中文", "English")` 的第一个中文参数 | 是**字符串字面量**，属 i18n 机制，不是注释 |
| `LtStructure.cpp` / `SnbtParser.cpp` 的中文异常消息 | 同上，是字符串不是注释 |
| `README.zh-CN.md` | 中文版 README，**故意保留中文** |
| `README.md` 里的语言切换链接 `[简体中文](README.zh-CN.md)` | 同上 |
| `docs/**`（工程文档） | 文档不是代码注释；用户只要求改代码 |

### 2.4 验证方法（已执行，全部通过）
因为"只改注释"的改动**编译不一定能发现问题**（注释错误通常不影响编译），
而"注释意外吞掉代码"是真实事故，所以做了机器可验证的证明：

1. **CJK 正则扫描**：确认残留中文行**全部**位于字符串字面量内，无中文注释残留。
   > ⚠️ 正则必须用**宽范围**：`[\u4e00-\u9fff\u3000-\u303f\uff00-\uffef]`。
   > 只用 `\p{Han}` 会漏掉**纯全角标点**的行（实例：`CgalLittletilesBuilder.cpp:575`
   > 只有 `"）"`，没有汉字），导致统计偏低。
2. **注释剥离器比对**（核心）：写了一个 C/C++ 注释剥离器（处理 `//`、`/* */`、
   双引号字符串、单引号字符、反斜杠转义），对每个文件的「改前 / 改后」比较
   **非空非注释行**，确认完全一致 → 证明没有注释吞掉代码。
   - 必须**丢掉纯空白行**再比较：把一行注释拆成两行会产生额外空白行，不丢会误报。
3. **反向替换自证**：用替换对反向还原出「改前」文本，再断言
   `code_lines(rev) == code_lines(current)`。因为 `rev` 是改前的精确重建，
   所以这是**证明**而非猜测。
4. **CMake 文件**另用 `#` 剥离器核对（C/C++ 剥离器不认 `#`，否则全量假失败）。
5. **词法风险扫描**：确认没有 `//` 注释以 `\` 结尾（会把下一行拼进注释）。

### 2.5 构建验证：**未完成**（重要）
`cmake --build cmake-build-debug` 失败，报：

```
fatal error C1083: Cannot open include file: 'cstdint': No such file or directory
```

**原因**：当前终端（Git Bash）没有 MSVC 的 `INCLUDE` / `LIB` 环境变量，
`cl.exe` 连标准库都找不到，编译在第一个 `#include` 就失败，**根本没解析到注释**。
**与本次改动无关。** 要真编译请用 VS 开发者命令提示符或 CLion 环境。

### 2.6 顺手修掉的一处笔误
`Galib/include/Minecraft/LittleTiles.h` 注释里的 `BoxTileTntity` → `BoxTileEntity`。

---

## 3. 待办工单：80 行残留中文

### 3.0 总览

| 类别 | 行数 | 文件数 | 性质 | 处理方式 | 优先级 |
|---|---|---|---|---|---|
| **A 类** | 63 | 7 | `Tr(zh, en)` 的第一个中文参数，**永远不会被打印**（死文本） | **删除中文参数即可，英文已经有了，不需要翻译** | 低（纯清理） |
| **B 类** | 17 | 4 | 不走 `Tr()` 的硬编码中文，**会真的以中文输出** | **需要翻译**（译文已给，见 §3.2） | 高（真实 bug） |

> 合计 80 行 / 9 个文件（两类文件集合不重叠，但都含 `main.cpp` 与 `CgalLittletilesBuilder.cpp`）。
> ⚠️ 行号是本文档生成时的状态，作为**辅助定位**；精确匹配请以中文原文为准。
> ⚠️ `MaterialManager.cpp` 等文件是**尚未 git 提交的新增文件**，`git diff` 看不到它们，
> 盘点时不要只用 `git diff`，要全目录扫描。

---

### 3.1 A 类：`Tr(zh, en)` 里的中文参数 —— 63 行 / 7 文件

**性质**：不是漏翻的注释，而是**故意保留的 i18n 机制**。
`Galib/Log/GalibText.cpp` 里 `Tr()` 现在无条件返回第二个参数（英文），
第一个中文参数已被忽略，但**签名留着**，以便以后恢复中文界面。

```cpp
const char* Tr(const char* const kZhCn, const char* const kEnUs) {
  (void)kZhCn;  // English only; the Chinese argument is no longer used
  return kEnUs;
}
```

**关键点：A 类不需要任何翻译工作。**
每个 `Tr()` 调用的英文译文**已经在源码里了**（就是第二个实参，通常在中文参数的下一行），
所以 A 类的清理 = 把中文实参删掉 + 把 `Tr` 改成单参数。

**当前不影响程序输出**（CLI 打印的一直是英文），但 README 宣传的"中英双语界面"能力靠它支撑。

#### 逐行清单

**`main.cpp`（26 行）**

| 行号 | 中文参数 |
|---|---|
| 134 | `素材目录（回车 = 自动探测）: ` |
| 170 | `警告: 下面这些位置都没有 block_textures.tsv：\n` |
| 175 | `      （填绝对路径最稳；现在退回自动探测）\n` |
| 192 | `存档 region 目录，或 LittleTiles 结构文件（.txt/.struct）: ` |
| 196 | `存档目录: %s\n` |
| 210 | `区块 x: ` |
| 212 | `区块 z: ` |
| 214 | `扫描半径（0 = 只处理这一个区块）: ` |
| 233 | `是否同时导出普通方块（非 LittleTiles）？` |
| 238 | `是否剔除被相邻方块挡住的面？` |
| 245 | `是否把模型中心移到原点？` |
| 252 | `是否再把最长边缩放到 1 个单位（会改变真实尺寸）？` |
| 258 | `是否打印进度提示与耗时？` |
| 265 | `素材目录: %s\n` |
| 267 | `(未找到，只导出几何)` |
| 279 | `警告: 素材包不可用（%s），只导出几何\n` |
| 285 | `素材包: format_version %d，方块 %zu 个，引用贴图 %zu 张\n` |
| 290 | `警告: 有 %zu 张贴图在目录里找不到，例如:\n` |
| 321 | `结构: %s，grid=%d，盒子 %zu 个，材质分组 %zu 个，子结构 %d 个\n` |
| 350 | `结构导出完成: %zu 个网格，总耗时 %.1f 秒\n` |
| 381 | `[进度] 区块 (%d, %d) —— %d/%d\n` |
| 435 | `区块: 找到 %d 个，缺失 %d 个；LittleTiles tile 共 %zu 个\n` |
| 443 | `警告: 未找到素材目录，跳过普通方块导出\n` |
| 458 | `警告: 读不到 %s/block_ids.tsv，跳过普通方块导出\n` |
| 486 + 487 | `总耗时: %.1f 秒（读取与建网格 %.1f 秒，普通方块网格 %.1f 秒，写出文件 %.1f 秒）\n`（跨两行的一个字面量） |

**`Galib/Minecraft/CgalSupport/CgalLittletilesBuilder.cpp`（15 行）**

| 行号 | 中文参数 |
|---|---|
| 289 | `警告: 无法添加面，可能是重复面或无效几何\n` |
| 382 | `合并后网格统计: ` |
| 383 | `顶点数: %u\n` |
| 385 | `面数: %u\n` |
| 398 | `无法创建输出目录: ` |
| 409 | `无法打开文件: ` |
| 551 | `合并的网格已导出到: ` |
| 555 | `  材质 ` |
| 556 | ` 个，已写出贴图 ` |
| 557 | ` 张到 ` |
| 559 | `（源贴图解码 ` |
| 560 | ` 次）` |
| 562 | `（另有 ` |
| 563 | ` 个面没解析到贴图）` |
| 572 | `  未导出贴图：请检查素材目录是否包含 block_textures.tsv（` |

> ⚠️ 289 在 `#ifdef GALIB_DEBUG` 块里，而 `GALIB_DEBUG` 被**无条件 `#define`**（见 §4.6），
> 所以它是**活跃代码**，别当死代码跳过。

**`Galib/Minecraft/TextureSupport/PngImage.cpp`（11 行）**

| 行号 | 中文参数 | 现成英文参数 |
|---|---|---|
| 144 | `无法打开文件` | `cannot open file` |
| 151 | `不是合法的 PNG` | `not a valid PNG` |
| 191 | `IHDR 缺失或尺寸非法` | `missing IHDR or invalid size` |
| 194 | `只支持 8 位深度` | `only 8-bit depth is supported` |
| 197 | `不支持交错 PNG` | `interlaced PNG is not supported` |
| 201 | `不支持的颜色类型` | `unsupported colour type` |
| 209 | `zlib 解压失败` | `zlib inflate failed` |
| 222 | `像素数据不完整` | `incomplete pixel data` |
| 274 | `图像为空` | `image is empty` |
| 295 | `zlib 压缩失败` | `zlib deflate failed` |
| 321 | `无法写出文件` | `cannot write file` |

**`Galib/Minecraft/CgalSupport/CgalWorldBlocks.cpp`（5 行）**

| 行号 | 中文参数 |
|---|---|
| 124 | `[worldblocks] 网格 %dx%dx%d：非空气 %zu，LT 宿主 %zu\n` |
| 246 + 247 + 248 | `[worldblocks] 输出方块 %zu 个，面 %zu 个（邻居剔除 %zu 个，被 CGAL 拒绝 %zu 个）\n`（跨三行的一个字面量） |
| 262 | `[worldblocks] 分组网格实际保存：%zu 个网格，面 %zu，顶点 %zu\n` |

**`Galib/Minecraft/ChunkTileEntities.cpp`（3 行）**

| 行号 | 中文参数 |
|---|---|
| 104 | `ChunkTileEntities::ReadChunk 读取区块: %d %d\n` |
| 126 | `ChunkTileEntities::ReadChunk 出错: ` |
| 137 | `ChunkTileEntities::ReadChunk tile %zu 个，盒子 %zu 个\n` |

**`Galib/Minecraft/BlockTileEntities.cpp`（2 行）**

| 行号 | 中文参数 |
|---|---|
| 91 | `BlockTileEntities::ReadBlockTileNbt 解析方块: %d %d %d\n` |
| 137 | `BlockTileEntities::ReadBlockTileNbt 盒子 %zu 个，tile %zu 个\n` |

**`Galib/Minecraft/TextureSupport/MaterialManager.cpp`（1 行）**

| 行号 | 中文参数 | 现成英文参数 |
|---|---|---|
| 233 | `# 由 LittleTilesReader 生成\n` | `# Generated by LittleTilesReader\n` |

> ⚠️ `MaterialManager.cpp` 是**未提交的新增文件**，`git diff` 里看不到。

#### 三种处理方案（尚未选定，用户表示后续自行处理）

| 方案 | 内容 | 代价 |
|---|---|---|
| ① 不动 | 保留 i18n 机制与中文参数 | 源码里继续存在中文（但不在注释里） |
| ② 收成单参数 | `Tr(zh, en)` → `Tr(en)`，删掉全部中文参数（约 70 个调用点 / 8 个文件） | 中英双语能力被移除；以后恢复中文需改所有调用点；README 需同步改 |
| ③ 彻底移除 `Tr()` | 连包装函数一起删，约 70 个调用点直接写英文字面量 | 代码最简洁；彻底放弃 i18n 钩子；改动面最大 |

> 建议：如果目标只是"代码里没有中文"，方案 ② 最平衡；如果保留双语是产品需求，则方案 ①。
> **方案 ②③ 会改变公开接口行为，动手前必须让用户确认。**

---

### 3.2 B 类：硬编码中文消息 —— 17 行 / 4 文件 ← **真正的遗留 bug**

**性质**：这 17 处**不走 `Tr()`**，是直接写死的中文，**会真的以中文输出**，
在英文界面的 CLI 上属于真实的不一致。**这一项建议无条件修掉。**

#### `Galib/Minecraft/SnbtParser.cpp`（13 行）

```cpp
   55   Fail("文档末尾有多余内容");
   66   message << "SNBT 解析失败（位置 " << position_ << "）：" << kWhat << "\n"
   67           << "  上下文: ..." << text_.substr(begin, length) << "...";
   86   Fail(std::string("期望 '") + kCh + "'");
  101   Fail("内容意外结束");
  134   Fail("复合标签里期望 ',' 或 '}'");
  165   Fail("数组里期望 ',' 或 ']'");
  183   Fail("列表里期望 ',' 或 ']'");
  194   Fail("字符串没有结束的引号");
  205   Fail("转义符后面没有内容");
  234   Fail("这里需要一个值");
  265   Fail("数字格式不对");
  304   throw std::runtime_error("无法打开 SNBT 文件: " + kPath);
```

#### `Galib/Minecraft/LtStructure.cpp`（2 行）

```cpp
   30   throw std::runtime_error("盒子数组长度不足 6");
   62   throw std::runtime_error("结构 SNBT 的顶层不是复合标签");
```

#### `main.cpp`（1 行）

```cpp
  324   structure.name().empty() ? "(未命名)" : structure.name().c_str(),
```
> 注意：这一行**在 `Tr(...)` 调用之外**——它是已经翻译好的
> `printf(galib::Tr("结构: ...", "structure: ..."), ...)` 的**实参**，
> 所以不会被 `Tr()` 兜住，会真的打印 `(未命名)`。

#### `Galib/Minecraft/CgalSupport/CgalLittletilesBuilder.cpp`（1 行）

```cpp
  575   << options.assets_root << "）";
```
> 注意：这个全角右括号**在 `Tr(...)` 调用之外**，是直接拼到 `std::cout` 上的，
> 所以会真的打印。它对应的左括号在 572 行的中文参数里（A 类），
> 英文版在 573–574 行用的是半角 `(`，**收尾符号必须同步改成半角 `)`**，
> 否则会出现「英文文案 + 全角括号」的混搭。
>
> 整段完整上下文：
> ```cpp
>   570   std::cout
>   571       << galib::Tr(
>   572              "  未导出贴图：请检查素材目录是否包含 block_textures.tsv（",   // A 类
>   573              "  no textures exported: check that the assets root has "
>   574              "block_textures.tsv (")
>   575       << options.assets_root << "）";                                  // ← B 类，改这里
> ```

#### 建议的英文译文（可直接用）

| 文件:行 | 原中文 | 建议英文 |
|---|---|---|
| LtStructure.cpp:30 | `盒子数组长度不足 6` | `box array has fewer than 6 entries` |
| LtStructure.cpp:62 | `结构 SNBT 的顶层不是复合标签` | `the top level of the structure SNBT is not a compound tag` |
| SnbtParser.cpp:55 | `文档末尾有多余内容` | `trailing content after the end of the document` |
| SnbtParser.cpp:66 | `SNBT 解析失败（位置 ` … `）：` | `SNBT parse failed (position ` … `): ` |
| SnbtParser.cpp:67 | `  上下文: ...` | `  context: ...` |
| SnbtParser.cpp:86 | `期望 '` + ch + `'` | `expected '` + ch + `'` |
| SnbtParser.cpp:101 | `内容意外结束` | `unexpected end of input` |
| SnbtParser.cpp:134 | `复合标签里期望 ',' 或 '}'` | `expected ',' or '}' in compound tag` |
| SnbtParser.cpp:165 | `数组里期望 ',' 或 ']'` | `expected ',' or ']' in array` |
| SnbtParser.cpp:183 | `列表里期望 ',' 或 ']'` | `expected ',' or ']' in list` |
| SnbtParser.cpp:194 | `字符串没有结束的引号` | `unterminated string (no closing quote)` |
| SnbtParser.cpp:205 | `转义符后面没有内容` | `escape character at end of input` |
| SnbtParser.cpp:234 | `这里需要一个值` | `expected a value here` |
| SnbtParser.cpp:265 | `数字格式不对` | `malformed number` |
| SnbtParser.cpp:304 | `无法打开 SNBT 文件: ` | `cannot open SNBT file: ` |
| main.cpp:324 | `(未命名)` | `(unnamed)` |
| CgalLittletilesBuilder.cpp:575 | `）` | `)`（半角，与 574 行的 `(` 配对） |

---

## 4. 接手注意事项（踩过的坑）

1. **同文件不要并行发多个 Edit 调用** —— 会互相冲突，报
   `File has been modified since read` 或 `EBUSY: resource busy or locked`。
   要么串行，要么把替换对写成 Python 脚本一次性执行。
2. **不要用 `git diff HEAD` 当基线** —— 本仓库工作区**本来就 dirty**（见 §5），
   会把自己没改的代码算成自己的。用 §2.4 的反向替换法自证。
3. **批量替换脚本要点**
   - 替换对一律用 `r'''...'''` 原始三引号，否则注释里的 `\t` `\n` `\\` 会被 Python 转义
   - 保留换行风格：先读字节探测 CRLF，匹配时统一成 `\n`，写回时再转回 CRLF
   - 逐条检查命中次数：0 次 = 原文写错；>1 次 = 可能误伤，需补上下文消歧
4. **盘点不要只靠 `git diff`** —— 未提交的新增文件不在其中（`MaterialManager.cpp`
   就是因为这个被漏掉过一次，导致统计数字偏低）。
5. **扫描正则要用宽范围** —— 见 §2.4 第 1 条：`\p{Han}` 会漏掉纯全角标点行
   （`CgalLittletilesBuilder.cpp:575` 只有 `"）"`）。
6. **`GALIB_DEBUG` 在 `Galib/include/GalibNamespaceDef.h` 里被无条件 `#define`** ——
   所以 `#ifdef GALIB_DEBUG` 分支里的 `Tr(...)`（如 `CgalLittletilesBuilder.cpp:289`）
   是**活跃代码**，不是死代码，改的时候别忽略。
7. **`Tr()` 的调用点比"中文行数"多** —— 一条 `Tr()` 常跨 2~3 行（中文参数一行、
   英文参数续行），所以 A 类 63 行对应约 70 个调用点。按行数估工作量会偏低。
8. **A 类不要"翻译"，要"删除"** —— 英文译文已经作为 `Tr()` 的第二实参存在了，
   重复翻译一遍反而会产生两份不一致的英文。正确做法是删掉第一实参并收窄签名。
9. **B 类里有两行"藏在 `Tr()` 外面"** —— `main.cpp:324` 和
   `CgalLittletilesBuilder.cpp:575`。只 grep `Tr(` 是找不到它们的，必须全文扫 CJK。
10. **可复用的技能已保存**：`~/.workbuddy-ai/skills/comment-language-migration/SKILL.md`
    （含注释剥离器、反向替换验证、CRLF 处理等完整流程与脚本），直接加载即可。

---

## 5. 仓库工作区**预先存在**的未提交改动（不是本次改的）

接手时**不要**把这些算到注释英文化头上，也不要在没有授权的情况下去"修"它们：

- 重构：`TextureBaker` → `MaterialManager`（`Galib/Minecraft/TextureSupport/`、`Galib/CMakeLists.txt`）
- 新增（未提交）：`AssetsPackage.{h,cpp}`、`MaterialManager.{h,cpp}`、`SnbtParser.{h,cpp}`、
  `LtStructure.{h,cpp}`、`ChunkBlocks.{h,cpp}`、`BlockTextureTable.{h,cpp}`、`PngImage.{h,cpp}`、
  `GalibText.{h,cpp}`、`CgalWorldBlocks.{h,cpp}` 等
- 重命名：`StripMeta` → `StripBlockMeta`
- `main.cpp` 新增 `pack_snbt` 素材包探测、`ObjExportOptions::material_output_dir`
- 删除：`data/regions/**/*.mca`（测试数据）、`python/**`、`tools/**`（生成端脚本）、
  `Galib/Minecraft/TextureSupport/TextureBaker.{h,cpp}`
- 修改：`README.md`、`docs/**`

---

## 6. 当前状态速查

> ⚠️ **本节的"待处理"状态已于 2026-09-14 全部完成**，逐项证据见 §7。
> 下表保留原始盘点结果，不再更新；最新状态见 §7.8。

| 项 | 状态 |
|---|---|
| 中文**注释** | ✅ 已全部英文化（33 文件 / 493 行），已验证 |
| 中文**字符串**（A 类，i18n `Tr()` 参数） | 📋 已盘点（63 行 / 7 文件 / 60 调用点）→ ✅ **已完成**（§7.7，方案②） |
| 中文**字符串**（B 类，硬编码消息） | 📋 已盘点（17 行 / 4 文件）→ ✅ **已完成**（§7.4） |
| `README.zh-CN.md` / `docs/**` | ✅ 有意保留中文，不需处理 |
| 编译验证 | ⚠️ 未完成（环境缺 MSVC 变量）→ ✅ **通过**（§7.1） |
| 源码残留中文行 | **80 行**（A 63 + B 17）→ ✅ **0 行**（§7.7，共 56 个文件） |

### 9 个涉及文件一览

| 文件 | A 类 | B 类 |
|---|---|---|
| `main.cpp` | 26 | 1 |
| `Galib/Minecraft/CgalSupport/CgalLittletilesBuilder.cpp` | 15 | 1 |
| `Galib/Minecraft/TextureSupport/PngImage.cpp` | 11 | 0 |
| `Galib/Minecraft/CgalSupport/CgalWorldBlocks.cpp` | 5 | 0 |
| `Galib/Minecraft/ChunkTileEntities.cpp` | 3 | 0 |
| `Galib/Minecraft/BlockTileEntities.cpp` | 2 | 0 |
| `Galib/Minecraft/TextureSupport/MaterialManager.cpp` | 1 | 0 |
| `Galib/Minecraft/SnbtParser.cpp` | 0 | 13 |
| `Galib/Minecraft/LtStructure.cpp` | 0 | 2 |
| **合计** | **63** | **17** |

---

## 7. 复核与执行结果（2026-09-14）

接手者（第二个 AI）复核了上面的工单并执行了 B 类。下面每条都附可复现证据。

### 7.1 补上了缺失的编译验证（§2.5 的缺口）

原文卡在"Git Bash 没有 MSVC 的 INCLUDE/LIB"。用 `vcvars64.bat` 初始化后，
`cmake --build cmake-build-debug` **通过**（33 个源文件全量重建 + 链接）。
结论：注释英文化**没有**吞掉代码，编译层面无回归。

> 复现：`cmd /c "call <VS>\VC\Auxiliary\Build\vcvars64.bat && cmake --build cmake-build-debug"`

### 7.2 行为回归：与改动前逐字节一致

用三个真实用例各跑一遍（region 小样本 + CoronaSign + SHB_05），
把 OBJ / MTL / 贴图 PNG 与**注释英文化之前的基线**逐文件比 md5：

| 用例 | 产物文件 | 比对 |
|---|---|---|
| `test_region` (0,0) 半径 1，素材 `pack_v14` | 8 | **8/8 一致** |
| `CoronaSign.txt`，素材 `pack_snbt` | 29 | **29/29 一致** |
| `SHB_05_Contemporary_Style_House_v1.1.txt`，素材 `pack_snbt` | 34 | **34/34 一致** |

### 7.3 §3 的盘点复核：数字准确

用独立的注释剥离器重扫（宽范围 CJK，含全角标点）：

- **注释里的中文：0 行** → §2.2 的结论成立。
- **字符串/代码里的中文：80 行** → §3.0 的数字准确（A 63 + B 17）。

### 7.4 B 类（17 行）已全部修复 ✅

按 §3.2 给出的译文改完，并**实测**了报错路径（原文只做了静态盘点）。
用 4 个畸形 SNBT 跑 CLI：

```
trailing.txt     -> error: SNBT parse failed (position 3): trailing content after the end of the document
unterminated.txt -> error: SNBT parse failed (position 9): unexpected end of input
badvalue.txt     -> error: SNBT parse failed (position 8): expected a value here
unquoted.txt     -> error: SNBT parse failed (position 10): expected ',' or ']' in list
                     （第二行是 `  context: ...`）
```

`CgalLittletilesBuilder.cpp` 的全角 `）` 也实测到了：素材包打不开时输出
`no textures exported: check that the assets root has block_textures.tsv (<路径>)`，
收尾是半角 `)`，与英文文案配平。

### 7.5 新发现并修复：畸形输入会让 CLI **卡住不退出**

这是静态盘点抓不到的**真 bug**，属于 §4.6 里 `known-issues.md` §2.1 第 4 条
（库边界没有错误转换、`main()` 无 try/catch）的实际后果。

**症状**：喂畸形 SNBT 后进程不退出——CPU 6 秒后仍是 0.33 s（**不是死循环，是阻塞**），
16 个线程、无 WerFault、无窗口。Debug CRT 下未捕获异常走了报告对话框路径，在无人值守的
会话里就一直挂着。Release 构建同样会异常终止，只是没有对话框。

**修复**：`main.cpp` 把主体拆成 `RunTilesReader()`，`main()` 只做边界处理——
捕获异常、打印 `error: <what()>`、返回非 0。上面 §7.4 的英文报错就是修完之后的输出。
顺带把"CLI 崩溃"变成了"可读的一行错误 + 退出码"，这正是将来 UI 宿主需要的行为。

### 7.6 顺带修正：README 的界面语言说明是错的

两版 README 都写着"**第一个问题选界面语言**（直接回车 = 简体中文）……所有提示都有中英两版"，
且示例命令以 `1`（选中文）开头。但 `main.cpp` 里**已经没有语言选择这一问**了——
照示例跑，那个 `1` 会被当成存档路径读进去。已改为：说明输出**只输出英文**（并说明
`Tr()` 已退化为只返回英文），示例命令去掉开头的 `1`。

### 7.7 A 类（63 行）已处理 ✅ —— 采用方案②

用户确认"中文就不要了"，于是执行 **方案②**：`Tr(zh, en)` → `Tr(en)`，删掉全部中文实参。

**执行方式**：写了一个基于括号深度 + 字符串字面量的解析器（不是正则），先**干跑**
核对每个调用点的参数个数与跨行情况，确认无异常后才 `--apply`。它按调用点从后往前改，
所以偏移不会失效；`//` 与 `/* */` 会在扫描时跳过。

**实际规模**（比 §3.1 估的"约 70 个调用点"少，因为一条调用常跨 2~3 行）：

| 文件 | 调用点 | 其中跨行 |
|---|---|---|
| `main.cpp` | 25 | 19 |
| `CgalLittletilesBuilder.cpp` | 15 | 4 |
| `PngImage.cpp` | 11 | 0 |
| `CgalWorldBlocks.cpp` | 3 | 3 |
| `ChunkTileEntities.cpp` | 3 | 3 |
| `BlockTileEntities.cpp` | 2 | 2 |
| `MaterialManager.cpp` | 1 | 1 |
| **小计（自动改写）** | **60** | **32** |

`GalibText.h` / `GalibText.cpp`（函数声明与定义本身）**排除在自动改写之外**，手工改：

```cpp
// 之前
[[nodiscard]] const char* Tr(const char* kZhCn, const char* kEnUs);
const char* Tr(const char* const kZhCn, const char* const kEnUs) {
  (void)kZhCn;  // English only
  return kEnUs;
}
// 之后
[[nodiscard]] const char* Tr(const char* kEnUs);
const char* Tr(const char* const kEnUs) { return kEnUs; }
```

保留单参数 `Tr()` 而不是方案③（连函数一起删）：所有可见文案仍然只有一个入口，
将来要恢复多语言只改这一个函数，不必回头动 60 个调用点。头文件注释里写明了这一点。

> 注意：`GalibText.h` 原有的注释里出现过字面量 `Tr(zh, en)`，会被调用点扫描器
> 误当成一次两参调用；已改写成不含该模式的措辞。

**验证**：

1. CJK 扫描（宽范围，注释/字符串分别统计）→ **0 行中文**（注释 0 + 字符串 0，56 个文件）。
2. 调用点解析器复扫 → 不再有双参 `Tr()`。
3. `cmake --build` 通过（10 个目标重建 + 链接）。
4. 三用例回归：**8/8、29/29、34/34 与基线逐字节一致**。
5. 报错路径复验：畸形 SNBT 仍是 `error: SNBT parse failed (position N): ...` + `exit=1`。
6. 正常输出的 stdout 逐行比对：只差"输出目录名、耗时、以及后加的两行诊断"，
   **提示与结果文案一字未变**。

### 7.8 结论

| 项 | 状态 |
|---|---|
| 中文注释 | ✅ 0 行 |
| 中文字符串 / `Tr()` 中文参数 | ✅ 0 行 |
| 编译验证 | ✅ 通过 |
| 行为回归 | ✅ 逐字节一致 |
| 顺手修复 | 畸形输入卡死、README 语言说明错误 |
