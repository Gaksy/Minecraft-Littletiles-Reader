/*
 * Copyright (c) 2024 Gaksy (Fuhongren)
 *
 * This work is licensed under the GNU Lesser General Public License v3.0.
 * You may obtain a copy of the license at https://www.gnu.org/licenses/lgpl-3.0.html.
 *
 * This source code form is subject to the terms of the LGPL v3.0 license.
 * If a copy of the LGPL was not distributed with this file, you can obtain one
 * at the above license URL.
 */

/*
 * Author: Gaksy
 * Date Created: 09/13/2026
 */

#ifndef GALIB_MINECRAFT_LTSTRUCTURE_H
#define GALIB_MINECRAFT_LTSTRUCTURE_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "GalibNamespaceDef.h"
#include "Minecraft/LittleTiles.h"
#include "Minecraft/MinecraftCoord.h"
#include "Minecraft/SnbtParser.h"

namespace galib::minecraft::littletiles {

// LittleTiles 的"结构"（游戏里复制出来的一整块建筑），来自 SNBT 文本。
//
// 与存档里的 tile entity 是同一套盒子编码（`box`/`boxes` + 角度偏移位域），
// 区别只在坐标：结构是**结构空间**的 grid 坐标（跨很多方块），而 tile entity 的
// 坐标是方块内的 grid 坐标。因此这里复用了 TileEntity，只是网格化时要按 grid
// 缩放到方块单位、并减去结构原点。
//
// 结构里可能嵌子结构（children，门/灯/粒子发射器……），v1 只取几何、忽略其
// 行为参数（动画、开关状态等）。
class LtStructure {
 public:
  // 一种材质（方块名 + 可选染色）下的一组盒子
  struct Group {
    std::string block_id;
    std::int32_t color{0};
    bool has_color{false};
    std::vector<TileEntity> boxes;
  };

  LtStructure() = default;

  static LtStructure FromSnbt(const snbt::Value& kRoot);
  static LtStructure FromSnbtFile(const std::string& kPath);

  // tile 分辨率（结构里 1 方块 = grid 个单位，实测这栋房子是 32）
  [[nodiscard]] int grid() const { return grid_; }
  // 结构原点（grid 单位），网格化时减掉它，模型就落在原点附近
  [[nodiscard]] const BlockCoordinate& min() const { return min_; }
  [[nodiscard]] const BlockCoordinate& size() const { return size_; }
  [[nodiscard]] const std::string& name() const { return name_; }
  [[nodiscard]] const std::vector<Group>& groups() const { return groups_; }
  [[nodiscard]] std::size_t BoxCount() const;
  [[nodiscard]] int child_group_count() const { return child_group_count_; }

 private:
  void ReadGroups(const snbt::Value& kGroup);

  int grid_{16};
  BlockCoordinate min_{};
  BlockCoordinate size_{};
  std::string name_;
  std::vector<Group> groups_;
  int child_group_count_{0};
};

}  // namespace galib::minecraft::littletiles

#endif  // GALIB_MINECRAFT_LTSTRUCTURE_H
