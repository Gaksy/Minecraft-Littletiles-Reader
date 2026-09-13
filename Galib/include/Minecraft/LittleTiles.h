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
 * Date Created: 4/17/2025
 */

#ifndef GALIB_MINECRAFT_LITTLETILES_H
#define GALIB_MINECRAFT_LITTLETILES_H

#include <vector>

#include "GalibNamespaceDef.h"
#include "Minecraft/Anvil.h"
#include "Minecraft/LittleTilesCoord.h"
#include "Minecraft/MinecraftCoord.h"

namespace galib::minecraft::littletiles {
// ChunkTileEntity -> BlockTileEntity -> BoxTileEntity -> TileEntity

enum class AngleID : std::uint8_t {
  EUN = 0,  // East Up North
  EUS = 1,  // East Up South
  EDN = 2,  // East Down North
  EDS = 3,  // East Down South
  WUN = 4,  // West Up North
  WUS = 5,  // West Up South
  WDN = 6,  // West Down North
  WDS = 7   // West Down South
};

enum class TileFaceID : std::uint8_t {
  EAST = 0,   // East
  WEST = 1,   // West
  SOUTH = 2,  // South
  NORTH = 3,  // North
  UP = 4,     // Up
  DOWN = 5    // Down
};

struct AngleOffset {
  bool x_enable{false};
  bool y_enable{false};
  bool z_enable{false};

  OffsetType x_offset{0};
  OffsetType y_offset{0};
  OffsetType z_offset{0};

  [[nodiscard]] bool has_any_enable() const {
    return x_enable || y_enable || z_enable;
  }
};

std::uint8_t ConvertAngleIdToInt(AngleID kAngleId);
AngleID ConvertIntToAngleId(std::uint8_t kNumId);

struct Flipped {
  bool down{false};
  bool up{false};
  bool north{false};
  bool south{false};
  bool west{false};
  bool east{false};

  [[nodiscard]] bool has_any_enable() const {
    return down || up || north || south || west || east;
  }
};

struct TileFace {
  LittleTilesCoord pos_1{0, 0, 0};
  LittleTilesCoord pos_2{0, 0, 0};
  LittleTilesCoord pos_3{0, 0, 0};
  LittleTilesCoord pos_4{0, 0, 0};
};

// Material key of a tile: block id + optional tint.
// LittleTiles stores different colours of the same block as different tile entries,
// so keying on the block id alone would merge or drop them.
struct TileMaterial {
  std::string block_id;
  std::int32_t color{0};
  bool has_color{false};

  bool operator<(const TileMaterial& kRhs) const {
    if (block_id != kRhs.block_id) {
      return block_id < kRhs.block_id;
    }
    if (has_color != kRhs.has_color) {
      return has_color < kRhs.has_color;
    }
    return color < kRhs.color;
  }
};

class TileEntity {
 public:
  TileEntity();
  ~TileEntity() = default;

 public:
  [[nodiscard]] bool has_any_offset_enable() const;
  [[nodiscard]] LittleTilesCoord ApplyAngleOffset(AngleID kAngleId) const;
  [[nodiscard]] AngleOffset GetAngleOffset(AngleID kAngleID) const;
  [[nodiscard]] LittleTilesCoord GetVertices(AngleID kAngleId) const;
  [[nodiscard]] LittleTilesCoord GetVertices(AngleID kAngleId,
                                             bool kWithOffset) const;
  [[nodiscard]] LittleTilesCoord GetVerticesApplyGrid(AngleID kAngleId,
                                                      GridType kGridType,
                                                      bool kWithOffset) const;
  [[nodiscard]] TileFace GetTileFace(TileFaceID kTileFaceID,
                                     bool kWithOffset = false) const;
  [[nodiscard]] const Flipped& flipped_data() const;
  [[nodiscard]] bool is_offset_off_boundary() const;
  [[nodiscard]] bool has_color() const;
  [[nodiscard]] std::int32_t color() const;
  // The unoffset box (in grid units, with the origin at the block corner)
  [[nodiscard]] const LittleTilesCoord& pos_1() const;
  [[nodiscard]] const LittleTilesCoord& pos_2() const;
  void set_pos(const LittleTilesCoord& kPos1, const LittleTilesCoord& kPos2);
  void set_flipped_data(const Flipped& kFlippedData);
  void set_color(std::int32_t kColor, bool kHasColor);
  void set_offset_data(AngleID kAngleId, const AngleOffset& kAngleOffsetData);
  void set_offset_data(const AngleOffset kOffsetData[8]);

 private:
  AngleOffset offset_data_[8];
  Flipped flipped_data_;
  LittleTilesCoord pos_1_;
  LittleTilesCoord pos_2_;
  std::int32_t color_{0};
  bool has_color_{false};
};

using BoxTileEnities = std::vector<TileEntity>;

// Parse a LittleTiles box array (the `box`/`boxes` in saves and the same encoding in
// structure SNBT): the first 6 entries are (x1,y1,z1,x2,y2,z2), the 7th (index 6) is
// the angle state bits, and the rest are packed 16-bit offsets.
//
// The rules come from the official source `LittleBox.create`:
//   * length == 6                         -> plain AABB
//   * array[6] < 0                        -> has angle offsets (this function decodes the offsets of the 8 corners and the flip bits)
//   * length == 7 or 11 and array[6] >= 0 -> legacy slice format, treated as a plain AABB
// Returning false means the array has no angle data (treat it as a plain AABB).
bool DecodeBoxAngleData(const std::vector<std::int32_t>& kBoxArray,
                        AngleOffset kOffsets[8], Flipped* p_desc_flipped);

class BlockTileEntities {
 public:
  using const_iterator = std::map<TileMaterial, BoxTileEnities>::const_iterator;
  // <material(block_id + color), array of box>
  using container = std::map<TileMaterial, BoxTileEnities>;
  using container_pair = std::pair<TileMaterial, BoxTileEnities>;
  using size_type = std::map<TileMaterial, BoxTileEnities>::size_type;

 public:
  BlockTileEntities();
  ~BlockTileEntities() = default;

 public:
  size_type ReadBlockTileNbt(const nbt::tag_compound& kBlockTilesNBT,
                             size_type* p_boxes_count = nullptr);

  [[nodiscard]] const galib::minecraft::BlockCoordinate& block_coordinate()
      const;
  [[nodiscard]] const GridType& grid() const;
  [[nodiscard]] const std::string& little_tiles_id() const;

  [[nodiscard]] const_iterator cbegin() const;
  [[nodiscard]] const_iterator cend() const;

  [[nodiscard]] size_type TileCount() const;

  // Whether the tiles at this position fully cover all 6 faces of the block (bit
  // order as in TileFaceID: EAST / WEST / SOUTH / NORTH / UP / DOWN). Used for
  // neighbour culling of full blocks: only a fully covered face can hide the face of
  // an adjacent full block, while something like a flower pot that occupies only a
  // small part must be kept.
  // A tile with angle offsets is a slanted/irregular shape that cannot be judged from
  // its box, so it is treated as "not fully covered" (conservative).
  [[nodiscard]] std::uint8_t covered_face_mask() const;

 private:
  [[nodiscard]] static bool ReadBoxesTilesNbt(
      const nbt::tag_compound& kBoxesTilesNbt,
      BoxTileEnities& desc_box_tile_enities, size_type& tile_count);

  [[nodiscard]] static bool SetAngleOffsetStateData(
      const nbt::tag_int_array& offset_nbt, AngleOffset* p_offset_data,
      Flipped* p_flipped_data);

 private:
  galib::minecraft::BlockCoordinate block_coordinate_;
  std::string little_tiles_id_;
  GridType grid_;
  container box_tile_entities_map_;
};

class ChunkTileEntities {
 public:
  using const_iterator = std::vector<BlockTileEntities>::const_iterator;
  using container = std::vector<BlockTileEntities>;
  using size_type = std::vector<BlockTileEntities>::size_type;

 public:
  ChunkTileEntities();
  ~ChunkTileEntities() = default;

 public:
  size_type ReadChunk(const galib::minecraft::AnvilReader::ChunkDataReference&
                          kChunkDataReference,
                      size_type* p_boxes_count = nullptr);

  // Read the chunk root NBT directly, bypassing Anvil / mca files.
  // This is the minimal entry point for scenarios such as "upload NBT data": the
  // "Level" child tag under the root is preferred, and if it does not exist the root
  // itself is treated as the level (the 1.18+ flattened structure).
  size_type ReadChunkNbt(const nbt::tag_compound& kChunkRootNbt,
                         size_type* p_boxes_count = nullptr);

  [[nodiscard]] const galib::minecraft::ChunkCoordinate& chunk_coordinate()
      const;

  [[nodiscard]] const_iterator cbegin() const;
  [[nodiscard]] const_iterator cend() const;

  void Clear();
  [[nodiscard]] bool isEmpty() const;

  [[nodiscard]] size_type TileCount() const;

 private:
  // Parse the "TileEntities" list under level and fill block_tile_entities_.
  // ReadChunk and ReadChunkNbt share this implementation so both entry points behave
  // identically.
  size_type ReadTileEntities(const nbt::tag_compound& kChunkLevelNbt,
                             size_type* p_boxes_count);

 private:
  galib::minecraft::ChunkCoordinate chunk_coordinate_;
  container block_tile_entities_;
};
}  // namespace galib::minecraft::littletiles

#endif  //GALIB_MINECRAFT_LITTLETILES_H
