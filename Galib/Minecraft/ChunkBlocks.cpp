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

#include <algorithm>
#include <map>
#include <string>

#include "Log/GalibLog.h"
#include "Log/GalibText.h"
#include "Minecraft/LittleTiles.h"

namespace galib::minecraft {

namespace {

const ChunkBlocks::State kAirState{};
const std::string kEmptyName;
const std::string kAirName = "minecraft:air";

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

// Note: with this libnbt++ version on macOS, value::as<nbt::tag_byte_array>()
// throws std::bad_cast, so - consistent with how box is handled in
// BlockTileEntities.cpp - get() + static_cast is used instead.
const nbt::tag_byte_array& AsByteArray(const nbt::value& kValue) {
  return static_cast<const nbt::tag_byte_array&>(kValue.get());
}

// Bits per packed palette index in a 1.16+ section: at least 4, otherwise just
// enough to hold the palette size (the packing may leave unused trailing bits in
// the last long, which is why values beyond the palette are ignored below).
std::size_t PaletteBits(const std::size_t kPaletteSize) {
  std::size_t bits = 4;
  while ((1ull << bits) < kPaletteSize && bits < 32) {
    ++bits;
  }
  return bits;
}

// Section Y as a plain int. 1.12.2 stores it as a TAG_Byte and 1.18+ as a
// TAG_Int, so the type is checked instead of assumed (a tag_compound::getInt in
// the game accepts any numeric type the same way).
int SectionY(const nbt::tag_compound& kSection) {
  if (!kSection.has_key("Y")) {
    return 0;
  }
  const nbt::value& value = kSection.at("Y");
  switch (value.get_type()) {
    case nbt::tag_type::Byte:
      return value.as<nbt::tag_byte>().get();
    case nbt::tag_type::Short:
      return value.as<nbt::tag_short>().get();
    case nbt::tag_type::Int:
      return value.as<nbt::tag_int>().get();
    default:
      return 0;
  }
}

}  // namespace

bool ChunkBlocks::ReadFromChunkLevel(const nbt::tag_compound& kChunkLevel) {
  // 1.12.2 keeps the sections in `Sections` (id + nibble metadata), 1.18+ in
  // `sections` (palette + packed longs); the key alone tells the two apart.
  if (kChunkLevel.has_key("sections")) {
    return ReadNames118(kChunkLevel);
  }
  if (kChunkLevel.has_key("Sections")) {
    return ReadIds12(kChunkLevel);
  }
  return false;
}

bool ChunkBlocks::ReadIds12(const nbt::tag_compound& kChunkLevel) {
  layout_ = Layout::kIds12;
  min_y_ = 0;
  size_y_ = 256;
  names_.clear();
  states_.assign(static_cast<std::size_t>(kSizeX) * size_y_ * kSizeZ, State{});

  const auto& sections = kChunkLevel.at("Sections").as<nbt::tag_list>();
  bool found_section = false;
  for (auto it = sections.begin(); it != sections.end(); ++it) {
    const auto& section = it->as<nbt::tag_compound>();
    if (!section.has_key("Blocks")) {
      continue;
    }
    // Note: in 1.12 Sections[].Y is a TAG_Byte (not an Int); reading it as
    // tag_int throws std::bad_cast, so the type is checked (see SectionY).
    const int section_y = SectionY(section);
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

    // One section is 16x16x16
    std::size_t local_index = 0;
    for (int y = 0; y < 16; ++y) {
      for (int z = 0; z < 16; ++z) {
        for (int x = 0; x < 16; ++x, ++local_index) {
          if (local_index >= blocks.size()) {
            break;
          }
          const int world_y = section_y * 16 + y;
          if (world_y < min_y_ || world_y >= min_y_ + size_y_) {
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

bool ChunkBlocks::ReadNames118(const nbt::tag_compound& kChunkLevel) {
  layout_ = Layout::kNames118;
  // 1.18+ worlds run from y = -64 up to y = 319, i.e. 384 rows (a section Y of -4
  // covers y -64..-49).
  min_y_ = -64;
  size_y_ = 384;
  states_.assign(static_cast<std::size_t>(kSizeX) * size_y_ * kSizeZ, State{});

  // One name table for the whole chunk: every section palette is appended to it,
  // so a State only carries a 16 bit index and the flattened grid built in
  // BuildWorldBlockMeshes stays valid when states are copied between chunks.
  // Index 0 means air / unknown.
  names_.clear();
  names_.push_back(std::string());
  std::map<std::string, std::uint16_t> name_index;

  const auto& sections = kChunkLevel.at("sections").as<nbt::tag_list>();
  bool found_section = false;
  for (auto it = sections.begin(); it != sections.end(); ++it) {
    const auto& section = it->as<nbt::tag_compound>();
    if (!section.has_key("block_states")) {
      continue;
    }
    const int section_y = SectionY(section);
    const auto& states = section.at("block_states").as<nbt::tag_compound>();
    if (!states.has_key("palette")) {
      continue;
    }
    const auto& palette_tag = states.at("palette").as<nbt::tag_list>();
    // Palette entry -> chunk-wide name index
    std::vector<std::uint16_t> palette_indices(palette_tag.size(), 0);
    for (std::size_t i = 0; i < palette_tag.size(); ++i) {
      const auto& entry = palette_tag[i].as<nbt::tag_compound>();
      if (!entry.has_key("Name")) {
        continue;
      }
      const std::string name = entry.at("Name").as<nbt::tag_string>().get();
      if (name.empty() || name == kAirName) {
        continue;
      }
      const auto found = name_index.find(name);
      if (found != name_index.end()) {
        palette_indices[i] = found->second;
        continue;
      }
      const auto index = static_cast<std::uint16_t>(names_.size());
      names_.push_back(name);
      name_index.emplace(name, index);
      palette_indices[i] = index;
    }

    std::vector<std::uint16_t> indices(4096, 0);
    if (states.has_key("data") && !palette_indices.empty()) {
      const auto& data = states.at("data").as<nbt::tag_long_array>();
      const std::vector<std::int64_t>& values = data.get();
      const std::size_t bits = PaletteBits(palette_tag.size());
      const std::size_t per_long = 64 / bits;
      const std::uint64_t mask = (1ull << bits) - 1ull;
      for (std::size_t index = 0; index < indices.size(); ++index) {
        const std::size_t long_index = index / per_long;
        if (long_index >= values.size()) {
          break;
        }
        const std::size_t palette_index = static_cast<std::size_t>(
            (static_cast<std::uint64_t>(values[long_index]) >>
             ((index % per_long) * bits)) &
            mask);
        if (palette_index < palette_indices.size()) {
          indices[index] = palette_indices[palette_index];
        }
      }
    } else if (!palette_indices.empty()) {
      // No data array: a single-entry palette fills the whole section
      std::fill(indices.begin(), indices.end(), palette_indices[0]);
    }

    for (std::size_t index = 0; index < indices.size(); ++index) {
      if (indices[index] == 0) {
        continue;  // air
      }
      const int y = section_y * 16 + static_cast<int>(index >> 8);
      if (y < min_y_ || y >= min_y_ + size_y_) {
        continue;
      }
      const int x = static_cast<int>(index & 15);
      const int z = static_cast<int>((index >> 4) & 15);
      State& state = states_[Index(x, y, z)];
      state.block_id = indices[index];
      state.meta = 0;
    }
    found_section = true;
  }
  return found_section;
}

const std::string& ChunkBlocks::BlockName(const State& kState) const {
  if (kState.block_id >= names_.size()) {
    return kEmptyName;
  }
  return names_[kState.block_id];
}

std::size_t ChunkBlocks::Index(const int kX, const int kY, const int kZ) const {
  // Index order: x + z*16 + (y - min_y) * 256 (YZX, matching Minecraft's chunk
  // storage); y is a world coordinate, so the layout offset is applied here.
  return static_cast<std::size_t>(kX) + static_cast<std::size_t>(kZ) * kSizeX +
         static_cast<std::size_t>(kY - min_y_) * kSizeX * kSizeZ;
}

void ChunkBlocks::MarkLittleTilesHosts(const nbt::tag_list& kTileEntities,
                                       const ChunkCoordinate& kChunkCoord) {
  for (auto it = kTileEntities.begin(); it != kTileEntities.end(); ++it) {
    const auto& entity = it->as<nbt::tag_compound>();
    if (!entity.has_key("x") || !entity.has_key("y") || !entity.has_key("z")) {
      continue;
    }
    // The tile entity's x/y/z are world coordinates; convert them to in-chunk coordinates
    const int x =
        entity.at("x").as<nbt::tag_int>().get() - kChunkCoord.x * kSizeX;
    const int y = entity.at("y").as<nbt::tag_int>().get();
    const int z =
        entity.at("z").as<nbt::tag_int>().get() - kChunkCoord.z * kSizeZ;
    if (x < 0 || x >= kSizeX || y < min_y_ || y >= min_y_ + size_y_ || z < 0 ||
        z >= kSizeZ) {
      continue;
    }
    states_[Index(x, y, z)].set_little_tiles_host(true);
  }
}

void ChunkBlocks::MarkLittleTilesCoverage(
    const littletiles::ChunkTileEntities& kChunkTileEntities) {
  const ChunkCoordinate& chunk_coord = kChunkTileEntities.chunk_coordinate();
  for (auto it = kChunkTileEntities.cbegin(); it != kChunkTileEntities.cend();
       ++it) {
    // The tile entity records world coordinates; convert them to in-chunk coordinates
    const int x = it->block_coordinate().x - chunk_coord.x * kSizeX;
    const int y = it->block_coordinate().y;
    const int z = it->block_coordinate().z - chunk_coord.z * kSizeZ;
    if (x < 0 || x >= kSizeX || y < min_y_ || y >= min_y_ + size_y_ || z < 0 ||
        z >= kSizeZ) {
      continue;
    }
    states_[Index(x, y, z)].set_covered_faces(it->covered_face_mask());
  }
}

const ChunkBlocks::State& ChunkBlocks::At(const int kX, const int kY,
                                          const int kZ) const {
  if (kX < 0 || kX >= kSizeX || kY < min_y_ || kY >= min_y_ + size_y_ ||
      kZ < 0 || kZ >= kSizeZ) {
    return kAirState;
  }
  return states_[Index(kX, kY, kZ)];
}

}  // namespace galib::minecraft
