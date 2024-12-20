#ifndef _GALIB_MINECRAFT_COORD_H_
#define _GALIB_MINECRAFT_COORD_H_

#include <cinttypes>
#include <string>
#include <tuple>

namespace galib {
    namespace minecraft {
        namespace Coord {
            struct Integer2dCoord {
                int32_t x{ 0 };
                int32_t z{ 0 };

                inline bool operator==(const Integer2dCoord& RHS)const {
                    return x == RHS.x && z == RHS.z;
                }

                inline bool operator<(const Integer2dCoord& RHS) const {
                    if (x != RHS.x) 
                        return x < RHS.x;
                    return z < RHS.z;
                }
                
                inline bool operator>(const Integer2dCoord& RHS) const {
                    if (x != RHS.x)
                        return x > RHS.x;
                    return z > RHS.z;
                }
            };

            struct Integer3dCoord {
                int32_t x{ 0 };
                int32_t y{ 0 };
                int32_t z{ 0 };

                inline bool operator==(const Integer3dCoord& RHS)const {
                    return x == RHS.x && y == RHS.y && z == RHS.z;
                }

                inline bool operator<(const Integer3dCoord& RHS) const {
                    return std::tie(x, y, z) < std::tie(RHS.x, RHS.y, RHS.z);
                }

                inline bool operator>(const Integer3dCoord& RHS) const {
                    return std::tie(x, y, z) > std::tie(RHS.x, RHS.y, RHS.z);
                }
            };

            struct Float3dCoord {
                float x{ 0.0f };
                float y{ 0.0f };
                float z{ 0.0f };

                inline bool operator==(const Float3dCoord& RHS)const {
                    return x == RHS.x && y == RHS.y && z == RHS.z;
                }
            };

            typedef Integer2dCoord  RegionChunkCoord;
            typedef Integer2dCoord  RegionCoord;
            typedef Integer2dCoord  ChunkCoord;
            typedef Integer3dCoord  SingleChunkCoord;
            typedef Integer3dCoord  WorldBlockCoord;
            typedef Float3dCoord    WorldCoord;

            RegionCoord ChunkCoordToRegionCoord(const ChunkCoord& kChunkCoord);
            RegionChunkCoord ChunkCoordToRegionChunkCoord(const ChunkCoord& kChunkCoord);
            WorldBlockCoord WorldBlockCoordToChunkBlockCoord(const WorldBlockCoord& kWorldBlockCoord);

            template<class Type3dCoord>
            std::string ToStr3D(const Type3dCoord& kDescCoord) {
                return std::to_string(kDescCoord.x) + " " + std::to_string(kDescCoord.y) + " " + std::to_string(kDescCoord.z);
            }

            template<class Type2dCoord>
            std::string ToStr2D(const Type2dCoord& kDescCoord) {
                return std::to_string(kDescCoord.x) + " " + std::to_string(kDescCoord.z);
            }
        }
    }
}

#endif // !_GALIB_MINECRAFT_COORD_H_
