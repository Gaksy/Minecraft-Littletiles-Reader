#include <cstdlib>
#include <cstdio>
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

namespace {
    // 读取一行 y/n 回答；直接回车（空行）使用推荐默认值。
    // 注意：调用前必须先清掉上一个 scanf 残留的换行符，否则会立刻读到空行。
    bool askYesNo(const char *const kPQuestion, const bool kDefaultValue) {
        printf("%s (y/n) [%c]: ", kPQuestion, kDefaultValue ? 'y' : 'n');
        fflush(stdout);

        char answer[64];
        if (!fgets(answer, sizeof(answer), stdin)) { return kDefaultValue; }

        for (const char *p = answer; *p != '\0'; ++p) {
            if (*p == 'y' || *p == 'Y') { return true; }
            if (*p == 'n' || *p == 'N') { return false; }
        }
        return kDefaultValue;
    }
}


int main() {
    printf("Hello, There is LittleTile Reader\n");

    printf("please key in region folder:");
    char region_folder[256];
    scanf("%255s", region_folder);      // 明确上限，避免超长路径写越界
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

    // 丢掉上一个 scanf(" %d") 之后残留的换行符，后面的选项按整行读取
    {
        int remaining = 0;
        while ((remaining = getchar()) != '\n' && remaining != EOF) { }
    }

    // 居中：把包围盒中心移到原点（推荐，导入第三方软件后一按 Frame Selected 就能看到）
    const bool is_need_geometry_center = askYesNo("Move the model center to the origin?", true);
    // 单位缩放：会把最长边压成 1，丢失"1 单位 = 1 方块"的真实尺寸，默认不做
    const bool is_need_normalize_scale = askYesNo("Also scale the longest edge to 1 unit (changes the real size)?", false);

    // writeToOff(chunk_mesh_management.getMeshArray(), "../python/offs/test");
    margeAndWriteToObj(
        chunk_mesh_management.getMeshArray(),
        obj_file_path.c_str(),
        is_need_geometry_center,
        is_need_normalize_scale
    );
    return EXIT_SUCCESS;
}
