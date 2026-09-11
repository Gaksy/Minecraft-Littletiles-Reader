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

// 一个区块里的"普通方块"（Level.Sections[].Blocks/Data/Add）。
// LittleTiles 的 tile entity 是挂在普通方块上的，那些方块的外观由 tile 表达，
// 因此导出完整方块时需要把它们排除（见 MarkLittleTilesHosts）。
class ChunkBlocks {
 public:
  static constexpr int kSizeX = 16;
  static constexpr int kSizeY = 256;
  static constexpr int kSizeZ = 16;
  static constexpr std::size_t kBlockCount =
      static_cast<std::size_t>(kSizeX) * kSizeY * kSizeZ;

  struct State {
    // bit0：LittleTiles 宿主位置；bit1..bit6：该位置 6 个面是否被 tile 整面铺满
    // （位序同 TileFaceID：EAST / WEST / SOUTH / NORTH / UP / DOWN）。
    // 打包成 1 字节是为了让整个 State 仍是 4 字节——大范围导出要同时驻留
    // 上百个区块的方块网格，这里每多一字节就是几十 MB。
    static constexpr std::uint8_t kLittleTilesHostFlag = 0x01;

    std::uint16_t block_id{0};  // 0 = 空气
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

  // 解析 Level.Sections（含 Add 高位数组）。没有 Sections 时返回 false。
  bool ReadFromChunkLevel(const nbt::tag_compound& kChunkLevel);

  // 标记 LittleTiles tile entity 所在的方块。
  // 注意：1.12 的 tile entity 里 x/y/z 是**世界坐标**，需要 chunk 坐标才能换算成区块内坐标；
  // 换算错了会一个都匹配不上，结果是 LT 结构下面的地形也被当成普通方块导出。
  void MarkLittleTilesHosts(const nbt::tag_list& kTileEntities,
                            const ChunkCoordinate& kChunkCoord);

  // 记录每个 LittleTiles 宿主位置的"哪些面被 tile 整面铺满"。
  // 完整方块的邻居剔除据此判断：只有铺满的面才真的挡住了相邻方块的表面。
  void MarkLittleTilesCoverage(
      const littletiles::ChunkTileEntities& kChunkTileEntities);

  // 局部坐标访问；越界返回空气
  const State& At(int kX, int kY, int kZ) const;

 private:
  std::vector<State> states_{kBlockCount};
};

}  // namespace galib::minecraft

#endif  // GALIB_MINECRAFT_CHUNKBLOCKS_H
