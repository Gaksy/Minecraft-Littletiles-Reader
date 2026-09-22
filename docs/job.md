# Job 接口：非交互驱动

本文是**宿主（UI / 脚本 / 服务）驱动 LittleTilesReader 的契约**。
CLI 有两种模式：不带参数时是原来那套交互式提问；带 `--job` 时一次跑完，不问任何问题。

```sh
LittleTilesReader --job <file.json> [--progress json]
LittleTilesReader --convert <1.20|1.12.2> --input <file.txt> [--out <file.txt>]
LittleTilesReader --help
```

| 参数 | 作用 |
|---|---|
| `--job <file.json>` | 读 job 文件跑一次，不再交互 |
| `--convert <版本>` | 结构 SNBT 互转（1.20 ↔ 1.12.2），不导出模型 |
| `--input <文件>` | 要转换的结构文件（配合 `--convert`），`.txt`（1.20 蓝图 `.struct` 同样可读） |
| `--out <文件>` | 转换后的输出文件（可省略，默认写在输入文件旁边；不带扩展名时补 `.txt`） |
| `--progress json` | stdout 只输出 NDJSON 事件（一行一个 JSON），不再输出人看的文本 |
| `--help` | 用法 |

`--progress json` 时**整条 stdout 都是事件流**：库自己的进度打印会被关掉，
builder 的摘要也通过 `ObjExportOptions::quiet` 静音，宿主不需要过滤人话。
错误仍然走 stderr 并以非 0 退出码结束（见 §5）。

## 1. job 文件

```json
{
  "schema": 1,
  "mode": "region",
  "input": {
    "world": {
      "root": "D:/saves/MyWorld",
      "dimension": "overworld"
    },
    "chunks": { "mode": "center", "x": 11, "z": -4, "radius": 1 }
  },
  "assets": { "package": "D:/app/cache/packs/9f2c1a4b" },
  "output": { "dir": "D:/projects/House/outputs/2026-09-14_0031", "name": "house" },
  "options": {
    "plain_blocks": true,
    "cull_hidden_faces": true,
    "center": true,
    "normalize_scale": false
  }
}
```

### mode

| 值 | 含义 | 需要的 input |
|---|---|---|
| `"region"` | 从存档导出（1.12.2 与 1.18+ / 1.20 两种区块布局都支持，见 [`nbt-format.md`](nbt-format.md)） | `input.world` + `input.chunks` |
| `"snbt"` | 从 LittleTiles 结构文件导出 | `input.snbt.path` |
| `"snbt_convert"` | 结构 SNBT 互转（1.20 ↔ 1.12.2），不导出模型 | `input.snbt.path` + `options.target` |

`snbt` 与 `snbt_convert` 都是**自动识别方言**的：1.12.2 的 `tiles` 列表与 1.20 的
`t` 映射都能直接读（见 [`snbt-format.md`](snbt-format.md)）。

### input.world —— 存档**根目录**，不是 region 目录

宿主让用户选的是存档（玩家在文件对话框里认识的那个文件夹），所以这里传的是
**含 `level.dat` 的存档根目录**，由库负责往下找 region。

| 字段 | 说明 |
|---|---|
| `root` | 存档根目录，例如 `.minecraft/saves/MyWorld` |
| `dimension` | `overworld`（默认）/ `nether` / `the_end`；也接受 `minecraft:the_nether` 这类写法 |
| `region_dir` | **可选逃生门**：直接指定放 `.mca` 的目录，跳过下面的解析 |

解析顺序（实现见 `Galib/Minecraft/SaveFolder.cpp` 的 `ResolveRegionFolder`）：

1. `<root>/<维度子目录>`：主世界 `<root>/region`、下界 `<root>/DIM-1/region`、末地 `<root>/DIM1/region`
2. 若上一步没有 `.mca`，则把 `root` 本身当 region 目录（**测试数据没有 `level.dat`，走的就是这条**）
3. 都不行 → 报错退出，消息里带上试过的路径

### input.chunks —— 三种选择模式

| `mode` | 字段 | 展开结果 |
|---|---|---|
| `"single"` | `x`, `z` | 1 个区块 |
| `"range"` | `x1`, `z1`, `x2`, `z2` | 任意矩形，**含两端**，长宽可不等 |
| `"center"`（默认） | `x`, `z`, `radius` | 边长 `2r+1` 的正方形，半径负数按 0 处理 |

坐标就是区块坐标（chunk x / z），和游戏里 F3 显示的一致。

### assets.package

一个**已经组合好的素材包目录**（见 `docs/assets-package.md`）：`block_textures.tsv`
+ `textures/`（+ 可选 `manifest.json` / `tint.tsv`）。留空则只导出几何，不写材质。

组合本身（原版 + 资源包 + 模组的叠加）由生成端负责，不在库这一侧做。

### output

| 字段 | 说明 |
|---|---|
| `dir` | 产物目录。留空则沿用历史相对路径（`../outputs/chunk/` 或 `../outputs/snbt/`，相对当前工作目录） |
| `name` | 输出名（不含扩展名）。留空则按输入推导 |

`dir` 是相对**进程当前工作目录**解析的；宿主应当传绝对路径。
贴图落在 `<obj 名>_textures/`（若 `output.dir` 给了就是 `<dir>/<name>_textures/`），
`map_Kd` 自动写成相对 MTL 的路径。

### options

| 字段 | 默认 | 说明 |
|---|---|---|
| `plain_blocks` | region 模式为 `true`，snbt 模式强制 `false` | 是否同时导出普通方块 |
| `cull_hidden_faces` | `true` | 剔除被相邻方块挡住的面 |
| `center` | `true` | 把包围盒中心移到原点 |
| `normalize_scale` | `false` | 再等比缩放到最长边 = 1（会丢失真实尺寸） |
| `target` | 无（`snbt_convert` 必填） | 目标版本：`"1.20"`（也可 `1.21`/`modern`/`new`）或 `"1.12.2"`（也可 `1.12`/`legacy`/`old`） |

### snbt_convert 的输出

`output.dir` / `output.name` 与 `snbt` 模式同义（`name` 不含扩展名，固定写 **`.txt`** ——
LittleTiles 的结构文件就是纯文本 `.txt`）。
两者都留空时，默认写在**输入文件旁边**，名字为 `<输入名>_<目标版本>.txt`。
转换**不读素材包**：方块名映射用的是库内嵌的 LittleTiles 表（见
[`snbt-format.md`](snbt-format.md) §5）。

## 2. 进度事件

每行一个 JSON 对象，`event` 字段是类型。顺序稳定：`assets?` → `start` → `chunk*`
（或 snbt 的三个 `stage`）→ `done`。

| 事件 | 字段 |
|---|---|
| `assets` | `format_version`, `blocks`, `textures`, `missing` |
| `warning` | `message`（例如素材包打不开） |
| `start` | `mode`（`region` / `snbt` / `snbt_convert`）, `chunks`, `world`, `dimension`, `assets`, `target`（转换模式的目标版本，其它模式为空串） |
| `chunk` | `index`, `total`, `x`, `z` |
| `stage` | `name`：snbt 模式为 `parse` / `mesh` / `write`；snbt_convert 模式为 `parse` / `convert` / `write` |
| `done` | `obj`, `ok`, `chunks_found`, `chunks_missing`, `tiles`, `vertices`, `faces`, `materials`, `textures_written`, `missing_texture_faces`, `seconds` |

snbt 模式的 `done` 只有 `obj` / `meshes` / `seconds`。
snbt_convert 模式的 `done` 是 `snbt`（产物路径）/ `from` / `to` / `boxes` / `tiles` /
`levels` / `renamed_blocks`（被改写的 tile 数）/ `unmapped_names`（没有对应方块名的
**种类**数）/ `unmapped_tiles`（涉及多少个 tile）/ `seconds`；转换过程中无法翻译的内容
（模组方块名、门的行为参数等）以 `warning` 事件给出，人读模式下是 `warning:` / `note:` 行。
进度条吃 `chunk` 的 `index`/`total` 即可；`assets` 与 `done` 用于展示统计。

## 3. 可直接跑的示例

```sh
# 存档：3×3 区块，素材用 pack_v14
LittleTilesReader --job examples/region.json --progress json

# 结构文件
LittleTilesReader --job examples/snbt.json --progress json

# 结构互转（1.12.2 -> 1.20）
LittleTilesReader --job examples/snbt_convert.json --progress json
```

（本仓库的测试数据在兄弟仓库 `minecraft-littletiles-reader-data`，
注意它的 region 目录没有 `level.dat`，所以示例里走的是"root 本身就是 region 目录"那条规则。）

## 4. 输出命名

| 情况 | 文件名 |
|---|---|
| `output.name` 给了 | `<name>.obj` |
| 正方形选择（中心 + 半径） | `marge_obj_from_chunk_<中心x>_<中心z>[_to_<中心x+r>_<中心z+r>].obj` |
| 矩形选择（range） | `marge_obj_from_chunk_<minx>_<minz>_to_<maxx>_<maxz>.obj` |

正方形沿用旧命名是为了不打乱既有文档、基准与用户已有的产物；
矩形是本轮新增的能力，没有"中心 + 半径"的等价形式，所以按角点命名。

## 5. 退出码与错误

| 退出码 | 含义 |
|---|---|
| `0` | 成功 |
| `1` | 失败：job 文件读不了 / 解析不了、输入不存在、导出过程抛异常 |

失败时 stderr 是**一行** `error: <原因>`，例如：

```
error: bad job file: unknown chunks.mode: box
error: SNBT parse failed (position 3): trailing content after the end of the document
```

`main()` 是边界：库内部靠抛异常报告失败，CLI 在这里转成退出码 + 一行消息。
（在补这个边界之前，畸形输入会让进程在 Debug 构建下卡在 CRT 的报告对话框上。）

## 6. 相关文档

- 素材包契约：`docs/assets-package.md`
- 应用侧的需求与设计：`minecraft-littletiles-reader-app` 的 `docs/design.md`
