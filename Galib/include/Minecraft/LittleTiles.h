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
#include "Minecraft/LittleTilesCoord.h"
#include "Minecraft/MinecraftCoord.h"
#include "Minecraft/Anvil.h"

namespace galib::minecraft::littletiles{
    // ChunkTileEntity -> BlockTileEntity -> BoxTileTntity -> TileEntity

    enum class AngleID: GALIB_STD uint8_t {
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
        EAST = 0,   // East
        WEST = 1,   // West
        SOUTH = 2,  // South
        NORTH = 3,  // North
        UP = 4,     // Up
        DOWN = 5    // Down
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

    GALIB_STD uint8_t convertAngleIdToInt(AngleID kAngleId);
    AngleID convertIntToAngleId(GALIB_STD uint8_t kNumId);

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

    // tile 的材质键：方块 id + 可选染色。
    // LittleTiles 会把同一种方块的不同颜色存成不同的 tile 条目，
    // 因此只用 block id 当键会把它们合并/丢弃。
    struct TileMaterial {
        GALIB_STD string block_id;
        GALIB_STD int32_t color { 0 };
        bool has_color { false };

        bool operator<(const TileMaterial &kRhs) const {
            if (block_id != kRhs.block_id) { return block_id < kRhs.block_id; }
            if (has_color != kRhs.has_color) { return has_color < kRhs.has_color; }
            return color < kRhs.color;
        }
    };

    class TileEntity {
    public:
        TileEntity();
        ~TileEntity()=default;
    public:
        GALIB_NODISCARD bool hasAnyOffsetEnable()const;
        GALIB_NODISCARD LittleTilesCoord applyAngleOffset(AngleID kAngleId)const;
        GALIB_NODISCARD AngleOffset getAngleOffset(AngleID kAngleID)const;
        GALIB_NODISCARD LittleTilesCoord getVertices(AngleID kAngleId)const;
        GALIB_NODISCARD LittleTilesCoord getVertices(AngleID kAngleId, bool kWithOffset)const;
        GALIB_NODISCARD LittleTilesCoord getVerticesApplyGrid(AngleID kAngleId, GridType kGridType, bool kWithOffset)const;
        GALIB_NODISCARD TileFace getTileFace(TileFaceID kTileFaceID, bool kWithOffset = false)const;
        GALIB_NODISCARD const Flipped& getFlippedData()const;
        GALIB_NODISCARD bool isOffsetOffBoundary()const;
        GALIB_NODISCARD bool hasColor()const;
        GALIB_NODISCARD GALIB_STD int32_t getColor()const;
        void setPos(const LittleTilesCoord& kPos1, const LittleTilesCoord& kPos2);
        void setFlippedData(const Flipped& kFlippedData);
        void setColor(GALIB_STD int32_t kColor, bool kHasColor);
        void setOffsetData(AngleID kAngleId, const AngleOffset& kAngleOffsetData);
        void setOffsetData(const AngleOffset kOffsetData[8]);

    private:
        AngleOffset offset_data_[8];
        Flipped flipped_data_;
        LittleTilesCoord pos_1_;
        LittleTilesCoord pos_2_;
        GALIB_STD int32_t color_ { 0 };
        bool has_color_ { false };
    };

    using BoxTileEnities = GALIB_STD vector<TileEntity>;

    class BlockTileEntities {
    public:
        using const_iterator = GALIB_STD map<TileMaterial, BoxTileEnities>::const_iterator;
        // <material(block_id + color), array of box>
        using container = GALIB_STD map<TileMaterial, BoxTileEnities>;
        using container_pair = GALIB_STD pair<TileMaterial, BoxTileEnities>;
        using size_type = GALIB_STD map<TileMaterial, BoxTileEnities>::size_type;

    public:
        BlockTileEntities();
        ~BlockTileEntities()=default;

    public:
        size_type readBlockTileNBT(const GALIB_NBT tag_compound& kBlockTilesNBT, size_type* p_boxes_count = nullptr);

        GALIB_NODISCARD const GALIB minecraft::BlockCoordinate& getBlockCoordinate()const;
        GALIB_NODISCARD const GridType& getGridType()const;
        GALIB_NODISCARD const GALIB_STD string& getLittleTilesID()const;

        GALIB_NODISCARD const_iterator cbegin()const;
        GALIB_NODISCARD const_iterator cend()const;

        GALIB_NODISCARD size_type tileCount()const;

    private:
        GALIB_NODISCARD static bool readBoxesTilesNbt_(
            const GALIB_NBT tag_compound& kBoxesTilesNbt,
            BoxTileEnities& desc_box_tile_enities,
            size_type& tile_count
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
        container box_tile_entities_map_;
    };

    class ChunkTileEntities {
    public:
        using const_iterator = GALIB_STD vector<BlockTileEntities>::const_iterator;
        using container = GALIB_STD vector<BlockTileEntities>;
        using size_type = GALIB_STD vector<BlockTileEntities>::size_type;

    public:
        ChunkTileEntities();
        ~ChunkTileEntities()=default;

    public:
        size_type readChunk(const GALIB minecraft::AnvilReader::ChunkDataReference& kChunkDataReference, size_type* p_boxes_count = nullptr);

        // 直接读取 chunk 根 NBT，不经过 Anvil / mca 文件。
        // 这是"上传 NBT 数据"这类场景的最小入口：优先取根下的 "Level" 子标签，
        // 若不存在则把根自身当作 level（1.18+ 的扁平结构）。
        size_type readChunkNbt(const GALIB_NBT tag_compound& kChunkRootNbt, size_type* p_boxes_count = nullptr);

        GALIB_NODISCARD const GALIB minecraft::ChunkCoordinate& getChunkCoordinate()const;

        GALIB_NODISCARD const_iterator cbegin()const;
        GALIB_NODISCARD const_iterator cend()const;

        void clear();
        GALIB_NODISCARD bool isEmpty()const;

        GALIB_NODISCARD size_type tileCount()const;

    private:
        // 解析 level 下的 "TileEntities" 列表并填充 block_tile_entities_。
        // readChunk 与 readChunkNbt 共用此实现，保证两条入口行为一致。
        size_type readTileEntities_(const GALIB_NBT tag_compound& kChunkLevelNbt, size_type* p_boxes_count);

    private:
        GALIB minecraft::ChunkCoordinate chunk_coordinate_;
        container block_tile_entities_;
    };
}

#endif //GALIB_MINECRAFT_LITTLETILES_H
