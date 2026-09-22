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

#ifndef GALIB_MINECRAFT_CHUNKBLOCKS_H
#define GALIB_MINECRAFT_CHUNKBLOCKS_H

#include <nbt_tags.h>

#include <cstdint>
#include <vector>

#include "Minecraft/MinecraftCoord.h"

namespace galib::minecraft::littletiles {
class ChunkTileEntities;
}  // namespace galib::minecraft::littletiles

namespace galib::minecraft {

// The "plain blocks" of a chunk. LittleTiles tile entities are attached to plain
// blocks, and the appearance of those blocks is expressed by the tiles, so they
// must be excluded when exporting full blocks (see MarkLittleTilesHosts).
//
// Two chunk layouts are supported:
//
//   kIds12     1.12.2: `Sections[].Blocks` byte array (+ `Data` metadata nibbles
//              and the `Add` high bits); a block is a numeric registry id, which
//              the assets package turns into a name (see BlockIdTable).
//   kNames118  1.18+ (1.20): `sections[].block_states` palette + packed long
//              array; a block already is its (flattened) name, and the id table is
//              not involved.
//
// The vertical range differs as well: 1.12.2 chunks cover y 0..255, 1.18+ ones
// y -64..319 (min_y()/size_y() report which one this chunk uses).
class ChunkBlocks {
 public:
  static constexpr int kSizeX = 16;
  static constexpr int kSizeZ = 16;
  // Tallest layout (1.18+, 384 rows); older chunks use fewer rows
  static constexpr int kMaxSizeY = 384;

  enum class Layout {
    kNone,     // nothing read yet
    kIds12,    // 1.12.2: numeric ids + metadata
    kNames118  // 1.18+: flattened block names from the section palette
  };

  struct State {
    // bit0: LittleTiles host position; bit1..bit6: whether each of the 6 faces at
    // this position is fully covered by tiles (bit order as in TileFaceID:
    // EAST / WEST / SOUTH / NORTH / UP / DOWN).
    // It is packed into 1 byte so that the whole State stays 4 bytes - a large-area
    // export keeps the block grids of hundreds of chunks resident at once, and every
    // extra byte here costs tens of MB.
    static constexpr std::uint8_t kLittleTilesHostFlag = 0x01;

    std::uint16_t block_id{0};  // 0 = air
    std::uint8_t meta{0};
    std::uint8_t flags{0};

    bool is_air() const { return block_id == 0; }

    bool little_tiles_host() const {
      return (flags & kLittleTilesHostFlag) != 0;
    }

    void set_little_tiles_host(const bool kIsHost) {
      if (kIsHost) {
        flags |= kLittleTilesHostFlag;
      } else {
        flags &= static_cast<std::uint8_t>(~kLittleTilesHostFlag);
      }
    }

    std::uint8_t covered_faces() const {
      return static_cast<std::uint8_t>(flags >> 1);
    }

    void set_covered_faces(const std::uint8_t kMask) {
      flags = static_cast<std::uint8_t>((flags & kLittleTilesHostFlag) |
                                        static_cast<std::uint8_t>(kMask << 1));
    }
  };

  // Parse Level.Sections (including the Add high-bit array). Returns false when
  // there is no Sections tag.
  bool ReadFromChunkLevel(const nbt::tag_compound& kChunkLevel);

  [[nodiscard]] Layout layout() const { return layout_; }
  // Whether block ids have to be resolved through block_ids.tsv
  [[nodiscard]] bool needs_id_table() const {
    return layout_ == Layout::kIds12;
  }

  // World Y of row 0 and the number of rows (1.12.2: 0/256, 1.18+: -64/384).
  [[nodiscard]] int min_y() const { return min_y_; }
  [[nodiscard]] int size_y() const { return size_y_; }

  // 1.18+ only: the flattened name a State refers to (empty in the id layout or
  // when the index is out of range). In the id layout the caller resolves the
  // name through the assets package instead.
  [[nodiscard]] const std::string& BlockName(const State& kState) const;

  // Mark the blocks where LittleTiles tile entities live.
  // Note: in 1.12 the tile entity's x/y/z are **world coordinates**, so the chunk
  // coordinate is needed to convert them to in-chunk coordinates; getting the
  // conversion wrong matches nothing, and the terrain underneath the LT structure
  // ends up being exported as plain blocks too.
  void MarkLittleTilesHosts(const nbt::tag_list& kTileEntities,
                            const ChunkCoordinate& kChunkCoord);

  // Record, for each LittleTiles host position, which faces are fully covered by
  // tiles. Neighbour culling of full blocks relies on this: only a fully covered
  // face really hides the surface of the neighbouring block.
  void MarkLittleTilesCoverage(
      const littletiles::ChunkTileEntities& kChunkTileEntities);

  // Local coordinate access; returns air when out of range
  const State& At(int kX, int kY, int kZ) const;

 private:
  [[nodiscard]] std::size_t Index(int kX, int kY, int kZ) const;
  bool ReadIds12(const nbt::tag_compound& kChunkLevel);
  bool ReadNames118(const nbt::tag_compound& kChunkLevel);

  Layout layout_{Layout::kNone};
  // World Y of row 0 and the number of rows this chunk covers
  int min_y_{0};
  int size_y_{256};
  // kNames118: block_state palette of the whole chunk; State::block_id is an index
  // into it. Kept per chunk (a palette is a few dozen names), never duplicated.
  std::vector<std::string> names_;
  std::vector<State> states_{static_cast<std::size_t>(kSizeX) * 256 * kSizeZ};
};

}  // namespace galib::minecraft

#endif  // GALIB_MINECRAFT_CHUNKBLOCKS_H
