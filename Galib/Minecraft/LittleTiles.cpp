#include "LittleTiles.h"
#include "../Exception.h"

using galib::Exception::GalibErrorCode;
using galib::Exception::GalibException;

galib::minecraft::littletiles::BoxesTiles::BoxesTiles() :
    tiles_array_()
{ }

bool galib::minecraft::littletiles::BoxesTiles::ReadBoxesTilesNbt(const nbt::tag_compound& kBoxesTilesNbt)
{
    if (!kBoxesTilesNbt.size()) {
        GalibException::SetException(GalibErrorCode::minecraft_decode_error, "Unable to parse this NBT data as Little Tiles data. Because the data is empty", "galib::minecraft::littletiles::BoxesTiles::ReadBoxesTilesNbt");
        return false;
    }

    try {
        nbt::tag_list boxes;

        // Get struct
        if (kBoxesTilesNbt.has_key("boxes")) {
            boxes = kBoxesTilesNbt.at("boxes").as<nbt::tag_list>();
        }
        else if (kBoxesTilesNbt.has_key("box")) {
            boxes.push_back(nbt::value_initializer(kBoxesTilesNbt.at("box").as<nbt::tag_int_array>().clone()));
        }
        // Decode Error
        else {
            GalibException::SetException(GalibErrorCode::minecraft_decode_error, "The kTileNbt don't have boxes tag or box tag.", "galib::minecraft::BoxesLtTiles");
            return false;
        }

        // Get tiles data
        container done_tiles_aray;

        for (nbt::tag_list::const_iterator it = boxes.cbegin(); it != boxes.cend(); ++it) {
            try {
                TileBoxData temp;
                const nbt::tag_int_array& int_array = it->as<nbt::tag_int_array>();
                if (int_array.size() < 6) { continue; }
                if (int_array.size() > 6 && !GetAngleOffsetStateData_(int_array, temp.offset_data, temp.flipped_data)) { continue; }

                temp.x_1 = int_array[0];
                temp.y_1 = int_array[1];
                temp.z_1 = int_array[2];
                temp.x_2 = int_array[3];
                temp.y_2 = int_array[4];
                temp.z_2 = int_array[5];
                done_tiles_aray.push_back(temp);
            }
            catch (...) {
                continue;
            }
        }

        // Chenk data
        if (done_tiles_aray.empty()) {
            return false;
        }

        // Save data
        tiles_array_.swap(done_tiles_aray);
    }
    catch (...) {
        GalibException::SetException(GalibErrorCode::minecraft_decode_error, "Unable to parse this NBT data as Little Tile data. Please check the original data", "galib::minecraft::littletiles::BoxesTiles::ReadBoxesTilesNbt");
        return false;
    }

    return true;
}

galib::minecraft::littletiles::BoxesTiles::const_iterator galib::minecraft::littletiles::BoxesTiles::cbegin() const
{
    IsEmptyAndThrow("Get cbegin error");
    return tiles_array_.cbegin();
}

galib::minecraft::littletiles::BoxesTiles::const_iterator galib::minecraft::littletiles::BoxesTiles::cend() const
{
    IsEmptyAndThrow("Get cend error");
    return tiles_array_.cend();
}

void galib::minecraft::littletiles::BoxesTiles::Clear()
{
    tiles_array_.clear();
}

bool galib::minecraft::littletiles::BoxesTiles::IsEmpty() const
{
    return tiles_array_.empty();
}

void galib::minecraft::littletiles::BoxesTiles::IsEmptyAndThrow(const char* const kPMessage)const
{
    if (IsEmpty()) {
        throw GalibException(GalibErrorCode::minecraft_object_uninitialized, kPMessage, "galib::minecraft::littletiles::BoxesTiles");
    }
}

bool galib::minecraft::littletiles::BoxesTiles::GetAngleOffsetStateData_(const nbt::tag_int_array& tile_nbt_data, AngleOffsetData* p_desc_offset_data, FlippedData& flipped_data)
{
    // Check nbt size, if < 7, the angle change data is null
    if (tile_nbt_data.size() < 7) {
        return false;
    }
    // Get angle change data state iterator and create change data buffer
    nbt::tag_int_array::const_iterator it = tile_nbt_data.cbegin() + 6;     // Angle change data state
    uint32_t state_binary = *it;
    std::vector<OffsetType> angle_offset_arry;                                 // Angle change data

    // Get Angle offset data array
    if (it + 1 != tile_nbt_data.cend()) {
        for (nbt::tag_int_array::const_iterator offset_it = it + 1; offset_it != tile_nbt_data.cend(); offset_it++) {
            OffsetType temp_2 = (OffsetType)(*offset_it & 0x0000FFFF);                // Get 16bit
            OffsetType temp_1 = (OffsetType)((*offset_it & 0xFFFF0000) >> 16);        // Get 16bit
            angle_offset_arry.push_back(temp_1);
            angle_offset_arry.push_back(temp_2);
        }
    }

    // Get Angle offset state and set offset value
    std::vector<int16_t>::const_iterator offset_it = angle_offset_arry.cbegin();
    for (size_t angle_id = 0; angle_id < 8; ++angle_id) {
        GetAngleOffsetData_(p_desc_offset_data[angle_id], offset_it, angle_offset_arry.cend(), state_binary, angle_id);
    }

    // Get Flipped
    flipped_data = GetFlippedData_(state_binary);

    return true;
}

void galib::minecraft::littletiles::BoxesTiles::GetAngleOffsetData_(AngleOffsetData& desc_angle_state, std::vector<OffsetType>::const_iterator& offset_it, std::vector<OffsetType>::const_iterator cend, uint32_t binary, uint32_t mask_type)
{
    desc_angle_state.x_enable = binary & (0x1 << (mask_type * 3));
    desc_angle_state.y_enable = binary & (0x2 << (mask_type * 3));
    desc_angle_state.z_enable = binary & (0x4 << (mask_type * 3));

    if (desc_angle_state.HasAnyEnable()) {
        if (desc_angle_state.x_enable && offset_it != cend) {
            desc_angle_state.x_offset = *(offset_it++);
        }
        if (desc_angle_state.y_enable && offset_it != cend) {
            desc_angle_state.y_offset = *(offset_it++);
        }
        if (desc_angle_state.z_enable && offset_it != cend) {
            desc_angle_state.z_offset = *(offset_it++);
        }
    }

    return;
}

galib::minecraft::littletiles::BoxesTiles::FlippedData& galib::minecraft::littletiles::BoxesTiles::GetFlippedData_(uint32_t binary)
{
    FlippedData temp;

    temp.down = binary & (0x1 << (8 * 3));
    temp.up = binary & (0x2 << (8 * 3));
    temp.north = binary & (0x4 << (8 * 3));
    temp.south = binary & (0x1 << (9 * 3));
    temp.west = binary & (0x2 << (9 * 3));
    temp.east = binary & (0x4 << (9 * 3));

    return temp;
}

galib::minecraft::littletiles::BlockTiles::BlockTiles() :
    block_coord_(),
    grid_(0),
    little_tiles_id_(),
    boxes_tiles_map_()
{ }

bool galib::minecraft::littletiles::BlockTiles::ReadBlockTilesNbt(const nbt::tag_compound& kBlockTilesNbt)
{
    // Check empty
    if (!kBlockTilesNbt.size()) {
        GalibException::SetException(GalibErrorCode::minecraft_decode_error, "Unable to parse this NBT data as Little Tiles data. Because the data is empty", "galib::minecraft::littletiles::BlockTiles::ReadBlockTilesNbt");
        return false;
    }
    try {
        // Get block coord
        galib::minecraft::Coord::WorldBlockCoord block_coord_temp{
            kBlockTilesNbt.at("x").as<nbt::tag_int>().get(),
            kBlockTilesNbt.at("y").as<nbt::tag_int>().get(),
            kBlockTilesNbt.at("z").as<nbt::tag_int>().get()
        };
        GridType grid_temp = 16;    // By default

        if (kBlockTilesNbt.has_key("grid")) {
            grid_temp = kBlockTilesNbt.at("grid").as<nbt::tag_int>().get();
        }

        // Get id
        std::string little_tiles_id_temp = kBlockTilesNbt.at("id").as<nbt::tag_string>().get();

        // Get tiles list
        nbt::tag_list tiles = kBlockTilesNbt.at("content").at("tiles").as<nbt::tag_list>();     // Tiles list
        std::string block_id;                                                                   // The buffer
        container boxes_tiles_map_temp_;

        for (nbt::tag_list::iterator it = tiles.begin(); it != tiles.end(); ++it) {
            nbt::tag_compound& boxes = (it->as<nbt::tag_compound>());           // Get boxes
            block_id = boxes.at("block").as<nbt::tag_string>().get();           // Get block id

            // if have block_id
            if (boxes_tiles_map_temp_.find(block_id) != boxes_tiles_map_temp_.end()) {
                continue;
            }

            // Get dataWE
            BoxesTiles boxes_manager;
            // If Get successful, then insert data
            if (boxes_manager.ReadBoxesTilesNbt(boxes)) {
                boxes_tiles_map_temp_.insert(std::pair<std::string, BoxesTiles>(block_id, boxes_manager));
            }
        }

        block_coord_ = galib::minecraft::Coord::WorldBlockCoordToChunkBlockCoord(block_coord_temp);
        grid_ = grid_temp;
        little_tiles_id_.swap(little_tiles_id_temp);
        boxes_tiles_map_.swap(boxes_tiles_map_temp_);
    }
    catch (...) {
        GalibException::SetException(GalibErrorCode::minecraft_decode_error, "Unable to parse this NBT data as Little Tiles data. Please check the original data", "galib::minecraft::littletiles::BlockTiles::ReadBlockTilesNbt");
        return false;
    }

    return true;
}

const galib::minecraft::Coord::WorldBlockCoord& galib::minecraft::littletiles::BlockTiles::GetBlockCoord() const
{
    IsEmptyAndThrow("Get block coord error");
    return block_coord_;
}

const galib::minecraft::littletiles::GridType& galib::minecraft::littletiles::BlockTiles::GetGrid() const
{
    IsEmptyAndThrow("Get grid error");
    return grid_;
}

const std::string& galib::minecraft::littletiles::BlockTiles::GetLittleTilesID() const
{
    IsEmptyAndThrow("Get little tiles id error");
    return little_tiles_id_;
}

void galib::minecraft::littletiles::BlockTiles::Clear()
{
    block_coord_ = { 0,0,0 };
    grid_ = 0;
    little_tiles_id_.clear();
    boxes_tiles_map_.clear();
}

bool galib::minecraft::littletiles::BlockTiles::IsEmpty() const
{
    return boxes_tiles_map_.empty();
}

galib::minecraft::littletiles::BlockTiles::const_iterator galib::minecraft::littletiles::BlockTiles::cbegin() const
{
    IsEmptyAndThrow("Get cbgein error");
    return boxes_tiles_map_.cbegin();
}

galib::minecraft::littletiles::BlockTiles::const_iterator galib::minecraft::littletiles::BlockTiles::cend() const
{
    IsEmptyAndThrow("Get cend error");
    return boxes_tiles_map_.cend();
}

void galib::minecraft::littletiles::BlockTiles::IsEmptyAndThrow(const char* const kPMessage) const
{
    if (IsEmpty()) {
        throw GalibException(GalibErrorCode::minecraft_object_uninitialized, kPMessage, "galib::minecraft::littletiles::BlockTiles");
    }
}

galib::minecraft::littletiles::ChunkTiles::ChunkTiles() :
    chunk_coord_(),
    block_tiles()
{ }

bool galib::minecraft::littletiles::ChunkTiles::ReadChunkRoot(const galib::minecraft::ChunkDataConstReference& kChunkData)
{
    if (!kChunkData.p_chunk_root) {
        GalibException::SetException(GalibErrorCode::minecraft_object_uninitialized, "The kChunkData is null", "galib::minecraft::littletiles::ChunkTiles::ReadChunkRoot");
        return false;
    }
    try {
        const nbt::tag_list& tiles_entities = kChunkData.p_chunk_level->at("TileEntities").as<nbt::tag_list>();
        galib::minecraft::Coord::ChunkCoord chunk_coord_temp = kChunkData.chunk_coord;
        container block_tiles_tmep;
        for (nbt::tag_list::const_iterator it = tiles_entities.cbegin(); it != tiles_entities.cend(); ++it) {
            BlockTiles tiles;
            if (tiles.ReadBlockTilesNbt(it->as<nbt::tag_compound>())) {
                block_tiles_tmep.push_back(tiles);
            }
        }

        chunk_coord_ = chunk_coord_temp;
        block_tiles.swap(block_tiles_tmep);
    }
    catch (...) {
        GalibException::SetException(GalibErrorCode::minecraft_decode_error, (std::string("Can't decode chunk data to liitle tiles\nThe last error:\n") + GalibException::GetLastException().what()).c_str(), "galib::minecraft::littletiles::ChunkTiles::ReadChunkRoot");
        return false;
    }

    return true;
}

const galib::minecraft::Coord::ChunkCoord galib::minecraft::littletiles::ChunkTiles::GetChunkCoord()const
{
    IsEmptyAndThrow("Get chunk coord error");
    return chunk_coord_;
}

void galib::minecraft::littletiles::ChunkTiles::Clear()
{
    chunk_coord_ = { 0,0 };
    block_tiles.clear();
}

bool galib::minecraft::littletiles::ChunkTiles::IsEmpty() const
{
    return block_tiles.empty();
}

galib::minecraft::littletiles::ChunkTiles::const_iterator galib::minecraft::littletiles::ChunkTiles::cbegin() const
{
    return block_tiles.cbegin();
}

galib::minecraft::littletiles::ChunkTiles::const_iterator galib::minecraft::littletiles::ChunkTiles::cend() const
{
    return block_tiles.cend();
}

void galib::minecraft::littletiles::ChunkTiles::IsEmptyAndThrow(const char* const kPMessage) const
{
    if (IsEmpty()) {
        throw GalibException(GalibErrorCode::minecraft_object_uninitialized, kPMessage, "galib::minecraft::littletiles::ChunkTiles");
    }
}