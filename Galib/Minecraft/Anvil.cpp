/*
 * This Source Code Form is subject to the terms of the GNU Lesser General Public License,
 * v. 3.0. If a copy of the LGPL was not distributed with this file, You can obtain one at
 * https://www.gnu.org/licenses/lgpl-3.0.html.
 *
 * Copyright (c) 2024 Gaksy (Fuhongren)
 *
 * Author: Gaksy
 * Date Created: 12/24/2024
 *
 * Additional Terms:
 * For commercial use of this software, you must obtain separate authorization from the original author.
 * Please contact Gaksy at gaksys@outlook.com to request commercial use authorization.
 */

#include <memory>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/back_inserter.hpp>

#include "Galib/Minecraft/Anvil.h"
#include "Galib/File/FileOperator.h"
#include "Galib/GalibNamespaceDef.h"
#include "Galib/File/FileState.h"
#include "Galib/File/PathFormat.h"
#include "Galib/Coord/CoordString.h"

using GALIB coord::Coord2DToString;

using GALIB file::FileStat;
using GALIB file::IsFolderAccessible;
using GALIB file::FormatFolderPath;
using GALIB file::IsFileAccessible;
using GALIB file::GetFileStat;
using GALIB file::ReadFileBasic;

using GALIB exception::MinecraftException;
using GALIB exception::MinecraftErrorCode;

using GALIB minecraft::AnvilEditor;
using GALIB minecraft::CacheManagerBase;
using GALIB minecraft::RegionChunkCoordinate;
using GALIB minecraft::RegionCoordinate;
using GALIB minecraft::ChunkCoordToRegionCoord;
using GALIB minecraft::ChunkCoordToRegionChunkCoord;
using GALIB minecraft::ChunkCoordToRegionCoord;
using GALIB minecraft::IsValidCheckForRegionChunkCoord;

using GALIB_STD string;
using GALIB_STD move;
using GALIB_STD to_string;
using GALIB_STD stringstream;
using GALIB_STD istringstream;
//
// ANVIL EDITOR
//

AnvilEditor::AnvilEditor(const char *const kPRegionFolderPath) {
    SetRegionFolder(kPRegionFolderPath);
}

bool AnvilEditor::SetRegionFolder(const char *const kPRegionFolderPath) {
    // Check Folder Path
    if (!IsFolderAccessible(kPRegionFolderPath)) { return false; }

    // Format folder path style
    string temp_region_folder(kPRegionFolderPath);
    if (!FormatFolderPath(&temp_region_folder.back(), temp_region_folder.length())) { return false; }

    // Set folder path
    region_folder_.swap(temp_region_folder);

    return true;
}

const string &AnvilEditor::GetRegionFolder() const {
    return region_folder_;
}

AnvilEditor::ChunkDataReference AnvilEditor::GetChunkData(const ChunkCoordinate &kChunkCoord) {
    // Get region chunk coord from chunk coord
    RegionCoordinate desc_region_coord = ChunkCoordToRegionCoord(kChunkCoord);
    RegionChunkCoordinate desc_region_chunk_coord = ChunkCoordToRegionChunkCoord(kChunkCoord);


    // Check mca cache
    McaManager::iterator desc_mca_manager_it = mca_cache_.find(desc_region_coord);
    if(desc_mca_manager_it == mca_cache_.end()) {
        // Create mca path
        string desc_mca_path = BuildMcaFilePath(region_folder_, desc_region_coord);
        if(!IsFileAccessible(desc_mca_path.c_str())) {
            throw MinecraftException(
                MinecraftErrorCode::mc_file_read,
                (string("The MCA file cannot be read because it does not exist.") + desc_mca_path).c_str()
            );
        }

        // Read mca file
        ByteArray desc_mca_data;
        if(!ReadMcaFile_(desc_mca_path, desc_mca_data)) {
            throw MinecraftException(
                MinecraftErrorCode::mc_file_read,
                (string("The MCA file cannot be read.") + desc_mca_path).c_str()
            );
        }

        // Save mca data
        desc_mca_manager_it = mca_cache_.insert(McaPair(desc_region_coord, move(desc_mca_data))).first;
    }
    ByteArray& desc_mca_manager = desc_mca_manager_it->second;

    // Find Chunk Manager Cache
    ChunkManager::iterator desc_chunk_manager_it = chunk_cache_.find(desc_region_coord);
    if(desc_chunk_manager_it == chunk_cache_.end()) {
        desc_chunk_manager_it = chunk_cache_.insert(ChunkPair(desc_region_coord, SingleChunkManager(32))).first;
    }
    SingleChunkManager& desc_chunk_manager = desc_chunk_manager_it->second;

    // Find Chunk Cache
    ChunkData* p_chunk_cache = desc_chunk_manager.GetCachePointer(desc_region_chunk_coord);

    if (!p_chunk_cache) {
        // Read chunk data
        p_chunk_cache = desc_chunk_manager.GetNewCachePointer(desc_region_chunk_coord);
        ChunkConstIterator &chunk_iterator_data = p_chunk_cache->chunk_const_iterator;

        // Set chunk coord data
        p_chunk_cache->chunk_info.region_coord = desc_region_coord;
        p_chunk_cache->chunk_info.region_chunk_coord = desc_region_chunk_coord;
        p_chunk_cache->chunk_info.chunk_coord = kChunkCoord;

        // Get chunk iterator of mca file
        chunk_iterator_data.chunk_info = p_chunk_cache->chunk_info;

        if (!GetChunkConstIterator_(desc_mca_manager, chunk_iterator_data)) {
            desc_chunk_manager.ClearCache(desc_region_chunk_coord);
            throw MinecraftException(MinecraftErrorCode::mc_decode, "Locate chunk index error in mca file");
        }

        // Decode the desc chunk data of mca file

        // Decompress
        ByteArray compressed_chunk_data;
        if (!DecompressChunkBinaryData_(chunk_iterator_data, compressed_chunk_data)) {
            desc_chunk_manager.ClearCache(desc_region_chunk_coord);
            throw MinecraftException(
                MinecraftErrorCode::mc_decode, "Failed to decompress binary nbt");
        }

        ChunkNbtRoot &chunk_nbt_root = p_chunk_cache->chunk_root;
        if (!DecompressChunkBinaryNbtData_(compressed_chunk_data, chunk_nbt_root)) {
            desc_chunk_manager.ClearCache(desc_region_chunk_coord);
            throw MinecraftException(MinecraftErrorCode::mc_decode, "Failed to decoding binary nbt");
        }
    }

    //ChunkDataConstReference desc_chunk_reference{ p_chunk_cache->chunk_info, p_chunk_cache->chunk_root.get(), &p_chunk_cache->chunk_root.get()->at("Level").get().as<nbt::tag_compound>() };
    //desc_chunk_reference.chunk_info = p_chunk_cache->chunk_info;
    //desc_chunk_reference.p_chunk_root = p_chunk_cache->chunk_root.get();
    //desc_chunk_reference.p_chunk_level = &p_chunk_cache->chunk_root.get()->at("Level").get().as<nbt::tag_compound>();

    return {
        p_chunk_cache->chunk_info,
        p_chunk_cache->chunk_root.get(),
        &p_chunk_cache->chunk_root.get()->at("Level").get().as<nbt::tag_compound>()
    };
}

//
// AnvilEditor::ChunkDataConstReference AnvilEditor::GetChunkConstData(const coord::ChunkCoord& kChunkCoord) const
// {
//     // Get region chunk coord from chunk coord
//     RegionCoord desc_region_coord = ChunkCoordToRegionCoord(kChunkCoord);
//     RegionChunkCoord desc_region_chunk_coord = ChunkCoordToRegionChunkCoord(kChunkCoord);
//
//     // Check mca cache
//     const CachePack* p_desc_mca_cache = mca_cache_.GetCachePointer(desc_region_coord);
//     CachePack cache_pack;
//     if (!p_desc_mca_cache) {
//         // Read mca file and insert to mca_cache
//         // Build mca file path and read file
//         //p_desc_mca_cache = mca_cache_.GetNewCachePointer(desc_region_coord);
//         if (!ReadMcaFile_(BuildMcaFilePath(region_folder_, desc_region_coord), cache_pack.mca_cache)) {
//             throw GalibMinecraftException(GalibMinecraftErrorCode::file_read);
//         }
//         p_desc_mca_cache = &cache_pack;
//     }
//
//     ChunkData* p_chunk_cache = nullptr;
//     // Check chunk cache
//     if (!p_desc_mca_cache->p_chunk_data) {
//         cache_pack.p_chunk_data = std::make_unique<CacheManagerBase<ChunkData> >(32);
//         p_chunk_cache = cache_pack.p_chunk_data->GetCachePointer(desc_region_chunk_coord);
//     }
//     else
//     {
//         p_chunk_cache = p_desc_mca_cache->p_chunk_data->GetCachePointer(desc_region_chunk_coord);
//     }
//
//     if (!p_chunk_cache) {
//         // Read chunk data
//         p_chunk_cache = p_desc_mca_cache->p_chunk_data->GetNewCachePointer(desc_region_chunk_coord);
//         ChunkConstIterator& chunk_iterator_data = p_chunk_cache->chunk_const_iterator;
//
//         // Set chunk coord data
//         p_chunk_cache->chunk_info.region_coord = desc_region_coord;
//         p_chunk_cache->chunk_info.region_chunk_coord = desc_region_chunk_coord;
//         p_chunk_cache->chunk_info.chunk_coord = kChunkCoord;
//
//         // Get chunk iterator of mca file
//         chunk_iterator_data.chunk_info = p_chunk_cache->chunk_info;
//
//         if (!GetChunkConstIterator_(p_desc_mca_cache->mca_cache, chunk_iterator_data)) {
//             p_desc_mca_cache->p_chunk_data->ClearCache(desc_region_chunk_coord);
//             throw GalibMinecraftException(GalibMinecraftErrorCode::mc_decode_nbt, "AnvilEditor",
//                 "Locate chunk index error in mca file");
//         }
//
//         // Decode the desc chunk data of mca file
//
//         // Decompress
//         ByteArray compressed_chunk_data;
//         if (!DecompressChunkBinaryData_(chunk_iterator_data, compressed_chunk_data)) {
//             p_desc_mca_cache->p_chunk_data->ClearCache(desc_region_chunk_coord);
//             throw GalibMinecraftException(GalibMinecraftErrorCode::mc_decode_nbt, "AnvilEditor",
//                 "Failed to decompress binary nbt");
//         }
//
//         ChunkNbtRoot& chunk_nbt_root = p_chunk_cache->chunk_root;
//         if (!DecompressChunkBinaryNbtData_(compressed_chunk_data, chunk_nbt_root)) {
//             p_desc_mca_cache->p_chunk_data->ClearCache(desc_region_chunk_coord);
//             throw GalibMinecraftException(GalibMinecraftErrorCode::mc_decode_nbt, "AnvilEditor",
//                 "Failed to decoding binary nbt");
//         }
//     }
//
//     return { p_chunk_cache->chunk_info, p_chunk_cache->chunk_root.get(), &p_chunk_cache->chunk_root.get()->at("Level").get().as<nbt::tag_compound>()};
// }

string AnvilEditor::BuildMcaFilePath(const string &kRegionFolderPath, const RegionCoordinate &kRegionCoord) {
    return kRegionFolderPath + "/r." + to_string(kRegionCoord.x) + "." + to_string(kRegionCoord.z) + ".mca";
}

bool AnvilEditor::ReadMcaFile_(const string &kMcaFilePath, ByteArray &desc_bytearray) {
    // Get file stat
    if (!IsFileAccessible(kMcaFilePath.c_str())) { return false; }

    FileStat desc_mca_file_stat;
    GetFileStat(kMcaFilePath.c_str(), &desc_mca_file_stat);

    // Read file
    desc_bytearray.resize(desc_mca_file_stat.st_size);
    if (!ReadFileBasic<ByteType>(kMcaFilePath.c_str(), &*desc_bytearray.begin(), desc_mca_file_stat.st_size)) {
        return false;
    }

    return true;
}

bool AnvilEditor::GetChunkConstIterator_(const ByteArray &kMcaData, ChunkConstIterator &desc_chunk_iterator) {
    // Check chunk coord
    if (!IsValidCheckForRegionChunkCoord(desc_chunk_iterator.chunk_info.region_chunk_coord)) { return false; }

    //const RegionCoord& kRegionCoord = desc_chunk_iterator.chunk_info.region_coord;
    const RegionChunkCoordinate kRegionChunkCoord = desc_chunk_iterator.chunk_info.region_chunk_coord;
    //const ChunkCoord kChunkCoord = desc_chunk_iterator.chunk_info.chunk_coord;

    // Calculating chunk offset index
    ByteIndex chunk_offset_index = 4 * ((kRegionChunkCoord.x & 31) + (static_cast<ByteIndex>(kRegionChunkCoord.z & 31)) * 32);

    // Calculating chunk offset
    ByteIndex chunk_offset = 0;
    chunk_offset = chunk_offset | (static_cast<ByteIndex>(kMcaData[chunk_offset_index]) << 16);
    chunk_offset = chunk_offset | (static_cast<ByteIndex>(kMcaData[chunk_offset_index + 1]) << 8);
    chunk_offset = chunk_offset | static_cast<ByteIndex>(kMcaData[chunk_offset_index + 2]);

    // Chunk if the chunk exists
    if (!chunk_offset) {
        throw MinecraftException(
            MinecraftErrorCode::mc_chunk_exists,
            (string("The chunk is exists ") + Coord2DToString(desc_chunk_iterator.chunk_info.chunk_coord)).c_str()
        );
    }

    // Get chunk length
    ByteIndex chunk_length = static_cast<unsigned char>(kMcaData[chunk_offset_index + 3]);

    // Get desc chunk iterator index
    ByteIndex chunk_begin_index = chunk_offset * 4096;
    ByteIndex chunk_end_index = chunk_begin_index + chunk_length * 4096;

    ByteIndex chunk_valid_begin_index = chunk_begin_index + 5;
    ByteIndex chunk_valid_end_index = 0;

    chunk_valid_end_index = chunk_valid_end_index | (static_cast<ByteIndex>(kMcaData[chunk_begin_index]) << 32);
    chunk_valid_end_index = chunk_valid_end_index | (static_cast<ByteIndex>(kMcaData[chunk_begin_index + 1]) << 16);
    chunk_valid_end_index = chunk_valid_end_index | (static_cast<ByteIndex>(kMcaData[chunk_begin_index + 2]) << 8);
    chunk_valid_end_index = chunk_valid_end_index | static_cast<ByteIndex>(kMcaData[chunk_begin_index + 3]);
    ByteIndex chunk_valid_size = chunk_valid_end_index;
    chunk_valid_end_index += chunk_valid_begin_index;

    // Copy chunk iterator
    desc_chunk_iterator.chunk_info.region_chunk_coord = kRegionChunkCoord;
    desc_chunk_iterator.begin = static_cast<ByteArray::difference_type>(chunk_begin_index) + kMcaData.begin();
    desc_chunk_iterator.end = static_cast<ByteArray::difference_type>(chunk_end_index) + kMcaData.begin();
    desc_chunk_iterator.valid_begin = static_cast<ByteArray::difference_type>(chunk_valid_begin_index) + kMcaData.begin();
    desc_chunk_iterator.valid_end = static_cast<ByteArray::difference_type>(chunk_valid_end_index) + kMcaData.begin();
    desc_chunk_iterator.compression_type = kMcaData[4 + chunk_begin_index];

    desc_chunk_iterator.chunk_offset = chunk_offset;
    desc_chunk_iterator.chunk_offset_data_index = chunk_offset_index;
    desc_chunk_iterator.chunk_size = chunk_length * 4096;
    desc_chunk_iterator.chunk_valid_size = chunk_valid_size;

    return true;
}

bool AnvilEditor::DecompressChunkBinaryData_(
    const ChunkConstIterator &kChunkIterator,
    ByteArray &desc_compressed_chunk_data
) {
    if (kChunkIterator.compression_type != 2) {
        return false;
    }

    try {
        // Get uncompressed data
        ::std::string uncompressed_string;
        uncompressed_string.assign(kChunkIterator.valid_begin, kChunkIterator.valid_end);

        // Create decompress stream
        boost::iostreams::filtering_istream stream_uncompressed;
        stream_uncompressed.push(boost::iostreams::zlib_decompressor());

        // Add Uncompressed data
        stringstream str_stream_uncompressed(uncompressed_string);
        stream_uncompressed.push(str_stream_uncompressed);

        // Get compressed data
        boost::iostreams::copy(stream_uncompressed, boost::iostreams::back_inserter(desc_compressed_chunk_data));
    } catch (...) {
        return false;
    }
    return true;
}

bool AnvilEditor::DecompressChunkBinaryNbtData_(const ByteArray &kCompressedChunkData,
                                                                  ChunkNbtRoot &desc_chunk_root) {
    try {
        // Copy kCompressedChunkData to decompressed_buff
        string decompressed_buff;
        for (const ByteType &byte: kCompressedChunkData) {
            decompressed_buff.push_back((char) byte);
        }

        // Put in stream
        istringstream in_str_stream(decompressed_buff);

        // create stream reader
        GALIB_NBT io::stream_reader in_nbt_stream(in_str_stream);
        // decode nbt binary data
        in_nbt_stream.read_compound().second.swap(desc_chunk_root);
    } catch (...) {
        return false;
    }

    return true;
}
