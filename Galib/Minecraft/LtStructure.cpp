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

#include "Minecraft/LtStructure.h"

#include <algorithm>
#include <stdexcept>

namespace galib::minecraft::littletiles {

namespace {

// 一个 box 数组 → TileEntity（复用存档路径的同一套语义）
void ReadBox(const std::vector<std::int64_t>& kNumbers,
             TileEntity* p_desc_tile) {
  if (kNumbers.size() < 6) {
    throw std::runtime_error("盒子数组长度不足 6");
  }
  LittleTilesCoord pos_1;
  LittleTilesCoord pos_2;
  pos_1.x = static_cast<double>(kNumbers[0]);
  pos_1.y = static_cast<double>(kNumbers[1]);
  pos_1.z = static_cast<double>(kNumbers[2]);
  pos_2.x = static_cast<double>(kNumbers[3]);
  pos_2.y = static_cast<double>(kNumbers[4]);
  pos_2.z = static_cast<double>(kNumbers[5]);
  p_desc_tile->set_pos(pos_1, pos_2);

  if (kNumbers.size() <= 6) {
    return;
  }
  std::vector<std::int32_t> compact;
  compact.reserve(kNumbers.size());
  for (const std::int64_t number : kNumbers) {
    compact.push_back(static_cast<std::int32_t>(number));
  }
  AngleOffset offsets[8];
  Flipped flipped;
  if (DecodeBoxAngleData(compact, offsets, &flipped)) {
    p_desc_tile->set_offset_data(offsets);
    p_desc_tile->set_flipped_data(flipped);
  }
}

}  // namespace

LtStructure LtStructure::FromSnbt(const snbt::Value& kRoot) {
  if (!kRoot.is_compound()) {
    throw std::runtime_error("结构 SNBT 的顶层不是复合标签");
  }
  LtStructure structure;
  if (kRoot.has_member("grid")) {
    structure.grid_ = static_cast<int>(kRoot.member("grid").as_int());
  }
  if (structure.grid_ <= 0) {
    structure.grid_ = 16;
  }
  if (kRoot.has_member("min")) {
    const std::vector<std::int64_t> min = kRoot.member("min").AsIntArray();
    if (min.size() >= 3) {
      structure.min_.x = static_cast<std::int32_t>(min[0]);
      structure.min_.y = static_cast<std::int32_t>(min[1]);
      structure.min_.z = static_cast<std::int32_t>(min[2]);
    }
  }
  if (kRoot.has_member("size")) {
    const std::vector<std::int64_t> size = kRoot.member("size").AsIntArray();
    if (size.size() >= 3) {
      structure.size_.x = static_cast<std::int32_t>(size[0]);
      structure.size_.y = static_cast<std::int32_t>(size[1]);
      structure.size_.z = static_cast<std::int32_t>(size[2]);
    }
  }
  if (kRoot.has_member("structure")) {
    const snbt::Value& info = kRoot.member("structure");
    if (info.is_compound() && info.has_member("name") &&
        info.member("name").is_string()) {
      structure.name_ = info.member("name").as_string();
    }
  }
  structure.ReadGroups(kRoot);
  return structure;
}

LtStructure LtStructure::FromSnbtFile(const std::string& kPath) {
  return FromSnbt(snbt::ParseFile(kPath));
}

void LtStructure::ReadGroups(const snbt::Value& kGroup) {
  // 一个分组 = 一种材质（block + 可选 color）+ 若干盒子
  if (kGroup.has_member("tile")) {
    const snbt::Value& tile = kGroup.member("tile");
    if (tile.is_compound() && tile.has_member("block") &&
        tile.member("block").is_string()) {
      LtStructure::Group group;
      group.block_id = tile.member("block").as_string();
      if (tile.has_member("color")) {
        group.color = static_cast<std::int32_t>(tile.member("color").as_int());
        group.has_color = true;
      }
      for (const char* const key : {"boxes", "box", "bBox"}) {
        if (!kGroup.has_member(key)) {
          continue;
        }
        const snbt::Value& holder = kGroup.member(key);
        if (holder.is_list() && holder.array_type() == '\0' &&
            !holder.items().empty() && holder.item(0).is_list()) {
          for (const snbt::Value& entry : holder.items()) {
            group.boxes.emplace_back();
            ReadBox(entry.AsIntArray(), &group.boxes.back());
          }
        } else {
          group.boxes.emplace_back();
          ReadBox(holder.AsIntArray(), &group.boxes.back());
        }
      }
      if (!group.boxes.empty()) {
        groups_.push_back(std::move(group));
      }
    }
  }
  // children：结构里嵌的子结构（门、灯……）。v1 只取几何，忽略其行为参数。
  if (kGroup.has_member("children")) {
    for (const snbt::Value& child : kGroup.member("children").items()) {
      ++child_group_count_;
      ReadGroups(child);
    }
  }
  if (kGroup.has_member("tiles")) {
    for (const snbt::Value& tile : kGroup.member("tiles").items()) {
      ReadGroups(tile);
    }
  }
}

std::size_t LtStructure::BoxCount() const {
  std::size_t count = 0;
  for (const LtStructure::Group& group : groups_) {
    count += group.boxes.size();
  }
  return count;
}

}  // namespace galib::minecraft::littletiles
