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

// The "plain blocks" of a chunk (Level.Sections[].Blocks/Data/Add).
// LittleTiles tile entities are attached to plain blocks, and the appearance of
// those blocks is expressed by the tiles, so they must be excluded when exporting
// full blocks (see MarkLittleTilesHosts).
class ChunkBlocks {
 public:
  static constexpr int kSizeX = 16;
  static constexpr int kSizeY = 256;
  static constexpr int kSizeZ = 16;
  static constexpr std::size_t kBlockCount =
      static_cast<std::size_t>(kSizeX) * kSizeY * kSizeZ;

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
  std::vector<State> states_{kBlockCount};
};

}  // namespace galib::minecraft

#endif  // GALIB_MINECRAFT_CHUNKBLOCKS_H
