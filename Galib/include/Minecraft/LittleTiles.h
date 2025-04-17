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
#include "Minecraft/Anvil.h"
#include "Exception/LittleTilesException.h"

namespace galib::minecraft::littletiles{
    class BlockTiles {
    public:

    };

    class ChunkTiles {
    public:
        enum class AngleOffsetID: GALIB_STD uint8_t {
            EUN, // East Up North
            EUS, // East Up South
            EDN, // East Down North
            EDS, // East Down South
            WUN, // West Up North
            WUS, // West Up South
            WDN, // West Down North
            WDS  // West Down South
        };

        struct AngleOffset {
            bool x_enable   { false };
            bool y_enable   { false };
            bool z_enable   { false };

            OffsetType x_offset { 0 };
            OffsetType y_offset { 0 };
            OffsetType z_offset { 0 };

            GALIB_NODISCARD bool HasAnyEnable()const {
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

            GALIB_NODISCARD bool HasAnyEnable()const {
                return down || up || north || south || west || east;
            }
        };

        struct Box {
            AngleOffset offset_data[8];
            Flipped flipped_data;
            LittleTilesCoord pos_1 {0, 0, 0};
            LittleTilesCoord pos_2 {0, 0, 0};

            GALIB_NODISCARD bool HasAnyOffsetEnable()const {
                return GALIB_STD any_of(
                    GALIB_STD begin(offset_data),
                    GALIB_STD end(offset_data),
                    [](const AngleOffset& data){return data.HasAnyEnable();}
                );
            }

            GALIB_NODISCARD LittleTilesCoord ApplyAngleOffset(const AngleOffsetID angle_id)const {
                LittleTilesCoord angle_coord = GetVertices(angle_id);

            }

            GALIB_NODISCARD LittleTilesCoord GetVertices(const AngleOffsetID angle_id)const {
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
