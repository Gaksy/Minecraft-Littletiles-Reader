# 代码风格

本项目的 C++ 代码遵循 **Google C++ Style Guide**，并对"全局函数 / 类内函数"的命名做了明确区分。

## 1. 命名

| 类别 | 规范 | 示例 |
|---|---|---|
| 全局（自由）函数 | PascalCase | `ConvertAngleIdToInt()`、`MergeAndWriteToObj()`、`IsFileAccessible()` |
| 类的动作型成员函数 | PascalCase | `ReadChunk()`、`ApplyGrid()`、`Clear()`、`TileCount()` |
| 类的访问器 | snake_case，与成员同名（去掉结尾下划线） | `region_folder()`、`chunk_coordinate()`、`block_coordinate()`、`color()`、`grid()` |
| 类的修改器 | `set_*` snake_case | `set_pos()`、`set_color()`、`set_block_id()` |
| 类的谓词 | `is_*` / `has_*` snake_case | `is_empty()`、`has_color()`、`is_offset_off_boundary()` |
| STL 风格容器接口 | 保持小写 | `cbegin()`、`cend()` |
| 类型 | PascalCase | `TileEntity`、`LtSurfaceMesh` |
| 成员变量 | snake_case + 结尾下划线 | `block_id_`、`surface_mesh_` |
| 常量 | `k` + PascalCase | `kBaseSize` |
| 命名空间 | 小写下划线 | `galib::minecraft::littletiles` |

注意首字母缩略词的写法：按单词处理，例如 `ReadBlockTileNbt()`、`ConvertToCgalPoint()`
（不是 `ReadBlockTileNBT()` / `ConvertToCGALPoint()`）。

## 2. 格式

由仓库根目录的 `.clang-format` 定义（`BasedOnStyle: Google`、`Standard: c++17`、
关闭 `ReflowComments` 以免重排中文注释）。

```sh
# Xcode 自带 clang-format（本机 21.0.0）
CF=/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang-format
git ls-files '*.cpp' '*.h' | while IFS= read -r f; do "$CF" -i "$f"; done
```

提交前建议跑一次，保证 diff 里只有语义改动。

## 3. 文档中的代码引用

**不要写 `文件:行号`** —— 行号会随每次格式化/重构失效（本项目已经历过一次）。
请写成"文件 + 符号名"，例如：

```
见 Anvil.cpp 的 GetChunkConstReference()
```

符号名可以直接 grep，比行号耐久。

## 4. 与 Google 风格的刻意差异

以下三项**决定不改**，属于本项目接受的差异：

| 项目 | 现状 | Google | 原因 |
|---|---|---|---|
| 文件扩展名 | `.cpp` / `.h` | `.cc` / `.h` | 保持现状，不做纯改名 |
| 文件名 | `Anvil.cpp`、`LittleTilesEntity.cpp` | 小写下划线且与类名对应 | 同上，改动面大而收益小 |
| 头文件保护宏 | `GALIB_MINECRAFT_ANVIL_H` | 建议结尾多一个下划线 | 无实际影响 |

**已迁移**：命名空间宏（`GALIB_STD` / `GALIB_NBT` / `GALIB_CGAL` / `GALIB_BOOST` / `GALIB_CSTD` / 裸 `GALIB`）
已全部去掉，直接写 `std::` / `nbt::` / `CGAL::` / `boost::` / `::` / `galib::`；
C++17 之后不再需要的 `GALIB_NODISCARD` / `GALIB_NOEXCEPT` 也已改为 `[[nodiscard]]` / `noexcept`。

仍然保留的宏有三类：

1. `GALIB_DEBUG`（调试输出的编译期开关；目前恒为打开，应改为 CMake 选项，见 `known-issues.md`。
   运行期的进度输出开关是 `galib::SetProgressEnabled()`，不走宏）；
2. `GALIB_STATIC_ASSERT_*`（把类型名拼进报错信息的辅助宏）；
3. `GALIB_PRINTF_LIKE(a, b)`（`Log/GalibLog.h`，给 `ProgressPrintf` 挂 GCC/Clang 的
   printf 格式检查；MSVC 下展开为空）。
