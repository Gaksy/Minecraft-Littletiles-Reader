#include <cstdlib>
#include "Minecraft/Anvil.h"

using galib::minecraft::AnvilReader;
using galib::minecraft::ChunkCoordinate;

int main() {
    printf("Hello, There is LittleTile Reader");

    AnvilReader anvil_reader;
    AnvilReader::ChunkDataReference chunk_data_reference;
    ChunkCoordinate chunk_coord = {-1, 0};

    anvil_reader.setRegionFolder(R"(D:\Minecraft\PLCII\.minecraft\versions\Lt\saves\test\region)");
    chunk_data_reference = anvil_reader.GetChunkDataReference(chunk_coord);



    return EXIT_SUCCESS;
}
