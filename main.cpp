#include <cstdlib>
#include <string>
#include <optional>

#include "Minecraft/Anvil.h"
#include "Minecraft/LittleTiles.h"

using galib::minecraft::AnvilReader;
using galib::minecraft::ChunkCoordinate;

using galib::minecraft::littletiles::ChunkTileEntities;

std::optional<std::string> get_region_folder() {
    const char* folder = std::getenv("REGION_FOLDER");
    if (folder == nullptr) {
        return std::nullopt;
    }
    return std::string(folder);
}


int main() {
    printf("Hello, There is LittleTile Reader\n");
    auto region_folder = get_region_folder();

    if (!region_folder) {
        printf("Env \"REGION_FOLDER\" Not found.\n");
        return EXIT_FAILURE;
    }
    else {
        printf("region folder path is %s\n", region_folder.value().c_str());
    }

    AnvilReader anvil_reader;
    constexpr ChunkCoordinate chunk_coord = {-1, 0};

    anvil_reader.setRegionFolder(region_folder.value().c_str());
    const AnvilReader::ChunkDataReference chunk_data_reference = anvil_reader.GetChunkDataReference(chunk_coord);

    ChunkTileEntities chunk_tile_manager;
    chunk_tile_manager.ReadChunk(chunk_data_reference);

    return EXIT_SUCCESS;
}
