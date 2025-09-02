/*
 * This Source Code Form is subject to the terms of the GNU Lesser General Public License,
 * v. 3.0. If a copy of the LGPL was not distributed with this file, You can obtain one at
 * https://www.gnu.org/licenses/lgpl-3.0.html.
 *
 * Copyright (c) 2024 Gaksy (Fuhongren)
 *
 * Author: Gaksy
 * Date Created: 12/22/2024
 *
 * Additional Terms:
 * For commercial use of this software, you must obtain separate authorization from the original author.
 * Please contact Gaksy at gaksys@outlook.com to request commercial use authorization.
 */

#include "GalibNamespaceDef.h"
#include "Minecraft/LittleTiles.h"
#include "Minecraft/MinecraftCoord.h"
#include "Exception/MinecraftException.h"
#include "Exception/LittleTilesException.h"

using GALIB_STD size_t;

using GALIB minecraft::littletiles::ChunkTileEntities;
using GALIB minecraft::ChunkCoordinate;

using GALIB exception::MinecraftException;
using GALIB exception::MinecraftErrorCode;
using GALIB exception::LittleTilesException;
using GALIB exception::LittleTilesErrorCode;

using GALIB_NBT tag_compound;
using GALIB_NBT tag_list;

GALIB minecraft::littletiles::ChunkTileEntities::ChunkTileEntities():
    chunk_coordinate_({})
{ }

ChunkTileEntities::size_type ChunkTileEntities::readChunk(
    const AnvilReader::ChunkDataReference &kChunkDataReference,
    size_type* p_boxes_count
) {
    // Check chunk root is not empty
    if (!kChunkDataReference.p_chunk_root) {
        throw MinecraftException(
            MinecraftErrorCode::mc_nbt_empty,
            "The chunk NBT root node is empty.",
            "ChunkTiles"
        );
    }

    // Chenk TileEnities is exist
    if (!kChunkDataReference.p_chunk_level->has_key("TileEntities")) {
        throw LittleTilesException(LittleTilesErrorCode::lt_tage_not_exist, "The NBT \"TileEntities\" tag does not exist", "ChunkTiles");
    }

    // Get TileEnities
    const tag_list& tiles_entities = kChunkDataReference.p_chunk_level->at("TileEntities").as<tag_list>();
    container block_tile_entities;

    size_type tile_count = 0;
    size_type boxes_count = 0;

    // Decode...
    for(tag_list::const_iterator it = tiles_entities.cbegin(); it != tiles_entities.cend(); ++it) {
        try {
            BlockTileEntities block_tiles;
            size_type block_boxes_count = 0;
            tile_count += block_tiles.readBlockTileNBT(it->as<tag_compound>(), &block_boxes_count);
            boxes_count += block_boxes_count;
            block_tile_entities.push_back(block_tiles);
        }
#ifndef GALIB_DEBUG
        catch (...) { }
#else
        catch (const GALIB_STD exception& e) {
            printf("ChunkTileEntities::readChunk error: %s\n", e.what());
        }
#endif
    }

    // DONE!!
    chunk_coordinate_ = kChunkDataReference.chunk_info.chunk_coord;
    block_tile_entities_.swap(block_tile_entities);

#ifdef GALIB_DEBUG
    printf("ChunkTileEntities::readChunk Tile count: %zu, Boxes count: %zu\n", tile_count, boxes_count);
#endif

    return tile_count;
}

const ChunkCoordinate & ChunkTileEntities::getChunkCoordinate() const {
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

bool ChunkTileEntities::isEmpty() const {
    return block_tile_entities_.empty();
}

ChunkTileEntities::size_type ChunkTileEntities::tileCount() const {
    size_t num = 0;
    for (const_iterator chunk_it = cbegin(); chunk_it != cend(); ++chunk_it) {
        num += chunk_it->tileCount();
    }
    return num;
}

