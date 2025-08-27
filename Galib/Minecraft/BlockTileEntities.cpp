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

#include <iostream>
#include "GalibNamespaceDef.h"
#include "Minecraft/LittleTiles.h"
#include "Minecraft/MinecraftCoord.h"
#include "Exception/LittleTilesException.h"
#include "nbt_tags.h"

using GALIB minecraft::littletiles::GridType;
using GALIB minecraft::littletiles::BlockTileEntities;
using GALIB minecraft::littletiles::AngleOffset;
using GALIB minecraft::littletiles::Flipped;

using GALIB minecraft::BlockCoordinate;

using GALIB exception::LittleTilesException;
using GALIB exception::LittleTilesErrorCode;

using GALIB_STD string;
using GALIB_STD uint32_t;
using GALIB_STD vector;

using GALIB_NBT tag_int;
using GALIB_NBT tag_string;
using GALIB_NBT tag_compound;
using GALIB_NBT tag_list;
using GALIB_NBT tag_int_array;

BlockTileEntities::BlockTileEntities():
    block_coordinate_({0, 0, 0}),
    grid_(0)
{ }

void BlockTileEntities::readBlockTileNBT(const tag_compound &kBlockTilesNBT) {
    // check block tiles root is not empty
    if (!kBlockTilesNBT.size()) {
        throw LittleTilesException(
            LittleTilesErrorCode::lt_tage_not_exist,
            "The block tile data does not exist.",
            "BlockTiles"
        );
    }

    try {

        // By default
        GridType grid_type = 16;

        if (kBlockTilesNBT.has_key("grid")) {
            grid_type = kBlockTilesNBT.at("grid").as<tag_int>().get();
        }

        // Get ID
        string little_tiles_id = kBlockTilesNBT.at("id").as<tag_string>();

        // Get tiles list
        auto tiles = kBlockTilesNBT.at("content").at("tiles").as<tag_list>();

        // Process block tiles
        container box_tile_enities_map;

        for (auto it = tiles.begin(); it != tiles.cend(); ++it) {
            tag_compound* p_boxes = &it->as<tag_compound>();   // Get boxes
            string block_id = p_boxes->at("block").as<tag_string>().get(); // Get block id

            // is haved block_id
            if(box_tile_enities_map.find(block_id) != box_tile_enities_map.end()) {
                continue;
            }

            // Get data, If Get successful, then insert data
            if(BoxTileEnities box_tile_enities; readBoxesTilesNbt_(*p_boxes, box_tile_enities)) {
                box_tile_enities_map.insert(container_pair(block_id, box_tile_enities));
            }
        }

        // Get block coord
        block_coordinate_ = {
            kBlockTilesNBT.at("x").as<tag_int>().get(),
            kBlockTilesNBT.at("y").as<tag_int>().get(),
            kBlockTilesNBT.at("z").as<tag_int>().get()
        };

        // DONE!!
        grid_ = grid_type;
        box_tile_entities_map_.swap(box_tile_enities_map);
        little_tiles_id_.swap(little_tiles_id);
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
    return box_tile_entities_map_.cbegin();
}

BlockTileEntities::const_iterator BlockTileEntities::cend() const {
    return box_tile_entities_map_.cend();
}

bool BlockTileEntities::readBoxesTilesNbt_(
    const tag_compound &kBoxesTilesNbt,
    BoxTileEnities &desc_box_tile_enities
) {
    if (!kBoxesTilesNbt.size()) { return false; }

    try {
        tag_list boxes_pos;

        // Get struct
        if (kBoxesTilesNbt.has_key("boxes")) {
            boxes_pos = kBoxesTilesNbt.at("boxes").as<tag_list>();
        }
        else if (kBoxesTilesNbt.has_key("box")) {
            boxes_pos.push_back(nbt::value_initializer(kBoxesTilesNbt.at("box").as<tag_int_array>().clone()));
        }
        else {
            return false;
        }

        // Get tiles data
        BoxTileEnities box_tile_enity_array;

        for (tag_list::const_iterator it = boxes_pos.cbegin(); it != boxes_pos.cend(); ++it) {
            try {
                TileEntity temp;
                // const nbt::tag_int_array& int_array = it->as<nbt::tag_int_array>();
                // const auto& int_array = it->as<nbt::tag_int_array>();
                // std::cerr << "boxes element type: " << *it << std::endl;

                const tag_int_array& int_array = it->as<tag_int_array>();
                // printf("a");

                // const tag_int_array& int_array = it->as<tag_int_array>();
                //
                if (int_array.size() < 6) { continue; }     // pos must have 6 num (two vertices)
                if (int_array.size() > 6) {                 // if > 6 , then have offset and flipped
                    AngleOffset angle_offset_data[8];
                    Flipped flipped_data;
                    // Get flipped and offert data
                    if(!setAngleOffsetStateData_(int_array, angle_offset_data, &flipped_data)) {
                        // if error
                        continue;
                    }
                    temp.setFlippedData(flipped_data);
                    temp.setOffsetData(angle_offset_data);
                }
                LittleTilesCoord pos_1;
                LittleTilesCoord pos_2;
                pos_1.x = int_array[0];
                pos_1.y = int_array[1];
                pos_1.z = int_array[2];
                pos_2.x = int_array[3];
                pos_2.y = int_array[4];
                pos_2.z = int_array[5];
                temp.setPos(pos_1, pos_2);

                box_tile_enity_array.push_back(temp);
            } catch (const std::exception &e) {
                std::cerr << "Error parsing box tile entity: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Unknown error parsing box tile entity." << std::endl;
            }
        }

        // Cheak data
        if (box_tile_enity_array.empty()){ return true; }
        // Save data
        desc_box_tile_enities.swap(box_tile_enity_array);
    }
    catch ( ... ) {
        return false;
    }
    return true;
}

bool BlockTileEntities::setAngleOffsetStateData_(
    const tag_int_array &offset_nbt,
    AngleOffset *p_offset_data,
    Flipped *p_flipped_data
) {
        // Check nbt size, if < 7, the angle change data is null
    if (offset_nbt.size() < 7) { return false; }

    // Get angle change data state iterator and create change data buffer
    const auto it = offset_nbt.cbegin() + 6;
    const uint32_t state_binary = *it;
    vector<OffsetType> angle_offset_array;

    // Get Angle offset data array
    if (it + 1 != offset_nbt.cend()) {
        for (auto offset_it = it + 1; offset_it != offset_nbt.cend(); ++offset_it) {
            angle_offset_array.push_back(static_cast<OffsetType>((*offset_it & 0xFFFF0000) >> 16));     // Get 16bit
            angle_offset_array.push_back(static_cast<OffsetType>(*offset_it & 0x0000FFFF));             // Get 16bit
        }
    }

    // Get Angle offset state and set offset value
    auto offset_it = angle_offset_array.cbegin();
    const auto offset_it_end = angle_offset_array.cend();
    for(size_t angle_id = 0; angle_id < 8; ++angle_id) {
        p_offset_data->x_enable = state_binary & (0x1 << (angle_id * 3));
        p_offset_data->y_enable = state_binary & (0x2 << (angle_id * 3));
        p_offset_data->z_enable = state_binary & (0x4 << (angle_id * 3));

        if (p_offset_data->hasAnyEnable()) {
            if (p_offset_data->x_enable && offset_it != offset_it_end) {
                p_offset_data->x_offset = *(offset_it++);
            }
            if (p_offset_data->y_enable && offset_it != offset_it_end) {
                p_offset_data->y_offset = *(offset_it++);
            }
            if (p_offset_data->z_enable && offset_it != offset_it_end) {
                p_offset_data->z_offset = *(offset_it++);
            }
        }
        p_offset_data++;
    }

    // Get Flipped
    p_flipped_data->down    = state_binary & (0x1 << (8 * 3));
    p_flipped_data->up      = state_binary & (0x2 << (8 * 3));
    p_flipped_data->north   = state_binary & (0x4 << (8 * 3));
    p_flipped_data->south   = state_binary & (0x1 << (9 * 3));
    p_flipped_data->west    = state_binary & (0x2 << (9 * 3));
    p_flipped_data->east    = state_binary & (0x4 << (9 * 3));

    return true;
}
