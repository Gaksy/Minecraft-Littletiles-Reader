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

using std::string;
using std::to_string;

#if WIN32
#define REGION_FOLDER "../test_region_medim"
#define OUT_OBJ_FILE_NAME "../out_file/marge_obj_from_chunk_"
#elif __APPLE__
#define REGION_FOLDER "../test_region_medim"
#define OUT_OBJ_FILE_NAME "../out_file/marge_obj_from_chunk_"
#endif


int main() {
    printf("Hello, There is LittleTile Reader\n");

    printf("please key in region folder:");
    char region_folder[256];
    scanf("%s", region_folder);
    printf("region folder path is %s\n", region_folder);

    AnvilReader anvil_reader;
    ChunkCoordinate::NumericType chunk_x = 0;
    ChunkCoordinate::NumericType chunk_z = 0;

    printf("chunk x: ");
    scanf("%d", &chunk_x);
    printf("chunk z: ");
    scanf("%d", &chunk_z);

    ChunkCoordinate chunk_coord {chunk_x, chunk_z};

    anvil_reader.setRegionFolder(region_folder);
    const AnvilReader::ChunkDataReference chunk_data_reference = anvil_reader.getChunkDataReference(chunk_coord);

    ChunkTileEntities chunk_tile_manager;
    ChunkTileEntities::size_type tile_count = chunk_tile_manager.readChunk(chunk_data_reference);

    printf("nbt tile processed count: %zu\n", tile_count);

    ChunkMesh chunk_mesh_management;
    const size_t all_tile_count = chunk_tile_manager.tileCount();
    const size_t processed_tile_count = chunk_mesh_management.addTilesFromChukTileEntities(chunk_tile_manager);

    printf("all tile count: %zu, porcessed tile count: %zu\n", all_tile_count, processed_tile_count);

    string obj_file_path = OUT_OBJ_FILE_NAME;
    obj_file_path.append(to_string(chunk_x));
    obj_file_path.append("_");
    obj_file_path.append(to_string(chunk_z));
    obj_file_path.append(".obj");

    char choice = 'n';
    printf("if need gemo center, press y (y/n):");
    // 注意 %c 前的空格：跳过上一个 %d 读取后残留的换行符，
    // 否则这里永远读到 '\n'，用户输入 y 也不会生效。
    scanf(" %c", &choice);
    const bool is_need_gemo_center = (choice == 'y' || choice == 'Y');

    printf("if need normalize to unit size, press y (y/n):");
    scanf(" %c", &choice);
    const bool is_need_normalize_scale = (choice == 'y' || choice == 'Y');

    // writeToOff(chunk_mesh_management.getMeshArray(), "../python/offs/test");
    margeAndWriteToObj(
        chunk_mesh_management.getMeshArray(),
        obj_file_path.c_str(),
        is_need_gemo_center,
        is_need_normalize_scale
    );
    return EXIT_SUCCESS;
}
