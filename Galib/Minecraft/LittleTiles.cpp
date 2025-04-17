/*
 * This Source Code Form is subject to the terms of the GNU Lesser General Public License,
 * v. 3.0. If a copy of the LGPL was not distributed with this file, You can obtain one at
 * https://www.gnu.org/licenses/lgpl-3.0.html.
 *
 * Copyright (c) 2024 Gaksy (Fuhongren)
 *
 * Author: Gaksy
 * Date Created: 4/17/2025
 *
 * Additional Terms:
 * For commercial use of this software, you must obtain separate authorization from the original author.
 * Please contact Gaksy at gaksys@outlook.com to request commercial use authorization.
 */

#include "Minecraft/LittleTiles.h"
#include "Exception/LittleTilesException.h"
#include "Exception/MinecraftException.h"
#include "GalibNamespaceDef.h"

using GALIB minecraft::littletiles::GridType;
using GALIB minecraft::littletiles::ChunkTileEntities;
using GALIB minecraft::littletiles::BlockTileEntities;

using GALIB minecraft::BlockCoordinate;
using GALIB minecraft::ChunkCoordinate;

using GALIB exception::MinecraftException;
using GALIB exception::MinecraftErrorCode;
using GALIB exception::LittleTilesException;
using GALIB exception::LittleTilesErrorCode;



BlockTileEntities::BlockTileEntities():
    block_coordinate_({0, 0, 0}),
    grid_(0)
{ }

void BlockTileEntities::readBlockTileNBT(const nbt::tag_compound &kBlockTilesNBT) {
    // check block tiles root is not empty
    if (!kBlockTilesNBT.size()) {
        throw LittleTilesException(LittleTilesErrorCode::lt_tage_not_exist, "The block tile data does not exist.", "BlockTiles");
    }

    try {
        // Get block coord
        BlockCoordinate block_coord = {
            kBlockTilesNBT.at("x").as<GALIB_NBT tag_int>().get(),
            kBlockTilesNBT.at("y").as<GALIB_NBT tag_int>().get(),
            kBlockTilesNBT.at("z").as<GALIB_NBT tag_int>().get()
        };

        // By default
        GridType grid_type = 16;

        if (kBlockTilesNBT.has_key("grid")) {
            grid_type = kBlockTilesNBT.at("grid").as<GALIB_NBT tag_int>().get();
        }

        // Get ID
        GALIB_STD string little_tiles_id = kBlockTilesNBT.at("id").as<GALIB_NBT tag_string>();

        // Get tiles list
        GALIB_NBT tag_list tiles = kBlockTilesNBT.at("content").at("tiles").as<nbt::tag_list>();

        // Process block tiles
        GALIB_STD string block_id;
        GALIB_NBT tag_compound* p_boxes;
        container box_tile_enities_map;

        for (GALIB_NBT tag_list::iterator it = tiles.begin(); it != tiles.cend(); ++it) {
            p_boxes = &it->as<GALIB_NBT tag_compound>();   // Get boxes
            block_id = p_boxes->at("block").as<GALIB_NBT tag_string>().get(); // Get block id

            // is haved block_id
            if(box_tile_enities_map.find(block_id) != box_tile_enities_map.end()) {
                continue;
            }

            // Get data
            BoxTileEnities box_tile_enities;
        }
    }
    catch (...) {
        throw LittleTilesException(LittleTilesErrorCode::lt_tage_not_exist, "Some tag not exist.", "BlockTiles");
    }
}

const BlockCoordinate & BlockTileEntities::getBlockCoordinate() const {
    return block_coordinate_;
}

const GridType & BlockTileEntities::getGridType() const {
    return grid_;
}

const std::string & BlockTileEntities::getLittleTilesID() const {
    return little_tiles_id_;
}

BlockTileEntities::const_iterator BlockTileEntities::cbegin() const {
    return box_tile_enities_map_.cbegin();
}

BlockTileEntities::const_iterator BlockTileEntities::cend() const {
    return box_tile_enities_map_.cend();
}

GALIB minecraft::littletiles::ChunkTileEntities::ChunkTileEntities():
	chunk_coordinate_({})
{ }

void ChunkTileEntities::ReadChunk(
    const AnvilReader::ChunkDataReference &kChunkDataReference
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
    if (!kChunkDataReference.p_chunk_root->has_key("TileEnities")) {
        throw LittleTilesException(LittleTilesErrorCode::lt_tage_not_exist, "The NBT \"TileEnities\" tag does not exist", "ChunkTiles");
    }

    // Get TileEnities
    const GALIB_NBT tag_list& tiles_entities = kChunkDataReference.p_chunk_root->at("TileEnities").as<GALIB_NBT tag_list>();
    container block_tile_entities;

    // Decode...
    for(GALIB_NBT tag_list::const_iterator it = tiles_entities.cbegin(); it != tiles_entities.cend(); ++it) {
        try {
            BlockTileEntities block_tiles;
            block_tiles.readBlockTileNBT(it->as<GALIB_NBT tag_compound>);
            block_tile_entities.push_back(block_tiles);
        }
        catch (...) { }
    }

    // DONE!!
    chunk_coordinate_ = kChunkDataReference.chunk_info.chunk_coord;
    block_tile_entities_.swap(block_tile_entities);
}

const ChunkCoordinate & ChunkTileEntities::GetChunkCoordinate() const {
    return chunk_coordinate_;
}

ChunkTileEntities::const_iterator ChunkTileEntities::cbegin() const {
    return block_tile_entities_.cbegin();
}

ChunkTileEntities::const_iterator ChunkTileEntities::cend() const {
    return block_tile_entities_.cend();
}

void ChunkTileEntities::Clear() {
    chunk_coordinate_ = {0, 0};
    block_tile_entities_.clear();
}

bool ChunkTileEntities::IsEmpty() const {
    return block_tile_entities_.empty();
}

