#ifndef _GALIB_MINECRAFT_LITTLE_TILES_H_
#define _GALIB_MINECRAFT_LITTLE_TILES_H_

#include <vector>
#include <map>

// NBT
#include "libnbtplusplus/nbt_tags.h"

// GALIB
#include "Coord.h"
#include "Anvil.h"


namespace galib {
    namespace minecraft {
        namespace littletiles {

            typedef int32_t     GridType;
            typedef int16_t     OffsetType;
            typedef int32_t     TileCoordType;

            class BoxesTiles {
            public:
                struct AngleOffsetData {
                    bool x_enable{ false };
                    bool y_enable{ false };
                    bool z_enable{ false };

                    OffsetType x_offset{0};
                    OffsetType y_offset{0};
                    OffsetType z_offset{0};

                    inline bool HasAnyEnable()const {
                        return x_enable || y_enable || z_enable;
                    }
                };
                struct FlippedData {
                    bool down{ false };
                    bool up{ false };
                    bool north{ false };
                    bool south{ false };
                    bool west{ false };
                    bool east{ false };

                    inline bool HasAnyEnable()const {
                        return down || up || north || south || west || east;
                    }
                };
                struct TileBoxData {
                    enum AngleOffsetID {
                        EUN, EUS, EDN, EDS, WUN, WUS, WDN, WDS
                    };

                    AngleOffsetData offset_data[8];
                    FlippedData flipped_data;
                    TileCoordType x_1{ 0 };
                    TileCoordType y_1{ 0 };
                    TileCoordType z_1{ 0 };
                    TileCoordType x_2{ 0 };
                    TileCoordType y_2{ 0 };
                    TileCoordType z_2{ 0 };

                    inline bool HasAnyOffsetEnable()const {
                        return
                            offset_data[EUN].HasAnyEnable() ||
                            offset_data[EUS].HasAnyEnable() ||
                            offset_data[EDN].HasAnyEnable() ||
                            offset_data[EDS].HasAnyEnable() ||
                            offset_data[WUN].HasAnyEnable() ||
                            offset_data[WUS].HasAnyEnable() ||
                            offset_data[WDN].HasAnyEnable() ||
                            offset_data[WDS].HasAnyEnable();
                    }

                    inline void ApplyAngleOffset(galib::minecraft::Coord::Integer3dCoord& target_coord, const BoxesTiles::AngleOffsetData& kAngle)const
                    {
                        if (kAngle.x_enable) {
                            target_coord.x += kAngle.x_offset;
                        }
                        if (kAngle.y_enable) {
                            target_coord.y += kAngle.y_offset;
                        }
                        if (kAngle.z_enable) {
                            target_coord.z += kAngle.z_offset;
                        }
                        return;
                    }

                    inline bool IsOffsetWithinBounds() const {
                        galib::minecraft::Coord::Integer3dCoord varry[8]{
                            { x_1, y_1, z_2 },
                            { x_1, y_1, z_1 },
                            { x_2, y_1, z_1 },
                            { x_2, y_1, z_2 },
                            { x_1, y_2, z_1 },
                            { x_1, y_2, z_2 },
                            { x_2, y_2, z_2 },
                            { x_2, y_2, z_1 }
                        };
                        ApplyAngleOffset(varry[0], offset_data[WDS]);
                        ApplyAngleOffset(varry[1], offset_data[WDN]);
                        ApplyAngleOffset(varry[2], offset_data[EDN]);
                        ApplyAngleOffset(varry[3], offset_data[EDS]);
                        ApplyAngleOffset(varry[4], offset_data[WUN]);
                        ApplyAngleOffset(varry[5], offset_data[WUS]);
                        ApplyAngleOffset(varry[6], offset_data[EUS]);
                        ApplyAngleOffset(varry[7], offset_data[EUN]);

                        for (size_t index = 0; index < 8; ++index) {
                            if (varry[index].x < x_1 || varry[index].x > x_2) {
                                return true;
                            }
                            if (varry[index].y < y_1 || varry[index].y > y_2) {
                                return true;
                            }
                            if (varry[index].z < z_1 || varry[index].z > z_2) {
                                return true;
                            }
                        }

                        return false;
                    }
                };

                typedef std::vector<TileBoxData>::const_iterator    const_iterator;
                typedef std::vector<TileBoxData>                    container;

            public:
                BoxesTiles();
                ~BoxesTiles()=default;

            public:
                bool ReadBoxesTilesNbt(const nbt::tag_compound& kBoxesTilesNbt);

                const_iterator cbegin()const;
                const_iterator cend()const;
                
                void Clear();
                bool IsEmpty()const;

            private:
                void IsEmptyAndThrow(const char* const kPMessage)const;

                bool GetAngleOffsetStateData_(const nbt::tag_int_array& tile_nbt_data, AngleOffsetData* p_desc_offset_data, FlippedData& flipped_data);
                static void GetAngleOffsetData_(AngleOffsetData& desc_angle_state, std::vector<OffsetType>::const_iterator& offset_it, std::vector<OffsetType>::const_iterator cend, uint32_t binary, uint32_t mask_type);
                static FlippedData& GetFlippedData_(uint32_t binary);

            private:
                container tiles_array_;
            };

            class BlockTiles {
            public:
                typedef std::map<std::string, BoxesTiles>::const_iterator const_iterator;
                typedef std::map<std::string, BoxesTiles>                 container;

            public:
                BlockTiles();
                ~BlockTiles()=default;

            public:
                bool ReadBlockTilesNbt(const nbt::tag_compound& kBlockTilesNbt);

                const galib::minecraft::Coord::WorldBlockCoord& GetBlockCoord()const;
                const GridType& GetGrid()const;
                const std::string& GetLittleTilesID()const;

                void Clear();
                bool IsEmpty()const;

                const_iterator cbegin()const;
                const_iterator cend()const;

            private:
                void IsEmptyAndThrow(const char* const kPMessage)const;

            private:
                galib::minecraft::Coord::WorldBlockCoord block_coord_;
                GridType grid_;
                std::string little_tiles_id_;

                container boxes_tiles_map_;
            };

            class ChunkTiles {
            public:
                typedef std::vector<BlockTiles>::const_iterator     const_iterator;
                typedef std::vector<BlockTiles>                     container;

            public:
                ChunkTiles();
                ~ChunkTiles()=default;

            public:
                bool ReadChunkRoot(const galib::minecraft::ChunkDataConstReference& kChunkData);
                const galib::minecraft::Coord::ChunkCoord GetChunkCoord()const;

                void Clear();
                bool IsEmpty()const;

                const_iterator cbegin()const;
                const_iterator cend()const;
            private:
                void IsEmptyAndThrow(const char* const kPMessage)const;

            private:
                galib::minecraft::Coord::ChunkCoord chunk_coord_;
                container block_tiles;
            };
        }
    }
}

#endif // !_GALIB_MINECRAFT_LITTLE_TILES_H_

