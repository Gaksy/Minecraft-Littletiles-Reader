#include <cstdlib>
#include <string>

#include "Minecraft/Anvil.h"
#include "Minecraft/LittleTiles.h"
#include "Minecraft/CgalSupport/CgalLittletilesBuilder.h"

using galib::minecraft::AnvilReader;
using galib::minecraft::ChunkCoordinate;

using galib::minecraft::littletiles::ChunkTileEntities;
using galib::minecraft::cgal_support::ChunkMesh;
using galib::minecraft::cgal_support::margeAndWriteToObj;
using galib::minecraft::cgal_support::margeAndWriteToOff;

#if WIN32
#define REGION_FOLDER "../test_region"
#elif __APPLE__
#define REGION_FOLDER "../test_region"
#endif


int main() {
    printf("Hello, There is LittleTile Reader\n");

    const std::string region_folder = REGION_FOLDER;
    printf("region folder path is %s\n", region_folder.c_str());

    AnvilReader anvil_reader;
    constexpr ChunkCoordinate chunk_coord = {-1, 0};

    anvil_reader.setRegionFolder(region_folder.c_str());
    const AnvilReader::ChunkDataReference chunk_data_reference = anvil_reader.getChunkDataReference(chunk_coord);

    ChunkTileEntities chunk_tile_manager;
    chunk_tile_manager.readChunk(chunk_data_reference);

    ChunkMesh chunk_mesh_management;
    chunk_mesh_management.addTilesFromChukTileEntities(chunk_tile_manager);
#ifdef _WIN32
    margeAndWriteToOff(chunk_mesh_management.getMeshArray(), "D:/Development/MinecraftProject/MinecraftLittletilesReader/python/offs/test");
#else
    margeAndWriteToOff(chunk_mesh_management.getMeshArray(), "../python/offs/test");
#endif
    return EXIT_SUCCESS;
}