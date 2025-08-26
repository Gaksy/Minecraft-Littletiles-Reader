#include <cstdlib>
#include <string>
#include <optional>

#include "Minecraft/Anvil.h"
#include "Minecraft/LittleTiles.h"

using galib::minecraft::AnvilReader;
using galib::minecraft::ChunkCoordinate;

using galib::minecraft::littletiles::ChunkTileEntities;

// std::string getEnvVar(const std::string& varName) {
//     char* value = nullptr;
//     size_t size = 0;
//     // Using _dupenv_s for safer environment variable retrieval
//     if (_dupenv_s(&value, &size, varName.c_str()) == 0 && value != nullptr) {
//         std::string result(value);
//         free(value);  // Don't forget to free the allocated memory
//         return result;
//     }
//     return "";  // Return empty string if environment variable is not found
// }

int main() {
    // printf("Hello, There is LittleTile Reader\n");
    // std::string region_folder = getEnvVar("REGION_FOLDER");
    // // std::string region_folder = "D:\\Minecraft\\PLCII\\.minecraft\\versions\\Lt\\saves\\test\\region";
    //
    // if (region_folder.empty()) {
    //     printf("Env \"REGION_FOLDER\" Not found.\n");
    //     return EXIT_FAILURE;
    // }
    // else {
    //     printf("region folder path is %s\n", region_folder.c_str());
    // }
    //
    // AnvilReader anvil_reader;
    // constexpr ChunkCoordinate chunk_coord = {0, 0};
    //
    // anvil_reader.setRegionFolder(region_folder.c_str());
    // const AnvilReader::ChunkDataReference chunk_data_reference = anvil_reader.getChunkDataReference(chunk_coord);
    //
    // ChunkTileEntities chunk_tile_manager;
    // chunk_tile_manager.readChunk(chunk_data_reference);

    return EXIT_SUCCESS;
}