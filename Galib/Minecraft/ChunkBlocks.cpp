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
 * Date Created: 09/12/2026
 */

#include "Minecraft/ChunkBlocks.h"

#include <string>

namespace galib::minecraft {

namespace {

const ChunkBlocks::State kAirState{};

// 索引顺序：x + z*16 + y*256（YZX，与 Minecraft 的区块存储一致）
std::size_t Index(const int kX, const int kY, const int kZ) {
  return static_cast<std::size_t>(kX) +
         static_cast<std::size_t>(kZ) * ChunkBlocks::kSizeX +
         static_cast<std::size_t>(kY) * ChunkBlocks::kSizeX *
             ChunkBlocks::kSizeZ;
}

std::uint8_t NibbleAt(const std::vector<std::uint8_t>& kData,
                      const std::size_t kIndex) {
  const std::size_t byte_index = kIndex / 2;
  if (byte_index >= kData.size()) {
    return 0;
  }
  return (kIndex % 2 == 0) ? (kData[byte_index] & 0x0F)
                           : ((kData[byte_index] >> 4) & 0x0F);
}

std::vector<std::uint8_t> TagBytes(const nbt::tag_byte_array& kTag) {
  const auto& values = kTag.get();
  return std::vector<std::uint8_t>(values.begin(), values.end());
}

// 注意：在这个 libnbt++ 版本 + macOS 上，value::as<nbt::tag_byte_array>() 会抛 std::bad_cast，
// 因此与 BlockTileEntities.cpp 里处理 box 的做法一致，改用 get() + static_cast。
const nbt::tag_byte_array& AsByteArray(const nbt::value& kValue) {
  return static_cast<const nbt::tag_byte_array&>(kValue.get());
}

}  // namespace

bool ChunkBlocks::ReadFromChunkLevel(const nbt::tag_compound& kChunkLevel) {
  if (!kChunkLevel.has_key("Sections")) {
    return false;
  }
  const auto& sections = kChunkLevel.at("Sections").as<nbt::tag_list>();
  bool found_section = false;
  for (auto it = sections.begin(); it != sections.end(); ++it) {
    const auto& section = it->as<nbt::tag_compound>();
    if (!section.has_key("Blocks")) {
      continue;
    }
    // 注意：1.12 里 Sections[].Y 是 TAG_Byte（不是 Int），按 tag_int 取会抛 std::bad_cast
    const int section_y =
        section.has_key("Y") ? section.at("Y").as<nbt::tag_byte>().get() : 0;
    const std::vector<std::uint8_t> blocks =
        TagBytes(AsByteArray(section.at("Blocks")));
    std::vector<std::uint8_t> add;
    if (section.has_key("Add")) {
      add = TagBytes(AsByteArray(section.at("Add")));
    }
    std::vector<std::uint8_t> data;
    if (section.has_key("Data")) {
      data = TagBytes(AsByteArray(section.at("Data")));
    }

    // 一个 section 是 16x16x16
    std::size_t local_index = 0;
    for (int y = 0; y < 16; ++y) {
      for (int z = 0; z < 16; ++z) {
        for (int x = 0; x < 16; ++x, ++local_index) {
          if (local_index >= blocks.size()) {
            break;
          }
          const int world_y = section_y * 16 + y;
          if (world_y < 0 || world_y >= kSizeY) {
            continue;
          }
          const std::uint16_t high = NibbleAt(add, local_index);
          const std::uint16_t block_id =
              static_cast<std::uint16_t>(blocks[local_index]) |
              static_cast<std::uint16_t>(high << 8);
          State& state = states_[Index(x, world_y, z)];
          state.block_id = block_id;
          state.meta = NibbleAt(data, local_index);
        }
      }
    }
    found_section = true;
  }
  return found_section;
}

void ChunkBlocks::MarkLittleTilesHosts(const nbt::tag_list& kTileEntities) {
  for (auto it = kTileEntities.begin(); it != kTileEntities.end(); ++it) {
    const auto& entity = it->as<nbt::tag_compound>();
    if (!entity.has_key("x") || !entity.has_key("y") || !entity.has_key("z")) {
      continue;
    }
    const int x = entity.at("x").as<nbt::tag_int>().get();
    const int y = entity.at("y").as<nbt::tag_int>().get();
    const int z = entity.at("z").as<nbt::tag_int>().get();
    if (x < 0 || x >= kSizeX || y < 0 || y >= kSizeY || z < 0 || z >= kSizeZ) {
      continue;
    }
    states_[Index(x, y, z)].little_tiles_host = true;
  }
}

const ChunkBlocks::State& ChunkBlocks::At(const int kX, const int kY,
                                          const int kZ) const {
  if (kX < 0 || kX >= kSizeX || kY < 0 || kY >= kSizeY || kZ < 0 ||
      kZ >= kSizeZ) {
    return kAirState;
  }
  return states_[Index(kX, kY, kZ)];
}

}  // namespace galib::minecraft
