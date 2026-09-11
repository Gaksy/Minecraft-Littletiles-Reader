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

#include "Exception/LittleTilesException.h"
#include "Exception/MinecraftException.h"
#include "GalibNamespaceDef.h"
#include "Minecraft/LittleTiles.h"
#include "Minecraft/MinecraftCoord.h"

using GALIB_STD size_t;

using GALIB minecraft::littletiles::ChunkTileEntities;
using GALIB minecraft::ChunkCoordinate;

using GALIB exception::MinecraftException;
using GALIB exception::MinecraftErrorCode;
using GALIB exception::LittleTilesException;
using GALIB exception::LittleTilesErrorCode;

using GALIB_NBT tag_compound;
using GALIB_NBT tag_list;

GALIB minecraft::littletiles::ChunkTileEntities::ChunkTileEntities()
    : chunk_coordinate_({}) {}

ChunkTileEntities::size_type ChunkTileEntities::readChunk(
    const AnvilReader::ChunkDataReference& kChunkDataReference,
    size_type* p_boxes_count) {
  // Check chunk root is not empty
  if (!kChunkDataReference.p_chunk_root) {
    throw MinecraftException(MinecraftErrorCode::mc_nbt_empty,
                             "The chunk NBT root node is empty.", "ChunkTiles");
  }

  // Check chunk level is not empty
  if (!kChunkDataReference.p_chunk_level) {
    throw MinecraftException(MinecraftErrorCode::mc_nbt_empty,
                             "The chunk NBT level node is empty.",
                             "ChunkTiles");
  }

  chunk_coordinate_ = kChunkDataReference.chunk_info.chunk_coord;
  return readTileEntities_(*kChunkDataReference.p_chunk_level, p_boxes_count);
}

ChunkTileEntities::size_type ChunkTileEntities::readChunkNbt(
    const tag_compound& kChunkRootNbt, size_type* p_boxes_count) {
  // Check chunk root is not empty
  if (!kChunkRootNbt.size()) {
    throw MinecraftException(MinecraftErrorCode::mc_nbt_empty,
                             "The chunk NBT root node is empty.", "ChunkTiles");
  }

  // 1.12 之前的结构为 root -> "Level" -> "TileEntities"；
  // 1.18+ 把 level 的内容摊平到了根上，此时直接把根当作 level。
  const tag_compound& chunk_level =
      kChunkRootNbt.has_key("Level")
          ? kChunkRootNbt.at("Level").as<tag_compound>()
          : kChunkRootNbt;

  // 若 level 内带有区块坐标，则同步到 chunk 坐标
  if (chunk_level.has_key("xPos") && chunk_level.has_key("zPos")) {
    chunk_coordinate_ = {chunk_level.at("xPos").as<GALIB_NBT tag_int>().get(),
                         chunk_level.at("zPos").as<GALIB_NBT tag_int>().get()};
  }

  return readTileEntities_(chunk_level, p_boxes_count);
}

ChunkTileEntities::size_type ChunkTileEntities::readTileEntities_(
    const tag_compound& kChunkLevelNbt, size_type* p_boxes_count) {
  // Chenk TileEnities is exist
  if (!kChunkLevelNbt.has_key("TileEntities")) {
    throw LittleTilesException(LittleTilesErrorCode::lt_tage_not_exist,
                               "The NBT \"TileEntities\" tag does not exist",
                               "ChunkTiles");
  }

  // Get TileEnities
  const tag_list& tiles_entities =
      kChunkLevelNbt.at("TileEntities").as<tag_list>();
  container block_tile_entities;

  size_type tile_count = 0;
  size_type boxes_count = 0;

#ifdef GALIB_DEBUG
  printf("ChunkTileEntities::readChunk read chunk: %d %d\n",
         chunk_coordinate_.x, chunk_coordinate_.z);
#endif

  // Decode...
  for (tag_list::const_iterator it = tiles_entities.cbegin();
       it != tiles_entities.cend(); ++it) {
    try {
      BlockTileEntities block_tiles;
      size_type block_boxes_count = 0;
      tile_count += block_tiles.readBlockTileNBT(it->as<tag_compound>(),
                                                 &block_boxes_count);
      boxes_count += block_boxes_count;
      block_tile_entities.push_back(block_tiles);
    }
#ifndef GALIB_DEBUG
    catch (...) {
    }
#else
    catch (const GALIB_STD exception& e) {
      printf("ChunkTileEntities::readChunk error: %s\n", e.what());
    }
#endif
  }

  // DONE!!
  block_tile_entities_.swap(block_tile_entities);

#ifdef GALIB_DEBUG
  printf("ChunkTileEntities::readChunk Tile count: %zu, Boxes count: %zu\n",
         tile_count, boxes_count);
#endif

  if (p_boxes_count) {
    *p_boxes_count = boxes_count;
  }

  return tile_count;
}

const ChunkCoordinate& ChunkTileEntities::getChunkCoordinate() const {
  return chunk_coordinate_;
}

ChunkTileEntities::const_iterator ChunkTileEntities::cbegin() const {
  return block_tile_entities_.cbegin();
}

ChunkTileEntities::const_iterator ChunkTileEntities::cend() const {
  return block_tile_entities_.cend();
}

void ChunkTileEntities::clear() {
  chunk_coordinate_ = {0, 0};
  block_tile_entities_.clear();
}

bool ChunkTileEntities::isEmpty() const { return block_tile_entities_.empty(); }

ChunkTileEntities::size_type ChunkTileEntities::tileCount() const {
  size_t num = 0;
  for (const_iterator chunk_it = cbegin(); chunk_it != cend(); ++chunk_it) {
    num += chunk_it->tileCount();
  }
  return num;
}
