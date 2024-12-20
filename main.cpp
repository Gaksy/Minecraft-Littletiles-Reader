#include <cstdio>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <chrono>
#include <iomanip> // For std::put_time

// GALIB
#include "Galib/Exception.h"
#include "Galib/Minecraft/Anvil.h"
#include "Galib/Minecraft/LittleTiles.h"
#include "Galib/Minecraft/LittleTilesObjBuilder.h"

int main(void) {
    printf("LITTLE TILES OBJ EXPORTER\n"); 
    std::string kRegionFolderPath = "D:/Minecraft/PLCII/.minecraft/versions/Lt/saves/new/region";
    std::string kObjFolderPath = "D:/Development/Project/Minecraft/";
    std::string kObjFileName = "test";

    std::cout << "Region Folder: ";
    std::cin >> kRegionFolderPath;

    

    galib::minecraft::AnvilReader mca_reader;
    if (!mca_reader.SetMacFolder(kRegionFolderPath.c_str())) {
        printf("%s\n", galib::Exception::GalibException::GetLastException().what());
        return EXIT_FAILURE;
    }

    // return EXIT_SUCCESS;

    int32_t x_1, z_1;
    int32_t x_2, z_2;
    float a = 0.5;
    std::cout << "start chunk x: ";
    std::cin >> x_1;
    std::cout << "start chunk z: ";
    std::cin >> z_1;
    std::cout << "end chunk x: ";
    std::cin >> x_2;
    std::cout << "end chunk z: ";
    std::cin >> z_2;

    galib::minecraft::Coord::ChunkCoord start_chunk{ x_1, z_1 };
    galib::minecraft::Coord::ChunkCoord end_chunk{ x_2, z_2 };
    int32_t x_min = std::min(start_chunk.x, end_chunk.x);
    int32_t x_max = std::max(start_chunk.x, end_chunk.x);
    int32_t z_min = std::min(start_chunk.z, end_chunk.z);
    int32_t z_max = std::max(start_chunk.z, end_chunk.z);

    std::vector<galib::minecraft::littletiles::ChunkTiles> chunk_lt_readers;

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int32_t chunk_z = z_min; chunk_z <= z_max; ++chunk_z) {
        for (int32_t chunk_x = x_min; chunk_x <= x_max; ++chunk_x) {

            printf("Reade chunk { %d, %d }:", chunk_x, chunk_z);
            galib::minecraft::ChunkDataConstReference chunk_data;
            if (!mca_reader.GetChunkData({ chunk_x, chunk_z }, chunk_data)) {
                printf("Reading chunk failed.\n");
                printf(galib::Exception::GalibException::GetLastException().what());
                continue;
            }
            printf("Reading chunk successfully.\n Read chunk lt nbt:");
            chunk_lt_readers.push_back(galib::minecraft::littletiles::ChunkTiles());
            galib::minecraft::littletiles::ChunkTiles& chunk_lt_reader = *(chunk_lt_readers.end() - 1);
            if (!chunk_lt_reader.ReadChunkRoot(chunk_data)) {
                printf("Reading chunk lt nbt failed.\n");
                printf(galib::Exception::GalibException::GetLastException().what());
                chunk_lt_readers.erase(chunk_lt_readers.end() - 1);
                continue;
            }
            printf("Reading chunk lt nbt successfully.\n\n");
        }
    }

    std::vector<galib::minecraft::littletiles::ChunkFaceBuilder> chunk_face_builders;
    int number_totle = chunk_lt_readers.size();
    int current_number = 0;
    for (galib::minecraft::littletiles::ChunkTiles& desc_chunk_tiles : chunk_lt_readers) {
        if (desc_chunk_tiles.IsEmpty()) { ++current_number; continue; }

        chunk_face_builders.push_back(galib::minecraft::littletiles::ChunkFaceBuilder());
        galib::minecraft::littletiles::ChunkFaceBuilder& chunk_face_reader = *(chunk_face_builders.end() - 1);
        const galib::minecraft::Coord::ChunkCoord& chunk_coord = desc_chunk_tiles.GetChunkCoord();
        int32_t x_offset = std::abs(start_chunk.x - chunk_coord.x);
        x_offset *= 16;
        int32_t z_offset = std::abs(start_chunk.z - chunk_coord.z);
        z_offset *= 16;
        printf("[%d / %d] Build chunk lt face { %d, %d } offset { %d, %d }:", ++current_number, number_totle, chunk_coord.x, chunk_coord.z, x_offset, z_offset);

        if (!chunk_face_reader.SetChunkTiles(desc_chunk_tiles, { x_offset, 0, z_offset })) {
            printf("Building chunk lt face failed.\n");
            printf(galib::Exception::GalibException::GetLastException().what());
            chunk_face_builders.erase(chunk_face_builders.end() - 1);
            continue;
        }
        printf("Building chunk lt face successfully.\n\n");
    }
    
    printf("Build obj to file \"%s\": ", (kObjFolderPath + "/" + kObjFileName).c_str());
    galib::minecraft::littletiles::ObjFormatBuilder obj_builder;
    number_totle = chunk_face_builders.size();
    current_number = 0;
    for (galib::minecraft::littletiles::ChunkFaceBuilder& chunk_face : chunk_face_builders) {
        if (current_number) {
            printf("\b\b\b\b\b\b\b%03d/%03d", ++current_number, number_totle);
        }
        else {
            printf("%03d/%03d", ++current_number, number_totle);
        }
        if (!obj_builder.AddChunkFace(chunk_face)) {
            printf("Has a error.");
        }
        
    }
    obj_builder.BuildFormatToFile(kObjFolderPath.c_str(), kObjFileName.c_str());
    printf("\b\b\b\b\b\b\bExport successfully!\n");

    // 记录程序结束时间
    auto end_time = std::chrono::high_resolution_clock::now();

    // 计算经过的时间
    std::chrono::duration<double> elapsed = end_time - start_time;
    auto total_seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();

    // 计算小时、分钟、秒
    int hours = total_seconds / 3600;
    int minutes = (total_seconds % 3600) / 60;
    int seconds = total_seconds % 60;

    // 打印结果
    std::cout << "Elapsed time: ";
    std::cout << hours << ":";
    std::cout << minutes << ":";
    std::cout << seconds << std::endl;


    return EXIT_SUCCESS;
}