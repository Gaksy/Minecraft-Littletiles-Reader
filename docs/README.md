# 工程文档

本目录是**随仓库分发**的工程文档，面向开发者与 AI 助手，内容以代码实测为准
（每条结论尽量附 `文件:行` 或可复现命令）。

| 文档 | 内容 |
|---|---|
| [`architecture.md`](architecture.md) | 模块划分、真实调用链、类型与 ownership、线程安全与异常体系现状、构建方式 |
| [`assets-package.md`](assets-package.md) | 素材组合契约：生成端/宿主/库的职责边界、素材目录布局、manifest、库侧接口与材质管理实测 |
| [`job.md`](job.md) | 非交互驱动契约：`--job` / `--progress json`、job 文件字段、进度事件、退出码 |
| [`nbt-format.md`](nbt-format.md) | Anvil `.mca` / chunk NBT / LittleTiles tile 的数据格式、角度偏移位域、坐标系统 |
| [`known-issues.md`](known-issues.md) | 已核实缺陷、已修复项、输出非确定性、测试数据与基线、README 与代码不一致之处 |
| [`reference-lt3d-importer.md`](reference-lt3d-importer.md) | 第三方模组 LT 3D Importer & Exporter 的实现研究，尤其 UV/纹理策略 |
| [`texture-mapping.md`](texture-mapping.md) | 贴图/UV 方案：六面 UV 约定、资源包解析、颜色处理与实现计划 |
| [`style.md`](style.md) | 代码风格（Google C++）：命名规范、格式化命令、文档引用约定 |

> 本项目另有一个**本地**目录 `.ai/`（被 `.gitignore` 忽略），其中的内容是本目录的超集，
> 包含本机环境细节、临时探针与待办路线。两者如有冲突，以代码与 `docs/` 为准，
> 并请把结论同步回 `.ai/`。

> **本仓库只含库（C++ 核心 + CLI）。** 生成端脚本（原 `tools/`）、MATLAB 辅助（原 `python/`）
> 与测试数据（原 `data/`：regions / snbt / matlab / assets / reference）**不随库分发**，
> 已移到两个兄弟仓库：生成端 **`minecraft-littletiles-reader-tools`**（`ltgen` + `tools/`
> + `python/`）与测试数据 **`minecraft-littletiles-reader-data`**。
> 本文档目录里提到 `tools/*.py`、`python/*.py`、
> `data/...` 的地方，指的都是那些外部资产——保留原文是为了记录"这份素材/这张表是怎么来的"，
> 不代表本仓库里还有这些文件。库侧契约见 [`assets-package.md`](assets-package.md)。

## 相关文档

- 面向使用者的说明：[`../README.md`](../README.md)
- 设计文档（PDF/DOCX）：[`./技术文档/`](./技术文档/)
