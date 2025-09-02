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
using galib::minecraft::cgal_support::writeToOff;

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
    constexpr ChunkCoordinate chunk_coord = {0, -1};

    anvil_reader.setRegionFolder(region_folder.c_str());
    const AnvilReader::ChunkDataReference chunk_data_reference = anvil_reader.getChunkDataReference(chunk_coord);

    ChunkTileEntities chunk_tile_manager;
    ChunkTileEntities::size_type tile_count = chunk_tile_manager.readChunk(chunk_data_reference);

    printf("nbt tile processed count: %zu\n", tile_count);

    ChunkMesh chunk_mesh_management;
    const size_t all_tile_count = chunk_tile_manager.tileCount();
    const size_t processed_tile_count = chunk_mesh_management.addTilesFromChukTileEntities(chunk_tile_manager);

    printf("all tile count: %zu, porcessed tile count: %zu\n", all_tile_count, processed_tile_count);
#ifdef _WIN32
    margeAndWriteToObj(chunk_mesh_management.getMeshArray(), "D:/Development/MinecraftProject/MinecraftLittletilesReader/python/test.obj");
    // writeToOff(chunk_mesh_management.getMeshArray(), "D:/Development/MinecraftProject/MinecraftLittletilesReader/python/offs/test");
#else
    // margeAndWriteToOff(chunk_mesh_management.getMeshArray(), "../python/offs/test");

    writeToOff(chunk_mesh_management.getMeshArray(), "../python/offs/test");
#endif
    return EXIT_SUCCESS;
}