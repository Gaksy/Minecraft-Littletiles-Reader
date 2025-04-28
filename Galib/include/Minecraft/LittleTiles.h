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

#include <cinttypes>
#include <vector>
#include <algorithm>

#include "GalibNamespaceDef.h"
#include "Minecraft/LittleTilesCoord.h"
#include "Minecraft/MinecraftCoord.h"
#include "Minecraft/Anvil.h"
#include "Exception/LittleTilesException.h"

namespace galib::minecraft::littletiles{
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

    enum class TileFaceID: GALIB_STD uint8_t {
        EAST = 0,  // East
        WEST = 1,  // West
        SOUTH = 2,  // South
        NORTH = 3,  // North
        UP = 4,  // Up
        DOWN = 5   // Down
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

    struct TileFace {
        LittleTilesCoord pos_1 {0, 0, 0};
        LittleTilesCoord pos_2 {0, 0, 0};
        LittleTilesCoord pos_3 {0, 0, 0};
        LittleTilesCoord pos_4 {0, 0, 0};
    };

    struct TileEntity {
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

        GALIB_NODISCARD AngleOffset getAngleOffset(const AngleOffsetID kAngleID)const {
            return offset_data[static_cast<GALIB_STD uint8_t>(kAngleID)];
        }

        GALIB_NODISCARD LittleTilesCoord getVertices(const AngleOffsetID kAngleId)const {
            switch (kAngleId) {
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

        GALIB_NODISCARD LittleTilesCoord getVertices(const AngleOffsetID kAngleId, const bool kWithOffset)const {
            if (kWithOffset) { return applyAngleOffset(kAngleId); }
            return getVertices(kAngleId);
        }

        GALIB_NODISCARD TileFace getTileFace(const TileFaceID kTileFaceID, const bool kWithOffst = false)const {
            switch (kTileFaceID) {
                case TileFaceID::EAST:
                    return {
                        getVertices(AngleOffsetID::EUN, kWithOffst),
                        getVertices(AngleOffsetID::EUS, kWithOffst),
                        getVertices(AngleOffsetID::EDS, kWithOffst),
                        getVertices(AngleOffsetID::EDN, kWithOffst)
                    };
                case TileFaceID::WEST:
                    return {
                        getVertices(AngleOffsetID::WUN, kWithOffst),
                        getVertices(AngleOffsetID::WUS, kWithOffst),
                        getVertices(AngleOffsetID::WDS, kWithOffst),
                        getVertices(AngleOffsetID::WDN, kWithOffst)
                    };
                case TileFaceID::SOUTH:
                    return {
                        getVertices(AngleOffsetID::EUS, kWithOffst),
                        getVertices(AngleOffsetID::WUS, kWithOffst),
                        getVertices(AngleOffsetID::WDS, kWithOffst),
                        getVertices(AngleOffsetID::EDS, kWithOffst)
                    };
                case TileFaceID::NORTH:
                    return {
                        getVertices(AngleOffsetID::EUN, kWithOffst),
                        getVertices(AngleOffsetID::EUS, kWithOffst),
                        getVertices(AngleOffsetID::WUN, kWithOffst),
                        getVertices(AngleOffsetID::WUS, kWithOffst)
                    };
                case TileFaceID::UP:
                    return {
                        getVertices(AngleOffsetID::EUN, kWithOffst),
                        getVertices(AngleOffsetID::EUS, kWithOffst),
                        getVertices(AngleOffsetID::WUS, kWithOffst),
                        getVertices(AngleOffsetID::WUN, kWithOffst)
                    };
                case TileFaceID::DOWN:
                    return {
                        getVertices(AngleOffsetID::EDN, kWithOffst),
                        getVertices(AngleOffsetID::EDS, kWithOffst),
                        getVertices(AngleOffsetID::WDS, kWithOffst),
                        getVertices(AngleOffsetID::WDN, kWithOffst)
                    };
                default:
                    throw exception::LittleTilesException(exception::LittleTilesErrorCode::lt_unknow_face);
            }
        }
    };

    using BoxTileEnities = GALIB_STD vector<TileEntity>;

    class BlockTileEntities {
    public:
        using const_iterator = GALIB_STD map<GALIB_STD string, BoxTileEnities>::const_iterator;
        // <block_id, array of box>
        using container = GALIB_STD map<GALIB_STD string, BoxTileEnities>;
        using container_pair = GALIB_STD pair<GALIB_STD string, BoxTileEnities>;

    public:
        BlockTileEntities();
        ~BlockTileEntities()=default;

    public:
        void readBlockTileNBT(const GALIB_NBT tag_compound& kBlockTilesNBT);

        GALIB_NODISCARD const GALIB minecraft::BlockCoordinate& getBlockCoordinate()const;
        GALIB_NODISCARD const GridType& getGridType()const;
        GALIB_NODISCARD const GALIB_STD string& getLittleTilesID()const;

        GALIB_NODISCARD const_iterator cbegin()const;
        GALIB_NODISCARD const_iterator cend()const;

    private:
        GALIB_NODISCARD static bool readBoxesTilesNbt_(
            const GALIB_NBT tag_compound& kBoxesTilesNbt,
            BoxTileEnities& desc_box_tile_enities
        );

        GALIB_NODISCARD static bool setAngleOffsetStateData_(
            const GALIB_NBT tag_int_array& offset_nbt,
            AngleOffset* p_offset_data,
            Flipped* p_flipped_data
        );

    private:
        GALIB minecraft::BlockCoordinate block_coordinate_;
        GALIB_STD string little_tiles_id_;
        GridType grid_;
        container box_tile_enities_map_;
    };

    class ChunkTileEntities {
    public:
        using const_iterator = GALIB_STD vector<BlockTileEntities>::const_iterator;
        using container = GALIB_STD vector<BlockTileEntities>;

    public:
        ChunkTileEntities();
        ~ChunkTileEntities()=default;

    public:
        void ReadChunk(const GALIB minecraft::AnvilReader::ChunkDataReference& kChunkDataReference);
        GALIB_NODISCARD const GALIB minecraft::ChunkCoordinate& GetChunkCoordinate()const;

        GALIB_NODISCARD const_iterator cbegin()const;
        GALIB_NODISCARD const_iterator cend()const;

        void Clear();
        GALIB_NODISCARD bool IsEmpty()const;

    private:
        GALIB minecraft::ChunkCoordinate chunk_coordinate_;
        container block_tile_entities_;
    };
}

#endif //GALIB_MINECRAFT_LITTLETILES_H
