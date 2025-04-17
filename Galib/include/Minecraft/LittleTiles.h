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
#ifndef GALIB_MINECRAFT_LITTLETILES_H
#define GALIB_MINECRAFT_LITTLETILES_H

#include <cinttypes>
#include <vector>
#include <algorithm>

#include "GalibNamespaceDef.h"
#include "Minecraft/LittleTilesCoord.h"
#include "Minecraft/MinecraftCoord.h"
#include "Minecraft/Anvil.h"
#include "Exception/LittleTilesException.h"

namespace galib::minecraft::littletiles{
    class BlockTiles {
    public:

    };

    class ChunkTiles {
    public:
        enum class AngleOffsetID: GALIB_STD uint8_t {
            EUN = 0, // East Up North
            EUS = 1, // East Up South
            EDN = 2, // East Down North
            EDS = 3, // East Down South
            WUN = 4, // West Up North
            WUS = 5, // West Up South
            WDN = 6, // West Down North
            WDS = 7  // West Down South
        };

        struct AngleOffset {
            bool x_enable   { false };
            bool y_enable   { false };
            bool z_enable   { false };

            OffsetType x_offset { 0 };
            OffsetType y_offset { 0 };
            OffsetType z_offset { 0 };

            GALIB_NODISCARD bool hasAnyEnable()const {
                return x_enable || y_enable || z_enable;
            }
        };

        struct Flipped {
            bool down       { false };
            bool up         { false };
            bool north      { false };
            bool south      { false };
            bool west       { false };
            bool east       { false };

            GALIB_NODISCARD bool hasAnyEnable()const {
                return down || up || north || south || west || east;
            }
        };

        struct Box {
            AngleOffset offset_data[8];
            Flipped flipped_data;
            LittleTilesCoord pos_1 {0, 0, 0};
            LittleTilesCoord pos_2 {0, 0, 0};

            GALIB_NODISCARD bool hasAnyOffsetEnable()const {
                return GALIB_STD any_of(
                    GALIB_STD begin(offset_data),
                    GALIB_STD end(offset_data),
                    [](const AngleOffset& data){return data.hasAnyEnable();}
                );
            }

            GALIB_NODISCARD LittleTilesCoord applyAngleOffset(const AngleOffsetID angle_id)const {
                LittleTilesCoord angle_coord = getVertices(angle_id);
                AngleOffset angle_offset = offset_data[static_cast<size_t>(angle_id)];
                // If unoffset, then offset is 0
                angle_coord.x += angle_offset.x_offset;
                angle_coord.y += angle_offset.y_offset;
                angle_coord.z += angle_offset.z_offset;
                return angle_coord;
            }

            GALIB_NODISCARD LittleTilesCoord getVertices(const AngleOffsetID angle_id)const {
                switch (angle_id) {
                    case AngleOffsetID::WDS:
                        return {pos_1.x, pos_1.y, pos_2.z};
                    case AngleOffsetID::WDN:
                        return {pos_1.x, pos_1.y, pos_1.z};
                    case AngleOffsetID::EDN:
                        return {pos_2.x, pos_1.y, pos_1.z};
                    case AngleOffsetID::EDS:
                        return {pos_2.x, pos_1.y, pos_2.z};
                    case AngleOffsetID::WUN:
                        return {pos_1.x, pos_2.y, pos_1.z};
                    case AngleOffsetID::WUS:
                        return {pos_1.x, pos_2.y, pos_2.z};
                    case AngleOffsetID::EUS:
                        return {pos_2.x, pos_2.y, pos_2.z};
                    case AngleOffsetID::EUN:
                        return {pos_2.x, pos_2.y, pos_1.z};
                    default:
                        throw exception::LittleTilesException(exception::LittleTilesErrorCode::lt_unknow_angle);
                }
            }
        };

        using Boxes = GALIB_STD vector<Box>;

        struct BlockTiles {
            using const_iterator = GALIB_STD map<GALIB_STD string, Boxes>::const_iterator;
            // <block_id, array of box>
            using container = GALIB_STD map<GALIB_STD string, Boxes>;

            GALIB minecraft::BlockCoordinate block_coordinate;
            GridType grid;
            GALIB_STD string little_tiles_id;
            container boxes_map;
        };

        using const_iterator = GALIB_STD vector<BlockTiles>::const_iterator;
        using container = GALIB_STD vector<BlockTiles>;

    public:
        ChunkTiles();
        ~ChunkTiles()=default;

    public:
        bool ReadChunk(const GALIB minecraft::AnvilReader::ChunkDataReference& kChunkDataReference);
        GALIB_NODISCARD const GALIB minecraft::ChunkCoordinate& GetChunkCoordinate()const;

        GALIB_NODISCARD const_iterator cbegin()const;
        GALIB_NODISCARD const_iterator cend()const;

        void Clear();
        GALIB_NODISCARD bool IsEmpty()const;

    private:
        GALIB minecraft::ChunkCoordinate chunk_coordinate_;
        container block_tileses_;
    };
}

#endif //GALIB_MINECRAFT_LITTLETILES_H
