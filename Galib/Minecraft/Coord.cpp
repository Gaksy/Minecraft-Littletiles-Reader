#include "Coord.h"

using galib::minecraft::Coord::RegionCoord;
using galib::minecraft::Coord::RegionChunkCoord;
using galib::minecraft::Coord::ChunkCoord;
using galib::minecraft::Coord::WorldBlockCoord;

RegionCoord galib::minecraft::Coord::ChunkCoordToRegionCoord(const ChunkCoord& kChunkCoord)
{
    return { (int)floor(kChunkCoord.x / 32.0), (int)floor(kChunkCoord.z / 32.0) };
}

RegionChunkCoord galib::minecraft::Coord::ChunkCoordToRegionChunkCoord(const ChunkCoord& kChunkCoord)
{
    RegionChunkCoord region_chunk_coord;

    if (kChunkCoord.x < 0) {
        region_chunk_coord.x = -kChunkCoord.x - 1;
        region_chunk_coord.x %= 32;
        region_chunk_coord.x = 31 - region_chunk_coord.x;
    }
    else {
        region_chunk_coord.x = kChunkCoord.x % 32;
    }

    if (kChunkCoord.z < 0) {
        region_chunk_coord.z = -kChunkCoord.z - 1;
        region_chunk_coord.z %= 32;
        region_chunk_coord.z = 31 - region_chunk_coord.z;
    }
    else {
        region_chunk_coord.z = kChunkCoord.z % 32;
    }

    return region_chunk_coord;
}

WorldBlockCoord galib::minecraft::Coord::WorldBlockCoordToChunkBlockCoord(const WorldBlockCoord& kWorldBlockCoord)
{
    WorldBlockCoord world_block_coord;
    world_block_coord.y = kWorldBlockCoord.y;

    if (kWorldBlockCoord.x < 0) {
        world_block_coord.x = -kWorldBlockCoord.x - 1;
        world_block_coord.x %= 16;
        world_block_coord.x = 15 - world_block_coord.x;
    }
    else {
        world_block_coord.x = kWorldBlockCoord.x % 16;
    }

    if (kWorldBlockCoord.z < 0) {
        world_block_coord.z = -kWorldBlockCoord.z - 1;
        world_block_coord.z %= 16;
        world_block_coord.z = 15 - world_block_coord.z;
    }
    else {
        world_block_coord.z = kWorldBlockCoord.z % 16;
    }

    return world_block_coord;
}


