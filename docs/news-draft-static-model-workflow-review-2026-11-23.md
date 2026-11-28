# 开发进展：LittleTiles 静态模型导出工作流需求审查

状态：待发布。日期：2026-11-23（Asia/Shanghai）。本地草稿，不是正式发行公告。

## 本次完成

我们对“在 Minecraft 内用 LittleTiles 建模，再将作品用于 MCreator 方块模组”的工作流完成了需求与现有源码审查。目标范围为静态几何和材质，不迁移 LittleTiles 的门动画、开关、交互等结构行为。

现有 Reader 已有 LittleTiles 结构读取、几何重建、基础贴图处理与颜色烘焙能力。目前模型导出链路为 OBJ、MTL 与 PNG；这不代表已经支持 Blockbench 工程或 Java 方块 JSON 导出。

## 调查结论

这一工作流具备扩展基础。拟议方向是导出可直接用于 MCreator 的静态模型资源，并让 Blockbench 成为可选的检查与编辑步骤。

仍需解决目标格式约束：规则盒子适合 Java 方块模型，任意逐角变形并不总能无损表达。超出目标格式能力的几何应明确报告，而不是静默删除或改变形状。

跨越多个方块的静态外观，也不自动获得多方块占位、碰撞或联动放置行为。此类行为需要在目标模组工具中另行配置。

游戏内建模可以直接预览作品，但导出基础材质不等于导出整套光影效果。复杂资源包、连接纹理、多层材质与透明效果需要额外检查。

## 尚未完成

- 尚未实现本需求对应的 Java 方块 JSON 或 Blockbench 工程导出器。
- 尚未完成本工作流在 Blockbench、MCreator 和目标游戏环境中的端到端验收。
- 本次没有发布包含这些新功能的版本。

## 下一步建议

先固定一组目标游戏与工具版本，用静态轴对齐盒子、基础不透明材质和染色完成最小闭环。通过导入、构建与游戏验收后，再评估自动拆分、透明材质和不规则形状等扩展。

---

以下为内部交付记录，发布正文时不包含本节：

- updateId: reader-static-model-workflow-review-2026-11-23
- type: progress
- changeStatus: in-progress
- sourceEvidence: docs/review-littletiles-to-mcreator.md（本地需求与代码审查，未实现）
- articleStatus: blocked
- articleId: 空
- revisionId: 空
- articleUrl: 空
- verifiedAt: 未执行公开回读
- blockReason: 本轮未取得可验证的生产发布会话；未创建线上草稿或文章。
