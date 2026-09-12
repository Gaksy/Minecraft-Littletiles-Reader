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
// ChunkTileEntity -> BlockTileEntity -> BoxTileTntity -> TileEntity

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

// tile 的材质键：方块 id + 可选染色。
// LittleTiles 会把同一种方块的不同颜色存成不同的 tile 条目，
// 因此只用 block id 当键会把它们合并/丢弃。
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
  // 未偏移的盒子（grid 单位，原点在方块角上）
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

// 解析 LittleTiles 的盒子数组（存档里的 `box`/`boxes`、结构 SNBT 里的同一个编码）：
// 前 6 个是 (x1,y1,z1,x2,y2,z2)，第 7 个（下标 6）是角度状态位，其后是打包的 16 位偏移。
//
// 规则来自官方源码 `LittleBox.create`：
//   * 长度 == 6                         → 普通 AABB
//   * 数组[6] < 0                       → 带角度偏移（本函数解出 8 个角的偏移与 flip 位）
//   * 长度 == 7 或 11 且 数组[6] >= 0   → 旧 slice 格式，按普通 AABB 处理
// 返回 false 表示这个数组没有角度数据（按普通 AABB 处理）。
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

  // 这个位置的 tile 是否把方块 6 个面"整面铺满"（位序同 TileFaceID：
  // EAST / WEST / SOUTH / NORTH / UP / DOWN）。用于完整方块的邻居剔除：
  // 只有铺满的面才挡得住相邻完整方块的面，花盆这种只占一小块的要保留。
  // 带角度偏移的 tile 是斜面/异形，无法用盒子判断，按"没铺满"处理（保守）。
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

  // 直接读取 chunk 根 NBT，不经过 Anvil / mca 文件。
  // 这是"上传 NBT 数据"这类场景的最小入口：优先取根下的 "Level" 子标签，
  // 若不存在则把根自身当作 level（1.18+ 的扁平结构）。
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
  // 解析 level 下的 "TileEntities" 列表并填充 block_tile_entities_。
  // ReadChunk 与 ReadChunkNbt 共用此实现，保证两条入口行为一致。
  size_type ReadTileEntities(const nbt::tag_compound& kChunkLevelNbt,
                             size_type* p_boxes_count);

 private:
  galib::minecraft::ChunkCoordinate chunk_coordinate_;
  container block_tile_entities_;
};
}  // namespace galib::minecraft::littletiles

#endif  //GALIB_MINECRAFT_LITTLETILES_H
