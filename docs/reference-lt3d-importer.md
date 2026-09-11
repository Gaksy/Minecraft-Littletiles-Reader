# 参考实现研究：LT 3D Importer & Exporter

研究对象：[Timardo/LT-3D-Importer](https://github.com/Timardo/LT-3D-Importer)（MIT，2020），
MC 1.12.2 / Forge + LittleTiles + CreativeCore 的第三方模组，功能是 LT 结构 ⇄ OBJ 互转。

> 与本项目同代（1.12.2 / LT 1.5.x），因此它的数据解释可直接与我们的解析结果对照。
> 本地副本（含完整源码）在 `research/`，该目录不随仓库分发；本文件只记录分析结论与行号引用。

## 1. 入口文件 `LT3DImporter.java` 没有解析逻辑

它只是模组入口（102 行）：注册两个 `LittleStructurePremade` 类型（`ModelImporter` / `ModelExporter`）、
两个 GUI handler 与两个网络包。真正的实现：

| 职责 | 文件 |
|---|---|
| 导出 LT → OBJ/MTL/PNG | `exporter/Exporter.java`（364 行） |
| 导入 OBJ → LT 结构 NBT | `importer/Importer.java`（484 行）、`Triangle.java`、`ConvertedModel.java` |
| OBJ 读写（javagl Obj 的改造版，支持材质组） | `obj3d/LightObj.java`（486 行）等 |

## 2. 导出流程：纹理与 UV 从哪来

**它不解析存档 NBT，而是运行在游戏客户端内，直接向渲染器要几何与 UV。**

```
LittlePreview.getCubes(blueprintStack, false)      // LT 模组 API 给出 RenderBox 列表
   → 每个 cube：BlockState + LT 染色 color + offset
   → Minecraft.getBlockRendererDispatcher().getModelForState(...)     // 方块烘焙模型
   → 6 个朝向 CreativeBakedModel.getBakedQuad(...)                    // BakedQuad
   → 顶点数据中已含 position / UV / color
```

随后（行号为 `exporter/Exporter.java`）：

| 步骤 | 做法 | 行 |
|---|---|---|
| UV 提取 | `sprite.getUnInterpolatedU(v)/16 % 1`，写出时 `1-v` 翻转 | 167-189 |
| 顶点焊接 | x 的 float 位 + yz 的 long 位打包成 key 去重 | 176-184 |
| UV 去重 | (u,v) 位打包成 long 做字典 | 186-189 |
| 退化面处理 | 用 `uniqueVertices` 检出重复顶点，四边形收缩为三角形或整面丢弃 | 150-159 |
| 材质名 | 精灵图 `ns:path` → `ns_path`，再拼上 tile 颜色整数 | 230、261 |
| 纹理烘焙 | **精灵图像素 × tile 颜色** 逐像素相乘 → PNG 写到 `textures/<ns>/<name>.png` | 235-277 |
| MTL | `map_Kd` 指向该 PNG，`Ka/Kd/D` 固定 | 279-288 |
| 重复面剔除 | 用**排序后**的顶点索引对做 key 统计重复，同材质重叠面只留一份（树叶除外） | 291-345 |
| 分组 | `setActiveMaterialGroupName` + `setActiveGroupNames` → `usemtl` + group | 328-331 |

要点与警示：

- **法线故意不导出**，注释说明 vanilla 方块的烘焙四边形没有可靠法线。
- 导出运行在裸 `Thread` 中，却在其中调用 `Minecraft.getMinecraft()` 等客户端 API
  （`Exporter.java:113-115`），这在 Forge 中是有风险的写法，**不要照抄该线程模型**。
- 导出源是**蓝图物品**（`ItemLittleRecipeAdvanced`）而非 chunk，因此其数据获取路径无法直接用于本项目的无头读取器。

## 3. 导入流程

把三角形沿三轴**切片**成小方块（`Triangle.calcBlocks`），每个采样点用 `ITexture.colorTile(uv)`
**按 UV 取纹理像素得到一个 int 颜色**，再生成 `LittleTileColored(block, meta, color)`
（`importer/ConvertedModel.java:50-58`）。可配置 grid、precision（默认 0.05）、最大尺寸与基础方块。

**这反过来证明了：LittleTiles 的原生材质表达是"每个 tile 一个颜色"，而非纹理坐标映射。**

## 4. 对本项目的意义

### 4.1 不可直接借鉴

**真实纹理与 UV**：纹理只存在于资源包（blockstates/model JSON + PNG + tint/colormap 逻辑），
存档 NBT 中没有。该模组能取得，是因为它运行在 Minecraft 客户端内部。
无头 C++ 程序要复刻，等于自行实现一套资源包解析与模型烘焙。

### 4.2 可以借鉴（不依赖资源包）

1. **per-tile 颜色**：NBT 中现成（我们的数据里 44% 的 tile 带 `color`），可直接用于上色。
2. **材质主键** = `block id (+meta)` + `color`。项目已把 `block_id_` 存于每个 `LtSurfaceMesh`，
   只是导出时丢弃；OBJ 用 `usemtl`/group，glTF 用 material。
3. **顶点焊接 + 重复面剔除**：纯几何操作，直接可用，对体积与透明渲染均有收益。
4. **输出规范化**：它用"排序后的索引对"做 key，顺带解决了本项目发现的输出非确定性问题
   （见 `known-issues.md` 第 3 节）。
5. **纹理的替代路线**：C++ 只输出 `block id + color + 面朝向`，浏览器端用离线预生成的
   block→texture 映射或图集贴图（如 Three.js 按材质名查纹理），使 C++ 保持无头、无资源包依赖。

### 4.3 只能作为语义参照

`LittlePreview.getCubes`、`LittleTileColored`、`LittleGridContext`、`LittleBox`、`BasicCombiner`
均为 mod 内部类，不能作为本项目依赖，只可用于对照语义。
