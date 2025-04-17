#include <cstdlib>
#include "Minecraft/Anvil.h"
#include "Minecraft/LittleTiles.h"

using galib::minecraft::AnvilReader;
using galib::minecraft::ChunkCoordinate;

using galib::minecraft::littletiles::ChunkTileEntities;

int main() {
    printf("Hello, There is LittleTile Reader");

    AnvilReader anvil_reader;
    constexpr ChunkCoordinate chunk_coord = {-1, 0};

    anvil_reader.setRegionFolder(R"(D:\Development\Minecaft\MinecraftLittletilesReader\region)");
    const AnvilReader::ChunkDataReference chunk_data_reference = anvil_reader.GetChunkDataReference(chunk_coord);

    ChunkTileEntities chunk_tile_manager;
    chunk_tile_manager.ReadChunk(chunk_data_reference);

    return EXIT_SUCCESS;
}
