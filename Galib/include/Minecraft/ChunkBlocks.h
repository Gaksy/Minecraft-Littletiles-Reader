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
    std::uint16_t block_id{0};  // 0 = 空气
    std::uint8_t meta{0};
    bool little_tiles_host{false};

    bool is_air() const { return block_id == 0; }
  };

  // 解析 Level.Sections（含 Add 高位数组）。没有 Sections 时返回 false。
  bool ReadFromChunkLevel(const nbt::tag_compound& kChunkLevel);

  // 标记 LittleTiles tile entity 所在的方块位置（区块内坐标）。
  void MarkLittleTilesHosts(const nbt::tag_list& kTileEntities);

  // 局部坐标访问；越界返回空气
  const State& At(int kX, int kY, int kZ) const;

 private:
  std::vector<State> states_{kBlockCount};
};

}  // namespace galib::minecraft

#endif  // GALIB_MINECRAFT_CHUNKBLOCKS_H
