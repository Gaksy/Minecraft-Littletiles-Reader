# SNBT → Java Block/Item BB 工程（第一版）

2026-11-24：已实现并构建，自动测试通过；等待用户提供 SNBT，在实际 Blockbench 中打开、编辑和保存确认。本功能尚未正式发行，未替换安装目录内的程序。

## 范围

- 从 LittleTiles SNBT `.txt/.struct` 导出 `.bbmodel`；文件或粘贴输入均可。
- 复用 Reader 的方言检测：旧版 `tiles` 列表、新版 `t` 映射。这里识别的是数据格式家族，不能从它推断精确 Minecraft／LittleTiles 版本。
- 输出为 `java_block` 工作区、BB 工程格式 `5.0`，需要支持该工程版本的 Blockbench。
- 只输出标准轴对齐长方体，保留独立可编辑元素、六面 UV、基础贴图、tile 颜色与子结构分组。
- 颜色／固定 tint 烘焙进 PNG，PNG 内嵌到工程；无需配套 MTL 或原始材质路径。
- 一方块对应 16 模型单位。默认把整体占用格范围中心放到 `[8,8,8]`，只平移、不缩放。
- 跨贴图重复单元的长方体会按单元边界拆成多个 cube，以保留材质重复；源盒子数与最终 cube 数可能不同。
- 不迁移门动画、开关、交互等行为。

## 不支持与失败行为

- 带角点偏移标记的盒子（包括斜面）直接拒绝整次 BB 导出，报告材质名称；不会静默跳过，也不会把斜面改回直角盒子。
- 整体和分块入口均要求整份蓝图最多跨越 3×3×3 个游戏方块格子；超出拒绝，不自动缩小。整体工程平移到 Java Block 的 `[-16,32]` 范围。
- 缺失材质映射、缺失／无法解码的 PNG、非正尺寸、非方形贴图均拒绝。输出方形基础贴图，动画贴图按元数据取第一帧；不保留动画播放、CTM、多层 overlay 或光影材质。
- `normalize_scale=true` 会被 CLI 拒绝；桌面端 BB job 会明确设为 false。
- 当前仅支持 SNBT 输入，不支持直接把整片 region 导成 BB。

偏移、尺寸与贴图校验在写工程前完成。校验失败不会生成部分工程或覆盖已有 BB 文件。写盘失败仍会返回失败，不能把不完整文件当成成功产物。

## 使用

桌面应用新增“导出 Blockbench（Java 方块工程）”，快速导出和项目模式均可选文件／粘贴文本。项目记录支持 BB 产物、重导出与自足 ZIP。应用必须搭配本次新增能力的核心；旧核心若忽略输出格式返回 OBJ，桌面端会判定失败并提示更新核心。

CLI 通过 job 选择输出格式，OBJ 的默认行为不变：

```json
{
  "schema": 1,
  "mode": "snbt",
  "input": {"snbt": {"path": "D:/models/example.txt"}},
  "assets": {"package": "D:/assets/vanilla"},
  "output": {"dir": "D:/models/out", "name": "example", "format": "bbmodel"},
  "options": {"center": true, "normalize_scale": false}
}
```

```powershell
.\LittleTilesReader.exe --job bbmodel-job.json --progress json
```

成功事件返回 `bbmodel` 路径、`source_dialect`、`source_boxes`、`cubes`、`faces`、`vertices`、`materials`、`textures_written` 与耗时；失败事件返回 error，进程非零退出，无 done。

`textures_written` 在此处指嵌入工程的 PNG 数，`vertices` 是各 cube 的 8 顶点计数，不是合并后的唯一顶点数。

## 本轮素材选择

用户指定使用现有安装中的原版 1.12.2 素材，不叠加资源包或模组。已从登记的 vanilla source 生成独立素材包，未修改安装目录或启用列表。后续用户样本按这套素材转换；若样本引用原版之外的方块，明确报告缺失，不擅自替换材质。

## 验证

- CTest：结构读写、存档 tile 解析、BB exporter 三项测试通过。
- 新增桌面／真实 CLI 集成检查：两代 SNBT、工作区、像素级染色、内嵌 PNG、自足 ZIP、项目记录及重导出。
- UV 检查覆盖六面、整格边界、负坐标与跨格重复；结构检查覆盖不同 grid 与子结构原点。
- 已回归 SNBT 粘贴、导出选项、打包、输出提示、项目管理和多语言。
- 尚待用户样本和实际 BB 界面验收；尚未在 MCreator 或游戏中验收，不宣称对应版本已正式支持。

实现入口：`Galib/Minecraft/BbmodelExporter.cpp` 的 `BuildJavaBbmodel`，由 `main.cpp` 的 SNBT 分支调用。PNG 内存编码复用 `PngImage::Encode`。

## 后续扩展：批量按方块拆分

已新增独立的 [整份蓝图批量拆分模式](bbmodel-blocks.md)，可自动生成多份 BB 工程，省去在游戏中逐格保存蓝图。
两种入口均受整份蓝图 3×3×3 范围限制；分块工程使用 0..16 的模型坐标，并以游戏轴向的 0..2 格坐标标记，仍拒绝偏移斜面。
