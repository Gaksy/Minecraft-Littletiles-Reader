/*
 * Copyright (c) 2024 Gaksy (Fuhongren)
 *
 * This work is licensed under the GNU Lesser General Public License v3.0.
 * You may obtain a copy of the license at https://www.gnu.org/licenses/lgpl-3.0.html.
 *
 * This source code form is subject to the terms of the LGPL v3.0 license.
 * If a copy of the LGPL was not distributed with this file, you can obtain one
 * at the above license URL.
 */

/*
 * Author: Gaksy
 * Date Created: 12/24/2024
 */

#include "Minecraft/Anvil.h"

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <memory>

#include "Coord/CoordString.h"
#include "File/FileOperator.h"
#include "File/FileState.h"
#include "File/PathFormat.h"
#include "GalibNamespaceDef.h"

using GALIB coord::Coord2DToString;

using GALIB file::FileStat;
using GALIB file::IsFolderAccessible;
using GALIB file::FormatFolderPath;
using GALIB file::IsFileAccessible;
using GALIB file::GetFileStat;
using GALIB file::ReadFileBasic;

using GALIB exception::MinecraftException;
using GALIB exception::MinecraftErrorCode;

using GALIB minecraft::AnvilReader;
using GALIB minecraft::CacheManagerBase;
using GALIB minecraft::RegionChunkCoordinate;
using GALIB minecraft::RegionCoordinate;
using GALIB minecraft::ChunkCoordToRegionCoord;
using GALIB minecraft::ChunkCoordToRegionChunkCoord;
using GALIB minecraft::ChunkCoordToRegionCoord;
using GALIB minecraft::IsValidCheckForRegionChunkCoord;

using GALIB_STD string;
using GALIB_STD to_string;
using GALIB_STD stringstream;
using GALIB_STD istringstream;

using GALIB_BOOST iostreams::filtering_istream;
using GALIB_BOOST iostreams::zlib_decompressor;
using GALIB_BOOST iostreams::copy;

using GALIB_NBT io::stream_reader;

//
// ANVIL EDITOR
//

AnvilReader::AnvilReader(const char* const kPRegionFolderPath) {
  SetRegionFolder(kPRegionFolderPath);
}

bool AnvilReader::SetRegionFolder(const char* const kPRegionFolderPath) {
  // Check Folder Path
  if (!IsFolderAccessible(kPRegionFolderPath)) {
    return false;
  }

  // Format folder path style
  string temp_region_folder(kPRegionFolderPath);
  if (!FormatFolderPath(&temp_region_folder.back(),
                        temp_region_folder.length())) {
    return false;
  }

  // Set folder path
  region_folder_.swap(temp_region_folder);

  return true;
}

const string& AnvilReader::region_folder() const { return region_folder_; }

AnvilReader::ChunkDataReference AnvilReader::GetChunkDataReference(
    const ChunkCoordinate& kChunkCoord) {
  // Get region chunk coord from chunk coord
  RegionCoordinate desc_region_coord = ChunkCoordToRegionCoord(kChunkCoord);
  const RegionChunkCoordinate desc_region_chunk_coord =
      ChunkCoordToRegionChunkCoord(kChunkCoord);

  // Check mca cache
  auto desc_mca_manager_it = mca_cache_.find(desc_region_coord);

  if (desc_mca_manager_it == mca_cache_.end()) {
    // Create mca path
    string desc_mca_path = BuildMcaFilePath(region_folder_, desc_region_coord);
    if (!IsFileAccessible(desc_mca_path.c_str())) {
      throw MinecraftException(
          MinecraftErrorCode::mc_file_read,
          (string("The MCA file cannot be read because it does not exist.") +
           desc_mca_path)
              .c_str());
    }

    // Read mca file
    ByteArray desc_mca_data;
    if (!ReadMcaFile(desc_mca_path, desc_mca_data)) {
      throw MinecraftException(
          MinecraftErrorCode::mc_file_read,
          (string("The MCA file cannot be read.") + desc_mca_path).c_str());
    }

    // Save mca data
    desc_mca_manager_it =
        mca_cache_.insert(McaPair(desc_region_coord, std::move(desc_mca_data)))
            .first;
  }
  const ByteArray& desc_mca_manager = desc_mca_manager_it->second;

  // Find Chunk Manager Cache
  auto desc_chunk_manager_it = chunk_cache_.find(desc_region_coord);
  if (desc_chunk_manager_it == chunk_cache_.end()) {
    desc_chunk_manager_it =
        chunk_cache_
            .insert(ChunkPair(desc_region_coord, SingleChunkManager(32)))
            .first;
  }
  SingleChunkManager& desc_chunk_manager = desc_chunk_manager_it->second;

  // Find Chunk Cache
  ChunkData* p_chunk_cache =
      desc_chunk_manager.GetCachePointer(desc_region_chunk_coord);

  if (!p_chunk_cache) {
    // Read chunk data
    p_chunk_cache =
        desc_chunk_manager.GetNewCachePointer(desc_region_chunk_coord);
    try {
      ChunkConstIterator& chunk_iterator_data =
          p_chunk_cache->chunk_const_iterator;

      // Set chunk coord data
      p_chunk_cache->chunk_info.region_coord = desc_region_coord;
      p_chunk_cache->chunk_info.region_chunk_coord = desc_region_chunk_coord;
      p_chunk_cache->chunk_info.chunk_coord = kChunkCoord;

      // Get chunk iterator of mca file
      chunk_iterator_data.chunk_info = p_chunk_cache->chunk_info;

      if (!GetChunkConstIterator(desc_mca_manager, chunk_iterator_data)) {
        throw MinecraftException(MinecraftErrorCode::mc_decode,
                                 "Locate chunk index error in mca file");
      }

      // Decode the desc chunk data of mca file

      // Decompress
      ByteArray compressed_chunk_data;
      if (!DecompressChunkBinaryData(chunk_iterator_data,
                                     compressed_chunk_data)) {
        throw MinecraftException(MinecraftErrorCode::mc_decode,
                                 "Failed to decompress binary nbt");
      }

      ChunkNbtRoot& chunk_nbt_root = p_chunk_cache->chunk_root;
      if (!DecompressChunkBinaryNbtData(compressed_chunk_data,
                                        chunk_nbt_root)) {
        throw MinecraftException(MinecraftErrorCode::mc_decode,
                                 "Failed to decoding binary nbt");
      }
    } catch (...) {
      // The half-constructed cache entry must not survive a failed read:
      // otherwise the next request for the same chunk would reuse it and
      // dereference its null chunk_root.
      desc_chunk_manager.ClearCache(desc_region_chunk_coord);
      throw;
    }
  }

  return {p_chunk_cache->chunk_info, p_chunk_cache->chunk_root.get(),
          &p_chunk_cache->chunk_root.get()
               ->at("Level")
               .get()
               .as<nbt::tag_compound>()};
}

void AnvilReader::Clear() {
  mca_cache_.clear();
  chunk_cache_.clear();
  region_folder_.clear();
}

string AnvilReader::BuildMcaFilePath(const string& kRegionFolderPath,
                                     const RegionCoordinate& kRegionCoord) {
  return kRegionFolderPath + "/r." + to_string(kRegionCoord.x) + "." +
         to_string(kRegionCoord.z) + ".mca";
}

bool AnvilReader::ReadMcaFile(const string& kMcaFilePath,
                              ByteArray& desc_bytearray) {
  // Get file stat
  if (!IsFileAccessible(kMcaFilePath.c_str())) {
    return false;
  }

  FileStat desc_mca_file_stat;
  GetFileStat(kMcaFilePath.c_str(), &desc_mca_file_stat);

  // Read file
  desc_bytearray.resize(desc_mca_file_stat.st_size);
  if (!ReadFileBasic<ByteType>(kMcaFilePath.c_str(), &*desc_bytearray.begin(),
                               desc_mca_file_stat.st_size)) {
    return false;
  }

  return true;
}

bool AnvilReader::GetChunkConstIterator(
    const ByteArray& kMcaData, ChunkConstIterator& desc_chunk_iterator) {
  // Check chunk coord
  if (!IsValidCheckForRegionChunkCoord(
          desc_chunk_iterator.chunk_info.region_chunk_coord)) {
    return false;
  }

  //const RegionCoord& kRegionCoord = desc_chunk_iterator.chunk_info.region_coord;
  const RegionChunkCoordinate kRegionChunkCoord =
      desc_chunk_iterator.chunk_info.region_chunk_coord;
  //const ChunkCoord kChunkCoord = desc_chunk_iterator.chunk_info.chunk_coord;

  // Calculating chunk offset index
  const ByteIndex chunk_offset_index =
      4 * (kRegionChunkCoord.x + kRegionChunkCoord.z * 32);

  // Calculating chunk offset
  ByteIndex chunk_offset = 0;
  chunk_offset =
      chunk_offset | static_cast<uint8_t>(kMcaData[chunk_offset_index]) << 16;
  chunk_offset = chunk_offset |
                 static_cast<uint8_t>(kMcaData[chunk_offset_index + 1]) << 8;
  chunk_offset =
      chunk_offset | static_cast<uint8_t>(kMcaData[chunk_offset_index + 2]);

  // Chunk if the chunk exists
  if (!chunk_offset) {
    throw MinecraftException(
        MinecraftErrorCode::mc_chunk_not_exist,
        (string("The chunk does not exist ") +
         Coord2DToString(desc_chunk_iterator.chunk_info.chunk_coord))
            .c_str());
  }

  // Get chunk length
  const ByteIndex chunk_length =
      static_cast<ByteIndex>(kMcaData[chunk_offset_index + 3]);

  // Get desc chunk iterator index
  const ByteIndex chunk_begin_index = chunk_offset * 4096;
  const ByteIndex chunk_end_index = chunk_begin_index + chunk_length * 4096;

  const ByteIndex chunk_valid_begin_index = chunk_begin_index + 5;
  ByteIndex chunk_valid_end_index = 0;

  chunk_valid_end_index =
      chunk_valid_end_index |
      (static_cast<uint8_t>(kMcaData[chunk_begin_index]) << 24);
  chunk_valid_end_index =
      chunk_valid_end_index |
      (static_cast<uint8_t>(kMcaData[chunk_begin_index + 1]) << 16);
  chunk_valid_end_index =
      chunk_valid_end_index |
      (static_cast<uint8_t>(kMcaData[chunk_begin_index + 2]) << 8);
  chunk_valid_end_index = chunk_valid_end_index |
                          static_cast<uint8_t>(kMcaData[chunk_begin_index + 3]);
  ByteIndex chunk_valid_size = chunk_valid_end_index;
  chunk_valid_end_index += chunk_valid_begin_index;

  // Copy chunk iterator
  desc_chunk_iterator.chunk_info.region_chunk_coord = kRegionChunkCoord;
  desc_chunk_iterator.begin =
      static_cast<ByteArray::difference_type>(chunk_begin_index) +
      kMcaData.begin();
  desc_chunk_iterator.end =
      static_cast<ByteArray::difference_type>(chunk_end_index) +
      kMcaData.begin();
  desc_chunk_iterator.valid_begin =
      static_cast<ByteArray::difference_type>(chunk_valid_begin_index) +
      kMcaData.begin();
  desc_chunk_iterator.valid_end =
      static_cast<ByteArray::difference_type>(chunk_valid_end_index) +
      kMcaData.begin();
  desc_chunk_iterator.compression_type = kMcaData[4 + chunk_begin_index];

  desc_chunk_iterator.chunk_offset = chunk_offset;
  desc_chunk_iterator.chunk_offset_data_index = chunk_offset_index;
  desc_chunk_iterator.chunk_size = chunk_length * 4096;
  desc_chunk_iterator.chunk_valid_size = chunk_valid_size;

  return true;
}

bool AnvilReader::DecompressChunkBinaryData(
    const ChunkConstIterator& kChunkIterator,
    ByteArray& desc_compressed_chunk_data) {
  if (kChunkIterator.compression_type != 2) {
    return false;
  }

  try {
    // Get uncompressed data
    string uncompressed_string;
    uncompressed_string.assign(kChunkIterator.valid_begin,
                               kChunkIterator.valid_end);

    // Create decompress stream
    filtering_istream stream_uncompressed;
    stream_uncompressed.push(zlib_decompressor());

    // Add Uncompressed data
    stringstream str_stream_uncompressed(uncompressed_string);
    stream_uncompressed.push(str_stream_uncompressed);

    // Get compressed data
    copy(stream_uncompressed,
         GALIB_BOOST iostreams::back_inserter(desc_compressed_chunk_data));
  } catch (...) {
    return false;
  }
  return true;
}

bool AnvilReader::DecompressChunkBinaryNbtData(
    const ByteArray& kCompressedChunkData, ChunkNbtRoot& desc_chunk_root) {
  try {
    // Copy kCompressedChunkData to decompressed_buff
    string decompressed_buff;
    for (const ByteType& byte : kCompressedChunkData) {
      decompressed_buff.push_back((char)byte);
    }

    // Put in stream
    istringstream in_str_stream(decompressed_buff);

    // create stream reader
    stream_reader in_nbt_stream(in_str_stream);
    // decode nbt binary data
    in_nbt_stream.read_compound().second.swap(desc_chunk_root);
  } catch (...) {
    return false;
  }
  return true;
}
