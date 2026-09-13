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

// A LittleTiles "structure" (a whole building copied out in-game), coming from
// SNBT text.
//
// It uses the same box encoding as the tile entities in save files (`box`/`boxes`
// plus the angle-offset bit field); the only difference is the coordinates: a
// structure uses **structure-space** grid coordinates (spanning many blocks),
// whereas a tile entity's coordinates are grid coordinates inside a block. So
// TileEntity is reused here, and meshing only has to scale by grid into block
// units and subtract the structure origin.
//
// A structure may embed child structures (children: doors / lamps / particle
// emitters / ...); v1 only takes their geometry and ignores their behavioural
// parameters (animations, open/closed state, ...).
class LtStructure {
 public:
  // A group of boxes under one material (block name + optional tint)
  struct Group {
    std::string block_id;
    std::int32_t color{0};
    bool has_color{false};
    std::vector<TileEntity> boxes;
  };

  LtStructure() = default;

  static LtStructure FromSnbt(const snbt::Value& kRoot);
  static LtStructure FromSnbtFile(const std::string& kPath);

  // Tile resolution (1 block = grid units inside a structure; measured 32 for
  // this house)
  [[nodiscard]] int grid() const { return grid_; }
  // Structure origin (in grid units); meshing subtracts it so the model lands
  // near the origin
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
