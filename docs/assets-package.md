# AssetsPackage：素材组合契约（草案 v2）

> 状态：设计草案，未实现。v2 按"生成端 / 宿主 / 库"三角色重写，
> 并把 §7 的"已实现部分"从设计项里摘出来——有三条前提在 v1 里被写成了待办，实际代码已完成。
> 目标：让 Python 管素材组合、宿主负责交互与文件来源、C++ 库只认"一个合法的素材目录 + 一个统一接口"。

---

## 1. 职责分层（三个角色）

```
┌──────────────── 生成端（Python，tools/*.py）────────────────┐
│  选型：原版 / 原版+资源包 / 原版+模组 / 任意叠加闭包           │
│  能力：解 jar、解析 blockstate/models、抠纹理、归一化、去重、缓存│
│  产物：一个素材目录 = 映射表 + 只读纹理源 + （可选）manifest    │
└──────────────────────────────┬──────────────────────────────┘
                               │ 只读源纹理 + 映射表，不产成品 PNG
                               ▼
┌──────────────── 宿主（UI 程序，独立于本库）──────────────────┐
│  决定：用哪个素材目录、输入文件从哪来、产物写到哪、怎么显示进度 │
│  负责：文件来源与可信性、交互问答、用户可见的输出与日志         │
└──────────────────────────────┬──────────────────────────────┘
                               │ 传目录 + 传输入 + 指定输出根
                               ▼
┌──────────────── 库（C++ 核心，Galib）───────────────────────┐
│  Open(dir) 校验 → 查表 → 结合网格烘焙 → OBJ/PNG/MTL          │
│  tile_color / tint 的最终调制在库内完成（依赖网格数据）        │
└──────────────────────────────────────────────────────────────┘
```

**硬性边界**

- **Python 端不烘焙**：tile 颜色是网格层信息，只有库在合并网格时才知道每个 tile 的颜色。
  合成 PNG（tint × tile 色）必须在库内做。Python 只提供"源纹理选择集 + 归一化映射表"。
- **库端不解析素材源**：库不读 jar、不解析 blockstate、不抠纹理。它只消费已归一化的目录。
- **库端不管来源与可信性**：目录是谁给的、从哪下载的、能不能信，全部是宿主的职责。
  库不做路径安全加固，也不校验"这个包该不该被处理"。
- **库端不提问、不决定输出位置**：交互全在宿主；输出根由宿主传入（见 §4.3）。

---

## 2. 产物：素材目录布局

```
assets_package/
├── manifest.json            # 可选；库只读 format_version（见 §3）
├── block_textures.tsv       # 归一化映射表：block(+meta) → 六面贴图 + tintindex
├── block_ids.tsv            # save 模式用的数字 id 表（从原版复制）
├── tint.tsv                 # 可选；tint 覆盖表（缺省用库内默认色，见 §4.1）
└── textures/
    └── <任意相对路径>.png    # 只读源纹理
```

`block_textures.tsv` 列顺序（与 `tools/resolve_block_textures.py` 的 `FACES` 一致）：

```
<block 或 block:meta>  <down> <up> <north> <south> <west> <east>
                       <down_tint> <up_tint> <north_tint> <south_tint> <west_tint> <east_tint>
```

- **纹理引用是不透明的相对路径，不强制 `<namespace>/` 前缀。**
  库的解析规则只有一条：`<dir>/textures/<path>.png`（`-` 表示该面无贴图）。
  现存三份素材的布局并不一致，这条规则让它们可以共存：

  | 素材包（示例） | 纹理引用的样子 | 实际落点 |
  |---|---|---|
  | `1.12.2`（纯原版） | `blocks/stone` | `textures/blocks/stone.png` |
  | `pack_v14`（原版+资源包） | `blocks/stone` | `textures/blocks/stone.png` |
  | `pack_snbt`（原版+模组） | `kirosblocks/blocks/bamboo_block` | `textures/kirosblocks/blocks/bamboo_block.png` |

  最后一份的 `textures/<ns>/blocks/...` 是 `add_mod_textures.py` 有意做的（避免模组与
  原版同名贴图撞车），但它是**生成端的实现选择**，不是库的契约。
  这三份素材包本身也不随库分发——库只认目录布局，不关心它们放在哪。
- **库把 `textures/` 视为只读源**，烘焙产物写到宿主指定的输出根，绝不写回这里。
- tint 列可选：旧表只有 7 列时全部按"不染色"处理（现有 `LoadFromTsv` 已是这个行为）。

---

## 3. manifest.json

**库只读一个字段：`format_version`**（读到不认识的版本要报错退出，而不是静默少读一半）。
其余字段是**生成端记账 + 给人看**的，库必须忽略未知字段。

```json
{
  "format_version": 1,
  "texture_layout": "<相对路径>.png",
  "layers": ["vanilla", "resourcepack", "mods"],
  "packs": [
    { "id": "vanilla",     "version": "1.12.2", "namespaces": ["minecraft"] },
    { "id": "kirosblocks", "version": "1.2.2",  "namespaces": ["kirosblocks"] },
    { "id": "littletiles", "version": "1.5.66", "namespaces": ["littletiles"] }
  ]
}
```

- `layers` / `packs` **不由库消费**。层序在 Python 合并的那一刻就已经落地了，
  库看到的永远是合并后的成品；库没有"按层叠加"这个动作（§4.1 的接口里也没有）。
  把它们放进 manifest 是为了让人和生成端能回溯这份目录是怎么来的。
- 现存 `pack_v14/pack-source.txt` 就是同一件事的雏形，manifest 应吸收它。
- **版本纪律**：只增字段、不改语义；新增字段时 `format_version` 递增。

---

## 4. 库侧统一接口

### 4.1 素材门面（取代散装的 `assets_root + "/..."` 拼装）

```cpp
// 现状：ObjExportOptions::assets_root 被 main.cpp / CgalLittletilesBuilder.cpp
// 各自拼路径（"+ /block_textures.tsv"、"+ /textures/" + path）。收敛成一个门面。
class AssetsPackage {
 public:
  // 失败返回 nullopt 并填错误串；不抛异常出库边界。
  static std::optional<AssetsPackage> Open(const std::string& kDir,
                                           std::string* p_desc_error);

  // 6 面贴图 + tintindex（-1 = 不染色）
  bool Lookup(const std::string& kBlockId,
              texture_support::BlockFaceTextures* p_desc_out) const;

  // 只读源：<dir>/textures/<rel>.png，不做路径安全校验（见 §1 边界）
  std::filesystem::path ResolveTexture(const std::string& kRel) const;

  // 某个方块在某个 tintindex 下应乘的固定颜色；表里没有时回落到库内默认值。
  // 键必须带 block：tintindex 单独不足以定色（redstone 的 tintindex 0 不是草色），
  // 现状把"非树叶一律当草色"是一种近似（见 docs/texture-mapping.md §6.2）。
  bool TintOverride(const std::string& kBlockId, int kTintIndex,
                    std::uint32_t* p_desc_argb) const;

  int format_version() const;
};
```

### 4.2 MaterialManager（结构化去重 + 解码缓存）

```cpp
// 去重键是结构化的 (texture, tint, tile_color)。结构化键的收益是消除理论撞名
// （纹理名形如 "grass_top_t91bd59" 时会与 grass_top + tint 撞名），
// 以及不再拿材质名当文件名——撞名时加序号，而不是把两种材质静默并成一个。
//
// 真正的性能收益在解码缓存：同一张源贴图只解码一次，各变体从缓存出图。
class MaterialManager {
 public:
  explicit MaterialManager(const AssetsPackage& kPackage);

  std::size_t Claim(const std::string& kTextureRel, std::uint32_t kTintRgb,
                    std::uint32_t kTileColorArgb);   // 返回材质下标

  // 输出目录在写出时传入，而不是构造时：缺省目录由 OBJ 路径推导，
  // 只有写出时才确定下来（见 §4.3）。
  std::size_t WriteTextures(const std::string& kOutputDir, std::string* p_desc_error);
  bool WriteMtl(const std::string& kMtlPath, const std::string& kMapKdPrefix,
                std::string* p_desc_error) const;

  std::size_t decode_count() const;   // 诊断：实际发生的源贴图解码次数
};
```

### 4.3 输出根由宿主指定

实现：`ObjExportOptions::material_output_dir`。留空时沿用从 OBJ 路径推导的
`<obj 名>_textures/`（见 `400b5c3`，本来就**不写回素材目录**）；指定后 PNG 写到
该目录，`map_Kd` 自动写成"MTL 所在目录 → 该目录"的相对路径。
CLI 宿主用环境变量 `LITTLETILES_MATERIAL_DIR` 转发这个选项。

### 4.4 库不打印（接口形态方向，非本阶段实施）

库现在自己往 stdout/stderr 写：`GalibNamespaceDef.h` 里的 `GALIB_DEBUG` 无条件 `#define`
（原本只是调试开关），库源码里有 22 处直接输出
（其中 `CgalLittletilesBuilder.cpp` 一个文件占 17 处）。被 UI 调用时这些会漏进宿主控制台。

方向是：进度用回调发出（谁显示由宿主决定），错误用返回值带出。
`galib::SetProgressEnabled()` 目前只是一个全局开关，是这条路上的临时形态。

---

## 5. 材质管理：实测数据与结论

### 5.1 为什么必须烘焙进像素

OBJ/MTL 的多数导入器（含 Blender）**不会**把 `Kd` 与 `map_Kd` 相乘，
所以颜色必须乘进像素——具体依据见 `docs/texture-mapping.md` §4。
**不要**为了减少贴图数量改走"共享基础图 + 颜色走 Kd"，那会让颜色在 Blender 里丢失。
（GLB 的 `baseColorFactor` 不受此限，留给将来的 GLB 出口。）

### 5.2 实测：贴图放大倍数

对 `outputs/chunk/marge_obj_from_chunk_-7_-26_to_13_-6.*`（真实大范围导出）统计：

| 指标 | 数值 |
|---|---|
| 材质数 | 143 |
| 唯一源贴图 | 49 |
| 唯一 tile 颜色 | **91** |
| 单贴图最多变体 | `blocks/glowstone` → **70 张** |
| MTL 文件大小 | 23.8 KB |
| 烘焙出图 | 143 张 / 9.9 MB |

两个结论：

1. **"MTL 臃肿" 不成立**：143 个 `newmtl` 只有 23.8 KB。真正冗余的是
   143 次 PNG 解码+编码（其中 70 次是同一张 glowstone），以及用 HD 材质包时
   70 × 1024² 的体积与耗时。**所以优化目标是解码缓存，不是缩小 MTL。**
2. **"颜色数量少、不会爆炸" 也不成立**：`docs/texture-mapping.md` §4.1 引用的
   "16 组合 / 6 颜色" 是 `test_region` 小样本；这份真实导出有 91 种颜色。

### 5.3 可选的第二个杠杆

91 种颜色里有一串是极小步长的渐变（`ff001b00` / `ff001e00` / `ff002700` …）。
若颜色数继续增长，可以对颜色做量化合并，但会有可见色带，需单独评估后再做。

---

## 6. 落地里程碑

按依赖排序，逐步切分。**M0–M2 已实现**（2026-09-13）：

- **M0 ✅** `AssetsPackage::Open` 门面 + `format_version` 校验 + 格式级 lint。
  lint 输出：方块数、引用贴图数、缺失贴图数与示例；打开失败带原因。
  实测：`pack_v14` 455 方块 / 301 贴图 / 缺 0；`pack_snbt` 650 方块 / 462 贴图 / 缺 0。
- **M1 ✅** `MaterialManager` 结构化键 + **同纹理只解一次的解码缓存**。
  实测（改接口前后逐字节一致，见 §7）：CoronaSign 27 → 6 次解码，
  SHB_05 28 → 14 次；大样本 97 → ≤49 次（其中 `blocks/glowstone` 一张占 70 次）。
- **M2 ✅** 输出根由宿主指定（`ObjExportOptions::material_output_dir`）。
- **M3 ⏳** tint 表外置：`tint.tsv` 的读取已经实现（键 `(block, tintindex)`，
  支持去掉 meta 后的回退与 `*` 通配），缺省仍回落库内默认色
  （`AssetsPackage.cpp` 的 `kDefaultGrassColor` / `kDefaultFoliageColor`）。
  待生成端为素材包补上 `tint.tsv` 后即可删掉这两个常量。

**本阶段明确不做**（各自属于宿主或后续阶段）：

- 线程化 / 线程安全改造：库当前不是线程安全的（`docs/architecture.md` §5），
  但本阶段不处理，也不因为"将来要多线程"而改动接口。
- 服务器化、并发任务调度、缓存与数据源隔离。
- 不可信输入加固（路径穿越校验等）：素材目录由宿主提供，由宿主负责。
- glTF / GLB 出口（它会让 §5.1 的烘焙约束消失，属独立议题）。

### 待定（需要拍板）

1. **tint 表的键形**：当前实现是 `(block, tintindex)`，且"非树叶一律按草色"
   仍是近似（红石的 tintindex 0 不是草色）。要不要改进成具名色槽
   （`grass` / `foliage` / `redstone` …）？后者更贴近 MC 语义，但要映射表多一列。
2. **颜色量化（§5.3）是否做**：可以一次性压掉大部分重复变体，但会引入可见色带。

---

## 7. 现状对照（哪些已经实现）

v1 把下面三条写成了待办，实际代码已完成：

| v1 的说法 | 代码现状 | 证据 |
|---|---|---|
| 烘焙产物写回素材目录，污染素材 | **早已隔离**：`TextureBaker` 只读素材（`image.Load(assets_root_+"/textures/"+...)`），写出走调用方传入的路径；产物落在 OBJ 旁边的 `<obj名>_textures/` | `400b5c3`，`docs/known-issues.md` §2.5 |
| Python 组合包是"待建能力" | **已实现两个**：`build_assets_from_pack.py`（原版+资源包 → `pack_v14`）、`add_mod_textures.py`（原版+模组 → `pack_snbt`） | 两个脚本的 docstring 与产物目录 |
| 靠路径命名空间"侥幸"避免撞车，无结构保证 | **一半不成立**：`pack_snbt` 的 `<ns>/blocks/...` 布局是有意设计；但确实没有 manifest/版本保证，且 `pack_v14` 仍是扁平布局 | `data/assets/pack_snbt/textures/` 与 `pack_v14/textures/` |

### 7.1 本次实现（2026-09-13）与验证

代码：

| 改动 | 位置 |
|---|---|
| 新增素材门面 | `Galib/.../TextureSupport/AssetsPackage.{h,cpp}` |
| 新增材质管理 | `Galib/.../TextureSupport/MaterialManager.{h,cpp}` |
| 删除 | `TextureSupport/TextureBaker.{h,cpp}`（职责拆给上面两者 + `ApplyTintAndTileColor`） |
| builder 改用门面 | `CgalLittletilesBuilder.cpp`（不再自己拼 `assets_root + "/..."`） |
| 宿主传输出目录 | `ObjExportOptions::material_output_dir`，CLI 用 `LITTLETILES_MATERIAL_DIR` |
| 宿主侧 lint 输出 | `main.cpp` 打开素材包后打印方块数 / 贴图数 / 缺失清单 |

验证方式：改接口**前后各跑一遍同一批用例，逐文件比 md5**。

| 用例 | 素材 | 面数 | 材质 | 产物文件 | 比对结果 |
|---|---|---|---|---|---|
| `test_region` (0,0) 半径 1（9 区块） | `pack_v14` | 4940 | 6 | 8 | **8/8 完全一致** |
| `CoronaSign.txt` | `pack_snbt` | 6106 | 27 | 29 | **29/29 完全一致** |
| `SHB_05_Contemporary_Style_House_v1.1.txt` | `pack_snbt` | 105727 | 32 | 34 | **34/34 完全一致** |

即：这一轮是纯重构，输出（含 OBJ/MTL/PNG 字节）没有变化。

仍然保留的散装点：

- `main.cpp` 的 `DetectAssetsRoot` 里硬编码 `pack_names = {"pack_snbt", "1.12.2"}`。
  这是 CLI 宿主的默认探测策略，宿主本来就可以自己决定用哪个包；
  不算越界，但换包时要改代码。

---

## 8. 这个文档自身

- 本文件是**契约草案 v2**，供评审。确认后按 §6 落 M0–M3。
- 相关现有实现：
  `Galib/.../TextureSupport/{AssetsPackage,MaterialManager,BlockTextureTable,PngImage}`、
  `Galib/.../CgalSupport/CgalLittletilesBuilder.cpp`（材质段）、
  `Galib/include/GalibNamespaceDef.h`（`GALIB_DEBUG`）、
  `main.cpp`（素材目录探测、CLI 交互）。
- 生成端参考（**不随库分发**，在生成端/测试数据包里）：`tools/resolve_block_textures.py`、
  `tools/add_mod_textures.py`、`tools/build_assets_from_pack.py`、
  `tools/generate_block_id_table.py`。
- 相关文档：`docs/texture-mapping.md`（UV 与染色依据）、
  `docs/architecture.md` §5（线程安全现状）、`docs/known-issues.md`（基线）。
