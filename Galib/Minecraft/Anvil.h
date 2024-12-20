#ifndef _GALIB_MINECRAFT_ANVIL_H_
#define _GALIB_MINECRAFT_ANVIL_H_

#include "Coord.h"
#include "libnbtplusplus/nbt_tags.h"

namespace galib {
    namespace minecraft {

        struct ChunkDataConstReference {
            galib::minecraft::Coord::RegionCoord        region_coord;           // Region Coordinates (corresponds to the r.x.z.mca file in MCA format)
            galib::minecraft::Coord::RegionChunkCoord   region_chunk_coord;     // Region Chunk Coordinates (relative coordinates of the chunk within the region)
            galib::minecraft::Coord::ChunkCoord         chunk_coord;            //  World Chunk Coordinates

            const nbt::tag_compound* p_chunk_root{ nullptr };                   // Chunk root directory (compound tag)
            const nbt::tag_compound* p_chunk_level{ nullptr };                  // Chunk level directory (compound tag)
        };

        class AnvilReader {
        private:
            typedef std::pair<std::string, std::unique_ptr<nbt::tag_compound>>                      ChunkNbtRoot;
            typedef unsigned char                                                                   ByteType;
            typedef std::vector<unsigned char>                                                      ByteArray;
            typedef std::vector<unsigned char>::size_type                                           ByteIndex;
            typedef std::vector<unsigned char>::difference_type                                     ByteIndexDifference;

            typedef std::map<galib::minecraft::Coord::RegionCoord, std::vector<unsigned char>>      McaMapCache;
            typedef std::pair<galib::minecraft::Coord::RegionCoord, std::vector<unsigned char>>     McaMapPair;

            typedef std::map<galib::minecraft::Coord::ChunkCoord, std::pair<std::string, std::unique_ptr<nbt::tag_compound>>> ChunkMapCache;
            typedef std::pair<galib::minecraft::Coord::ChunkCoord, std::pair<std::string, std::unique_ptr<nbt::tag_compound>>> ChunkMapPair;

            struct ChunkBinaryDataIterator {
                galib::minecraft::Coord::RegionChunkCoord region_chunk_coord;   // Coordinates of the chunk in the MCA file

                ByteArray::iterator valid_begin;                                // Valid data start iterator (NBT compressed data)
                ByteArray::iterator valid_end;                                  // Valid data end iterator (NBT compressed data)
                ByteArray::iterator begin;                                      // Anvil sector start iterator
                ByteArray::iterator end;                                        // Anvil sector end iterator

                ByteType compression_type{ 0xFF };                              // Compression type
                ByteIndex chunk_offset{ 0 };                                    // Chunk offset (original value, in units of 4096 bytes)
                ByteIndex chunk_offset_data_index{ 0 };                         // Byte position of the block (in bytes)
                ByteIndex chunk_size{ 0 };                                      // Chunk size (in bytes, equivalent to the chunk's occupied sector size)
                ByteIndex chunk_valid_size{ 0 };                                // Valid data size of the chunk (in bytes)

            };

            struct ChunkCompressedData {
                ChunkBinaryDataIterator chunk_binary_data_iterator;             // Chunk binary data iterator
                std::string compressed_binary_nbt_data_string;                  // Decompressed binary NBT data of the valid Chunk data
                ChunkNbtRoot chunk_nbt;                                         // Structured NBT data of the chunk
            };

        public: // Constructor or Destructor
            AnvilReader();
            AnvilReader(const char* const kPFolderPath);
            ~AnvilReader() = default;

        public: // Operator
            AnvilReader& operator=(const AnvilReader& RHS) = default;

        public: // Public Member Functions
            bool SetMacFolder(const char* const kPFolderPath);
            bool GetChunkData(const galib::minecraft::Coord::ChunkCoord& kChunkCoord, ChunkDataConstReference& desc_chunk_coord);

            void Clear();
            bool IsEmpty()const;

        private: // Private Member Functions
            bool GetMcaData_(const galib::minecraft::Coord::ChunkCoord& kChunkCoord, ByteArray*& p_desc_byte_array);
            static bool ReadMcaFile_(const char* const kPFilePath, ByteArray& desc_byte_array);
            static bool GetChunkBinaryDataIterator_(const galib::minecraft::Coord::RegionChunkCoord& kRegionChunkCoord, ChunkBinaryDataIterator& desc_chunk_iterator, ByteArray& mca_byte_array);
            static bool DecompressChunkbinaryData_(const ChunkBinaryDataIterator& kChunkBinaryDataIterator, std::string& desc_compressed_binary_nbt);
            static bool GetChunkNBT_(ChunkNbtRoot& chunk_nbt_root, const std::string& decompressed_nbt);

        private: // Private Member Variables or Constants
            McaMapCache mca_map_cache_;
            ChunkMapCache chunk_map_cache_;

            std::string mca_folder_;
        };
    }
}

#endif // !_GALIB_MINECRAFT_ANVIL_H_
