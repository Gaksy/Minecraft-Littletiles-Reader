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
 * Date Created: 12/22/2024
 */

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <typeinfo>
#include <vector>

#include "Exception/LittleTilesException.h"
#include "GalibNamespaceDef.h"
#include "Minecraft/LittleTiles.h"
#include "Minecraft/MinecraftCoord.h"
#include "nbt_tags.h"

using galib::minecraft::littletiles::AngleOffset;
using galib::minecraft::littletiles::BlockTileEntities;
using galib::minecraft::littletiles::Flipped;
using galib::minecraft::littletiles::GridType;
using galib::minecraft::littletiles::TileEntity;
using galib::minecraft::littletiles::TileMaterial;

using galib::minecraft::BlockCoordinate;

using galib::exception::LittleTilesErrorCode;
using galib::exception::LittleTilesException;

using std::string;
using std::uint32_t;
using std::vector;

using nbt::tag_compound;
using nbt::tag_int;
using nbt::tag_int_array;
using nbt::tag_list;
using nbt::tag_string;

BlockTileEntities::BlockTileEntities()
    : block_coordinate_({0, 0, 0}), grid_(0) {}

BlockTileEntities::size_type BlockTileEntities::ReadBlockTileNbt(
    const tag_compound& kBlockTilesNBT, size_type* p_boxes_count) {
  // check block tiles root is not empty
  if (!kBlockTilesNBT.size()) {
    throw LittleTilesException(LittleTilesErrorCode::lt_tage_not_exist,
                               "The block tile data does not exist.",
                               "BlockTiles");
  }

  try {
    // By default
    GridType grid_type = 16;

    if (kBlockTilesNBT.has_key("grid")) {
      grid_type = kBlockTilesNBT.at("grid").as<tag_int>().get();
    }

    // Get ID
    string little_tiles_id = kBlockTilesNBT.at("id").as<tag_string>();

    // Get tiles list
    auto tiles = kBlockTilesNBT.at("content").at("tiles").as<tag_list>();

    // Process block tiles
    container box_tile_enities_map;
    size_type tile_count = 0;
    size_type boxes_count = 0;

    // Get block coord
    block_coordinate_ = {kBlockTilesNBT.at("x").as<tag_int>().get(),
                         kBlockTilesNBT.at("y").as<tag_int>().get(),
                         kBlockTilesNBT.at("z").as<tag_int>().get()};

#ifdef GALIB_DEBUG
    printf("BlockTileEntities::ReadBlockTileNbt read block: %d %d %d\n",
           block_coordinate_.x, block_coordinate_.y, block_coordinate_.z);
#endif

    for (auto it = tiles.begin(); it != tiles.cend(); ++it) {
      tag_compound* p_boxes = &it->as<tag_compound>();  // Get boxes

      // 材质键 = 方块 id + 可选颜色。
      // 同一种方块可以用不同染色，LittleTiles 会把它们存成不同的 tile 条目。
      TileMaterial material;
      material.block_id = p_boxes->at("block").as<tag_string>().get();
      if (p_boxes->has_key("color")) {
        material.color = p_boxes->at("color").as<tag_int>().get();
        material.has_color = true;
      }

      // 只有 (block, color) 完全相同才跳过。
      // 原先只用 block id 判重，会把同种方块的不同染色整条丢弃
      // （实测 chunk(-136,49) 因此丢了 45% 的 box）。
      if (box_tile_enities_map.find(material) != box_tile_enities_map.end()) {
        continue;
      }

      // Get data, If Get successful, then insert data
      if (BoxTileEnities box_tile_enities;
          ReadBoxesTilesNbt(*p_boxes, box_tile_enities, tile_count)) {
        if (material.has_color) {
          for (TileEntity& tile : box_tile_enities) {
            tile.set_color(material.color, true);
          }
        }
        box_tile_enities_map.insert(container_pair(material, box_tile_enities));
        ++boxes_count;
      }
    }

    // DONE!!
    grid_ = grid_type;
    box_tile_entities_map_.swap(box_tile_enities_map);
    little_tiles_id_.swap(little_tiles_id);

#ifdef GALIB_DEBUG
    printf(
        "BlockTileEntities::ReadBlockTileNbt Boxes count: %zu, Tile count: "
        "%zu\n",
        boxes_count, tile_count);
#endif
    if (p_boxes_count) {
      *p_boxes_count = boxes_count;
    }
    return tile_count;
  } catch (...) {
    throw LittleTilesException(LittleTilesErrorCode::lt_tage_not_exist,
                               "Some tag not exist.", "BlockTiles");
  }
}

const BlockCoordinate& BlockTileEntities::block_coordinate() const {
  return block_coordinate_;
}

const GridType& BlockTileEntities::grid() const { return grid_; }

const std::string& BlockTileEntities::little_tiles_id() const {
  return little_tiles_id_;
}

BlockTileEntities::const_iterator BlockTileEntities::cbegin() const {
  return box_tile_entities_map_.cbegin();
}

BlockTileEntities::const_iterator BlockTileEntities::cend() const {
  return box_tile_entities_map_.cend();
}

BlockTileEntities::size_type BlockTileEntities::TileCount() const {
  size_type tile_count = 0;
  for (const_iterator it = box_tile_entities_map_.cbegin();
       it != box_tile_entities_map_.cend(); ++it) {
    tile_count += it->second.size();
  }
  return tile_count;
}

namespace {

// 6 个面的几何约定（下标与 TileFaceID 一致）：
//   0 EAST(+x) 1 WEST(-x) 2 SOUTH(+z) 3 NORTH(-z) 4 UP(+y) 5 DOWN(-y)
// normal_axis 是面法线所在的轴；u/v 是面内两个轴，用来把面栅格化成 grid×grid。
struct FaceAxes {
  int normal_axis;
  int u_axis;
  int v_axis;
  bool positive;
};

constexpr FaceAxes kFaceAxes[6] = {
    {0, 1, 2, true},  {0, 1, 2, false}, {2, 0, 1, true},
    {2, 0, 1, false}, {1, 0, 2, true},  {1, 0, 2, false},
};

int ClampToGrid(const int kValue, const int kGrid) {
  if (kValue < 0) {
    return 0;
  }
  if (kValue > kGrid) {
    return kGrid;
  }
  return kValue;
}

}  // namespace

std::uint8_t BlockTileEntities::covered_face_mask() const {
  const int grid = grid_;
  if (grid <= 0) {
    return 0;
  }

  // 先把"没有角度偏移"的 tile 盒子整理成整数区间：
  // 有偏移的 tile 是斜面/异形，盒子不代表实际形状，保守当作没覆盖。
  struct Box {
    int low[3];
    int high[3];
  };
  std::vector<Box> boxes;
  boxes.reserve(TileCount());
  for (const auto& entry : box_tile_entities_map_) {
    for (const TileEntity& tile : entry.second) {
      if (tile.has_any_offset_enable()) {
        continue;
      }
      const double first[3] = {tile.pos_1().x, tile.pos_1().y, tile.pos_1().z};
      const double second[3] = {tile.pos_2().x, tile.pos_2().y, tile.pos_2().z};
      Box box{};
      for (int axis = 0; axis < 3; ++axis) {
        box.low[axis] = static_cast<int>(std::min(first[axis], second[axis]));
        box.high[axis] = static_cast<int>(std::max(first[axis], second[axis]));
      }
      boxes.push_back(box);
    }
  }
  if (boxes.empty()) {
    return 0;
  }

  // 面被栅格化为 grid×grid 个小格；每个贴在该面上的 tile 把自己的矩形格子标满。
  const auto grid_size = static_cast<std::size_t>(grid);
  std::vector<std::uint8_t> covered(grid_size * grid_size, 0);
  std::uint8_t mask = 0;

  for (int face = 0; face < 6; ++face) {
    const FaceAxes axes = kFaceAxes[face];
    std::fill(covered.begin(), covered.end(), 0);
    std::size_t marked_area = 0;

    for (const Box& box : boxes) {
      // 必须真的贴在面所在的平面上（例如方块底面就是 y = 0 那个平面）
      const bool touches = axes.positive ? box.high[axes.normal_axis] >= grid
                                         : box.low[axes.normal_axis] <= 0;
      if (!touches) {
        continue;
      }

      const int u_begin = ClampToGrid(box.low[axes.u_axis], grid);
      const int u_end = ClampToGrid(box.high[axes.u_axis], grid);
      const int v_begin = ClampToGrid(box.low[axes.v_axis], grid);
      const int v_end = ClampToGrid(box.high[axes.v_axis], grid);
      if (u_begin >= u_end || v_begin >= v_end) {
        continue;
      }
      marked_area += static_cast<std::size_t>(u_end - u_begin) *
                     static_cast<std::size_t>(v_end - v_begin);
      for (int v = v_begin; v < v_end; ++v) {
        std::uint8_t* const row =
            covered.data() + static_cast<std::size_t>(v) * grid_size;
        std::fill(row + u_begin, row + u_end, static_cast<std::uint8_t>(1));
      }
    }

    // 面积不够一定铺不满；够的话再确认没有空洞（tile 之间可能重叠）
    if (marked_area < grid_size * grid_size) {
      continue;
    }
    bool is_full = true;
    for (const std::uint8_t cell : covered) {
      if (cell == 0) {
        is_full = false;
        break;
      }
    }
    if (is_full) {
      mask |= static_cast<std::uint8_t>(1u << face);
    }
  }
  return mask;
}

bool BlockTileEntities::ReadBoxesTilesNbt(const tag_compound& kBoxesTilesNbt,
                                          BoxTileEnities& desc_box_tile_enities,
                                          size_type& tile_count) {
  if (!kBoxesTilesNbt.size()) {
    return false;
  }

  try {
    tag_list boxes_pos;

    // Get struct
    if (kBoxesTilesNbt.has_key("boxes")) {
      boxes_pos = kBoxesTilesNbt.at("boxes").as<tag_list>();
    } else if (kBoxesTilesNbt.has_key("box")) {
#ifdef _WIN32
      boxes_pos.push_back(nbt::value_initializer(
          kBoxesTilesNbt.at("box").as<tag_int_array>().clone()));
#elif __APPLE__
      boxes_pos.push_back(
          nbt::value_initializer(static_cast<const nbt::tag_array<int32_t>&>(
                                     kBoxesTilesNbt.at("box").get())
                                     .clone()));
#endif
    } else {
      return false;
    }

    // Get tiles data
    BoxTileEnities box_tile_enity_array;

    for (tag_list::const_iterator it = boxes_pos.cbegin();
         it != boxes_pos.cend(); ++it) {
      try {
        TileEntity temp;
        auto& inner_tag = it->get();
        // 强制转换
#ifdef __APPLE__
        const auto& int_array =
            static_cast<const nbt::tag_array<int32_t>&>(inner_tag);
#elif _WIN32
        const auto& int_array = it->as<tag_int_array>();
#endif

        if (int_array.size() < 6) {
          continue;
        }  // pos must have 6 num (two vertices)
        if (int_array.size() > 6) {  // if > 6 , then have offset and flipped
          AngleOffset angle_offset_data[8];
          Flipped flipped_data;
          if (!SetAngleOffsetStateData(int_array, angle_offset_data,
                                       &flipped_data)) {
            continue;
          }
          temp.set_flipped_data(flipped_data);
          temp.set_offset_data(angle_offset_data);
        }

        LittleTilesCoord pos_1, pos_2;
        pos_1.x = int_array[0];
        pos_1.y = int_array[1];
        pos_1.z = int_array[2];
        pos_2.x = int_array[3];
        pos_2.y = int_array[4];
        pos_2.z = int_array[5];
        temp.set_pos(pos_1, pos_2);

        box_tile_enity_array.push_back(temp);
        ++tile_count;
      } catch (const std::exception& e) {
        std::cerr << "Error parsing box tile entity: " << e.what() << std::endl;
      }
    }

    // Cheak data
    if (box_tile_enity_array.empty()) {
      return true;
    }
    // Save data
    desc_box_tile_enities.swap(box_tile_enity_array);
  }
#ifndef GALIB_DEBUG
  catch (...) {
    return false;
  }
#else
  catch (const std::exception& e) {
    printf("ChunkTileEntities::ReadBoxesTilesNbt error: %s\n", e.what());
    return false;
  }
#endif
  return true;
}

bool BlockTileEntities::SetAngleOffsetStateData(const tag_int_array& offset_nbt,
                                                AngleOffset* p_offset_data,
                                                Flipped* p_flipped_data) {
  // Check nbt size, if < 7, the angle change data is null
  if (offset_nbt.size() < 7) {
    return false;
  }

  // Get angle change data state iterator and create change data buffer
  const auto it = offset_nbt.cbegin() + 6;
  const uint32_t state_binary = *it;
  vector<OffsetType> angle_offset_array;

  // Get Angle offset data array
  if (it + 1 != offset_nbt.cend()) {
    for (auto offset_it = it + 1; offset_it != offset_nbt.cend(); ++offset_it) {
      angle_offset_array.push_back(static_cast<OffsetType>(
          (*offset_it & 0xFFFF0000) >> 16));  // Get 16bit
      angle_offset_array.push_back(
          static_cast<OffsetType>(*offset_it & 0x0000FFFF));  // Get 16bit
    }
  }

  // Get Angle offset state and set offset value
  auto offset_it = angle_offset_array.cbegin();
  const auto offset_it_end = angle_offset_array.cend();
  for (size_t angle_id = 0; angle_id < 8; ++angle_id) {
    p_offset_data->x_enable = state_binary & (0x1 << (angle_id * 3));
    p_offset_data->y_enable = state_binary & (0x2 << (angle_id * 3));
    p_offset_data->z_enable = state_binary & (0x4 << (angle_id * 3));

    if (p_offset_data->has_any_enable()) {
      if (p_offset_data->x_enable && offset_it != offset_it_end) {
        p_offset_data->x_offset = *(offset_it++);
      }
      if (p_offset_data->y_enable && offset_it != offset_it_end) {
        p_offset_data->y_offset = *(offset_it++);
      }
      if (p_offset_data->z_enable && offset_it != offset_it_end) {
        p_offset_data->z_offset = *(offset_it++);
      }
    }
    p_offset_data++;
  }

  // Get Flipped
  p_flipped_data->down = state_binary & (0x1 << (8 * 3));
  p_flipped_data->up = state_binary & (0x2 << (8 * 3));
  p_flipped_data->north = state_binary & (0x4 << (8 * 3));
  p_flipped_data->south = state_binary & (0x1 << (9 * 3));
  p_flipped_data->west = state_binary & (0x2 << (9 * 3));
  p_flipped_data->east = state_binary & (0x4 << (9 * 3));

  return true;
}
