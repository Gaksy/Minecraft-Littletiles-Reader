#include <stdlib.h>
#include <sstream>

#include "Anvil.h"
#include "../File.h"
#include "../Exception.h"

#include "boost/iostreams/filtering_stream.hpp"
#include "boost/iostreams/filter/zlib.hpp"
#include "boost/iostreams/copy.hpp"
#include "boost/iostreams/device/back_inserter.hpp"

using galib::file::FileInfo;
using galib::file::AccessFolder;
using galib::file::FormatPath;
using galib::file::GetFileInfo;
using galib::file::ReadFile;

using galib::Exception::GalibException;
using galib::Exception::GalibErrorCode;

using galib::minecraft::Coord::RegionCoord;
using galib::minecraft::Coord::RegionChunkCoord;
using galib::minecraft::Coord::ChunkCoord;
using galib::minecraft::Coord::ChunkCoordToRegionCoord;
using galib::minecraft::Coord::ChunkCoordToRegionChunkCoord;

galib::minecraft::AnvilReader::AnvilReader() :
    mca_folder_(),
    mca_map_cache_()
{ }

galib::minecraft::AnvilReader::AnvilReader(const char* const kPFolderPath) :
    mca_folder_(),
    mca_map_cache_()
{
    SetMacFolder(kPFolderPath);
}

bool galib::minecraft::AnvilReader::SetMacFolder(const char* const kPFolderPath)
{
    // Chunk Folder Path
    if (!AccessFolder(kPFolderPath)) {
        GalibException::SetException(GalibErrorCode::galib_failed_open_folder, (std::string("Folder \"") + kPFolderPath + "\" Can't open").c_str(), "galib::minecraft::AnvilReader::SetMacFolder");
        return false;
    }

    // Format Folde Path
    std::string temp = FormatPath(kPFolderPath);
    if (temp.empty()) {
        return false;
    }
    if (*(temp.end() - 1) == '/') {
        temp.erase((temp.end() - 1));
    }

    // Set Folder Path
    mca_folder_ = temp;
    return true;
}

bool galib::minecraft::AnvilReader::GetChunkData(const ChunkCoord& kChunkCoord, ChunkDataConstReference& desc_chunk_data)
{
    if (IsEmpty()) {
        GalibException::SetException(GalibErrorCode::minecraft_object_uninitialized, "The mca folder path is uninitialized.", "galib::minecraft::AnvilReader::GetChunkData");
        return false;
    }

    // Check Chunk cache
    // std::pair<std::map<galib::minecraft::Coord::ChunkCoord, std::pair<std::string, std::unique_ptr<nbt::tag_compound>>>::iterator, bool>
    std::pair<ChunkMapCache::iterator, bool> chunk_map_result = chunk_map_cache_.insert(ChunkMapPair(kChunkCoord, ChunkNbtRoot()));
    ChunkNbtRoot& chunk_nbt_root = chunk_map_result.first->second;
    if (chunk_map_result.second) {
        // If the result of the insertion is true, it indicates that the CHUNK cache data does not exist and needs to be read.
        // (first is the iterator of the inserted element, and second is the result)
        // Otherwise, if the insertion fails, it will return the iterator to the conflicting element.

        // Get Mac Data
        ByteArray* p_mca_byte_array = nullptr;
        if (!GetMcaData_(kChunkCoord, p_mca_byte_array)) {
            chunk_map_cache_.erase(chunk_map_result.first);
            return false;
        }

        // Get Chunk Offset From Mac Data
        ChunkBinaryDataIterator chunk_binary_iterator;
        RegionChunkCoord region_chunk_coord = ChunkCoordToRegionChunkCoord(kChunkCoord);    // Get region chunk coord in region from chunk coord

        if (!GetChunkBinaryDataIterator_(region_chunk_coord, chunk_binary_iterator, *p_mca_byte_array)) {
            chunk_map_cache_.erase(chunk_map_result.first);
            return false;
        }

        // Compress chunk data
        std::string chunk_compressed_nbt_str;
        if (!DecompressChunkbinaryData_(chunk_binary_iterator, chunk_compressed_nbt_str)) {
            chunk_map_cache_.erase(chunk_map_result.first);
            return false;
        }

        // Decode to nbt
        if (!GetChunkNBT_(chunk_nbt_root, chunk_compressed_nbt_str)) {
            chunk_map_cache_.erase(chunk_map_result.first);
            return false;
        }


    }

    try {
        // To ChunkDataConstReference
        desc_chunk_data.p_chunk_root = chunk_nbt_root.second.get();
        desc_chunk_data.p_chunk_level = &desc_chunk_data.p_chunk_root->at("Level").get().as<nbt::tag_compound>();
        // Get chunk coord
        desc_chunk_data.chunk_coord = {
            desc_chunk_data.p_chunk_level->at("xPos").as<nbt::tag_int>().get() ,
            desc_chunk_data.p_chunk_level->at("zPos").as<nbt::tag_int>().get()
        };
        desc_chunk_data.region_coord = ChunkCoordToRegionCoord(kChunkCoord);
        desc_chunk_data.region_chunk_coord = ChunkCoordToRegionChunkCoord(kChunkCoord);
    }
    catch (...) {
        chunk_map_cache_.erase(chunk_map_result.first);
        GalibException::SetException(GalibErrorCode::minecraft_decode_error, "Failed to parse chunk NBT data.", "galib::minecraft::AnvilReader::GetChunkData");
        return false;
    }

    return true;
}



void galib::minecraft::AnvilReader::Clear()
{
    mca_map_cache_.clear();
    chunk_map_cache_.clear();
    mca_folder_.clear();
}

bool galib::minecraft::AnvilReader::IsEmpty()const
{
    return mca_folder_.empty();
}

bool galib::minecraft::AnvilReader::ReadMcaFile_(const char* const kPFilePath, ByteArray& desc_byte_array)
{
    FileInfo file_info;
    if (GetFileInfo(kPFilePath, file_info)) {
        desc_byte_array.resize(file_info.st_size);
        if (ReadFile(kPFilePath, (char*)&*desc_byte_array.begin(), file_info.st_size)) {
            return true;
        }
    }
    return false;
}

bool galib::minecraft::AnvilReader::GetChunkBinaryDataIterator_(const RegionChunkCoord& kRegionChunkCoord, ChunkBinaryDataIterator& desc_chunk_iterator, ByteArray& mca_byte_array)
{
    // Chenk Chunk Coord
    if (kRegionChunkCoord.x >= 32 || kRegionChunkCoord.z >= 32) { return false; }
    try {

        // Calculating chunk offset index
        ByteIndex chunk_offset_index = 4 * ((kRegionChunkCoord.x & 31) + (kRegionChunkCoord.z & 31) * 32);

        // Calculating chunk offset
        ByteIndex chunk_offset = 0;
        chunk_offset = chunk_offset | ((ByteIndex)mca_byte_array[chunk_offset_index] << 16);
        chunk_offset = chunk_offset | ((ByteIndex)mca_byte_array[chunk_offset_index + 1] << 8);
        chunk_offset = chunk_offset | ((ByteIndex)mca_byte_array[chunk_offset_index + 2]);

        // Check if the chunk exists.
        if (!chunk_offset) {
            throw GalibException(GalibErrorCode::minecraft_chunk_exists, "", "galib::minecraft::AnvilReader::GetChunkBinaryDataIterator_");
        }

        // Get chunk length
        ByteIndex chunk_length = (ByteIndex)mca_byte_array[chunk_offset_index + 3];

        // Get desc chunk iterator 
        ByteIndex chunk_begin_index = chunk_offset * 4096;
        ByteIndex chunk_end_index = chunk_begin_index + chunk_length * 4096;

        ByteIndex chunk_valid_begin_index = chunk_begin_index + 5;
        ByteIndex chunk_valid_size = 0;
        ByteIndex chunk_valid_end_index = 0;
        chunk_valid_end_index = chunk_valid_end_index | ((ByteIndex)mca_byte_array[chunk_begin_index] << 32);
        chunk_valid_end_index = chunk_valid_end_index | ((ByteIndex)mca_byte_array[chunk_begin_index + 1] << 16);
        chunk_valid_end_index = chunk_valid_end_index | ((ByteIndex)mca_byte_array[chunk_begin_index + 2] << 8);
        chunk_valid_end_index = chunk_valid_end_index | ((ByteIndex)mca_byte_array[chunk_begin_index + 3]);
        chunk_valid_size = chunk_valid_end_index;
        chunk_valid_end_index += chunk_valid_begin_index;

        // Copy chunk iterator
        desc_chunk_iterator.region_chunk_coord = kRegionChunkCoord;
        desc_chunk_iterator.begin = mca_byte_array.begin() + chunk_begin_index;
        desc_chunk_iterator.end = mca_byte_array.begin() + chunk_end_index;
        desc_chunk_iterator.valid_begin = mca_byte_array.begin() + chunk_valid_begin_index;
        desc_chunk_iterator.valid_end = mca_byte_array.begin() + chunk_valid_end_index;
        desc_chunk_iterator.compression_type = mca_byte_array[chunk_begin_index + 4];

        desc_chunk_iterator.chunk_offset = chunk_offset;
        desc_chunk_iterator.chunk_offset_data_index = chunk_offset_index;
        desc_chunk_iterator.chunk_size = chunk_length * 4096;
        desc_chunk_iterator.chunk_valid_size = chunk_valid_size;
    }
    catch (...) {
        return false;
    }

    return true;
}

bool galib::minecraft::AnvilReader::DecompressChunkbinaryData_(const ChunkBinaryDataIterator& kChunkBinaryDataIterator, std::string& desc_compressed_binary_nbt)
{
    try {
        // Get uncompress_string
        std::string uncompress_string;
        uncompress_string.assign(kChunkBinaryDataIterator.valid_begin, kChunkBinaryDataIterator.valid_end);

        // Create decompress stream
        boost::iostreams::filtering_istream stream_uncompress;
        stream_uncompress.push(boost::iostreams::zlib_decompressor());

        // Add uncompress data
        std::stringstream sstream_uncompress(uncompress_string);
        stream_uncompress.push(sstream_uncompress);

        // Get compressed data
        boost::iostreams::copy(stream_uncompress, boost::iostreams::back_inserter(desc_compressed_binary_nbt));
        //if (!p_desc_compressed_binary_nbt) {
        //    boost::iostreams::copy(stream_uncompress, boost::iostreams::back_inserter(desc_compressed_binary_nbt));
        //}
        //else {
        //    std::vector<char> nbt_data;

        //    boost::iostreams::copy(stream_uncompress, boost::iostreams::back_inserter(nbt_data));
        //    *p_desc_compressed_binary_nbt = std::vector<ByteType>(nbt_data.begin(), nbt_data.end());
        //    desc_comressed_binary_nbt = std::string(nbt_data.begin(), nbt_data.end());
        //}
    }
    catch (...) {
        return false;
    }
    return true;
}

bool galib::minecraft::AnvilReader::GetChunkNBT_(ChunkNbtRoot& chunk_nbt_root, const std::string& decompressed_nbt)
{
    try {
        // Create string stream to read chunk binary nbt
        std::istringstream in_string_stream(decompressed_nbt);
        nbt::io::stream_reader reader(in_string_stream);

        // Get Nbt root
        chunk_nbt_root = reader.read_compound();
    }
    catch (...) {
        GalibException::SetException(GalibErrorCode::minecraft_decode_error, "Failed to parse chunk NBT data.", "galib::minecraft::AnvilReader::GetChunkNBT");
        return false;
    }

    return true;
}




bool galib::minecraft::AnvilReader::GetMcaData_(const ChunkCoord& kChunkCoord, ByteArray*& p_desc_byte_array)
{
    // Get mac data
    RegionCoord region_coord = ChunkCoordToRegionCoord(kChunkCoord);

    // Check mca cache and Get Mca Data
    // std::pair<std::map<galib::minecraft::Coord::RegionCoord, std::vector<unsigned char>>::iterator, bool>
    std::pair<McaMapCache::iterator, bool> mca_arry_result = mca_map_cache_.insert(McaMapPair(region_coord, ByteType()));
    ByteArray& mca_data = mca_arry_result.first->second;
    if (mca_arry_result.second) {
        // Set mac file path
        std::string mca_file_path = mca_folder_ + "/r." + std::to_string(region_coord.x) + "." + std::to_string(region_coord.z) + ".mca";
        // If the result of the insertion is true, it indicates that the MCA cache data does not exist and needs to be read.
        // (first is the iterator of the inserted element, and second is the result)
        // Otherwise, if the insertion fails, it will return the iterator to the conflicting element.
        if (!ReadMcaFile_(mca_file_path.c_str(), mca_data)) {
            mca_map_cache_.erase(mca_arry_result.first);
            return false;
        }
    }
    p_desc_byte_array = &mca_data;
    return true;
}