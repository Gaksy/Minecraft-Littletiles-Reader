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

#ifndef GALIB_MINECRAFT_ANVIL_H
#define GALIB_MINECRAFT_ANVIL_H

#include <cstddef>
#include <vector>
#include <string>
#include <memory>
#include <libnbtplusplus/nbt_tags.h>

#include "Galib/GalibNamespaceDef.h"
#include "Galib/Minecraft/MinecraftCoord.h"
#include "Galib/Exception/MinecraftException.h"

namespace galib::minecraft {
    // Used for 2D data caching
    template<typename CacheType, typename ArgNumericType>
    class CacheManagerBase {
    private:
        using ColumnsCache  = GALIB_STD vector<GALIB_STD unique_ptr<CacheType>>;
        using RowsCache     = GALIB_STD vector<GALIB_STD unique_ptr<ColumnsCache>>;
        using Coord2dType   = GALIB coord::Coordinate2D<ArgNumericType>;

    public:
        // kBaseSize represents the number of data to be cached, which is the length and width of the 2D array.
        // Once set, it cannot be changed.
        explicit CacheManagerBase(const GALIB_STD size_t kBaseSize) : kBaseSize_(kBaseSize) {
            if (kBaseSize < 2) {
                throw GALIB exception::MinecraftException(GALIB exception::MinecraftErrorCode::mc_invalid_args);
            }
        }

        CacheManagerBase(CacheManagerBase &&RSH) noexcept
            : kBaseSize_(RSH.kBaseSize_) {
            this->cache_ = GALIB_STD move(RSH.cache_);
        }

        CacheManagerBase &operator=(CacheManagerBase &&RSH) = delete;

        CacheManagerBase &operator=(const CacheManagerBase &RSH) = delete;

        ~CacheManagerBase() = default;

    public:
        // Get the pointer to the cached data, requires a 2D coordinate.
        CacheType *GetCachePointer(const Coord2dType &kInteger2dCoord) {
            if (!IsHadCache(kInteger2dCoord)) {
                return nullptr;
            }

            return cache_[kInteger2dCoord.x].get()->at(kInteger2dCoord.y).get();
        }

        // Get the pointer to the cached data, requires a 2D coordinate.
        const CacheType *GetCachePointer(const Coord2dType &kInteger2dCoord) const {
            if (!IsHadCache(kInteger2dCoord)) {
                return nullptr;
            }

            return cache_[kInteger2dCoord.x].get()->at(kInteger2dCoord.y).get();
        }

        // Get a cached data; if the cache does not exist, create it. Requires a 2D coordinate.
        CacheType *GetNewCachePointer(const Coord2dType &kInteger2dCoord) {
            if (cache_.empty()) {
                cache_.resize(kBaseSize_);
            }

            if (!IsValidCheckForCoord(kBaseSize_, kInteger2dCoord)) {
                throw GALIB exception::MinecraftException(GALIB exception::MinecraftErrorCode::mc_invalid_coord);
            }

            // If a bad allocation occurs, then the std::bad_alloc exception will be thrown by the new operator.
            ColumnsCache *p_desc_columns_cache = cache_[kInteger2dCoord.x].get();
            if (!p_desc_columns_cache) {
                cache_[kInteger2dCoord.x].reset(new ColumnsCache(kBaseSize_));
                p_desc_columns_cache = cache_[kInteger2dCoord.x].get();
            }

            CacheType *p_desc_cache = p_desc_columns_cache->at(kInteger2dCoord.y).get();
            if (!p_desc_cache) {
                p_desc_columns_cache->at(kInteger2dCoord.y).reset(new CacheType);
                p_desc_cache = p_desc_columns_cache->at(kInteger2dCoord.y).get();
            }

            return p_desc_cache;
        }

        // Clear the specified cache.
        void ClearCache(const Coord2dType &kInteger2dCoord) {
            if (IsHadCache(kInteger2dCoord)) {
                (*cache_[kInteger2dCoord.x].get())[kInteger2dCoord.y].reset(nullptr);
            }
        }

        // Clear all cache
        void ClearCache() {
            cache_.clear();
        }

        // Check if the cache is empty
        GALIB_NODISCARD bool IsEmpty() const {
            return cache_.empty();
        }

        // Check if the specified cache exists, requires a 2D coordinate
        bool IsHadCache(const Coord2dType &kInteger2dCoord) const {
            if (cache_.empty()) {
                return false;
            }

            if (!IsValidCheckForCoord(kBaseSize_, kInteger2dCoord)) {
                throw GALIB exception::MinecraftException(GALIB exception::MinecraftErrorCode::mc_invalid_coord);
            }

            return cache_[kInteger2dCoord.x].get() && (*cache_[kInteger2dCoord.x].get())[kInteger2dCoord.y].get();
        }

    private:
        static bool IsValidCheckForCoord(const GALIB_STD size_t kBaseSize, const Coord2dType &kInteger2dCoord) {
            return !(kInteger2dCoord.x >= kBaseSize || kInteger2dCoord.y >= kBaseSize);
        }

    private:
        RowsCache cache_;
        const GALIB_STD size_t kBaseSize_;
    };


    class AnvilEditor {
    public:
        using ByteType = char;
        using ByteArray = GALIB_STD vector<ByteType>;
        using ByteIndex = ByteArray::size_type;
        using ByteIndexDifference = ByteArray::difference_type;

        // The std::unique_ptr has automatic resource managemen
        using ChunkNbtRoot = GALIB_STD unique_ptr<GALIB_NBT tag_compound>;

    public:
        struct ChunkInfo {
            GALIB minecraft::RegionCoordinate      region_coord{};
            // Region Coordinates (corresponds to the r.x.z.mca file in Anvil format)
            GALIB minecraft::RegionChunkCoordinate region_chunk_coord{};
            // Region Chunk Coordinates (relative coordinates of the chunk within the region)
            GALIB minecraft::ChunkCoordinate       chunk_coord{};    // World Chunk Coordinates
        };

    private:
        struct ChunkConstIterator {
            ChunkInfo chunk_info;

            ByteArray::const_iterator valid_begin;      // Valid data start iterator (NBT compressed data)
            ByteArray::const_iterator valid_end;        // Valid data end iterator (NBT compressed data)
            ByteArray::const_iterator begin;            // Anvil sector start iterator
            ByteArray::const_iterator end;              // Anvil sector end iterator

            ByteType  compression_type{static_cast<char>(0xFF)};    // Compression type
            ByteIndex chunk_offset{0};                              // Chunk offset (original value, in units of 4096 bytes)
            ByteIndex chunk_offset_data_index{0};                   // Byte position of the block (in bytes)
            ByteIndex chunk_size{0};                                // Chunk size (in bytes, equivalent to the chunk's occupied sector size)
            ByteIndex chunk_valid_size{0};                          // Valid data size of the chunk (in bytes)
        };

    public:
        // struct ChunkDataConstReference {
        //     ChunkInfo chunk_info{};
        //     const nbt::tag_compound *const p_chunk_root{nullptr}; // Chunk root directory (compound tag)
        //     const nbt::tag_compound *const p_chunk_level{nullptr}; // Chunk level directory (compound tag)
        // };

        struct ChunkDataReference
        {
            ChunkInfo chunk_info{};
            GALIB_NBT tag_compound* p_chunk_root  { nullptr };  // Chunk root directory (compound tag)
            GALIB_NBT tag_compound* p_chunk_level { nullptr };  // Chunk level directory (compound tag)
        };

        struct ChunkData {
            ChunkInfo chunk_info{};
            ChunkConstIterator chunk_const_iterator;
            ChunkNbtRoot chunk_root;                    // Chunk root directory (compound tag)
            GALIB_NBT tag_compound *p_chunk_level;           // Chunk level directory (compound tag)
        };

        //struct CachePack {
        //    _STD unique_ptr<CacheManagerBase<ChunkData, _GLIB minecraft::coord::RegionChunkCoord::NumericType>> p_chunk_data;
        //    ByteArray mca_cache;
        //};

    private:
        using SingleChunkManager = CacheManagerBase<ChunkData, GALIB minecraft::RegionChunkCoordinate::NumericType>;
        using McaManager         = GALIB_STD map<GALIB minecraft::RegionCoordinate, ByteArray>;
        using ChunkManager       = GALIB_STD map<GALIB minecraft::RegionCoordinate, SingleChunkManager>;

        using McaPair            = GALIB_STD pair<GALIB minecraft::RegionCoordinate, ByteArray>;
        using ChunkPair          = GALIB_STD pair<GALIB minecraft::RegionCoordinate, SingleChunkManager>;

    public:
        AnvilEditor() = default;
        explicit AnvilEditor(const char *kPRegionFolderPath);
        ~AnvilEditor() = default;

    public:
        // Set the directory path for the Region folder in the Minecraft save file.
        bool SetRegionFolder(const char *kPRegionFolderPath);

        // Get the current directory address of the Region folder.
        GALIB_NODISCARD const GALIB_STD string &GetRegionFolder() const;

        // Get the chunk data, requires passing the chunk coordinates. This function will throw an exception.
        ChunkDataReference GetChunkData(const GALIB minecraft::ChunkCoordinate &kChunkCoord);
        //ChunkDataConstReference GetChunkConstData(const coord::ChunkCoord& kChunkCoord)const;

    private:
        // Build the mca file directory.
        static GALIB_STD string BuildMcaFilePath(
            const GALIB_STD string &kRegionFolderPath,
            const GALIB minecraft::RegionCoordinate &kRegionCoord
        );

        // Read the mca file.
        static bool ReadMcaFile_(
            const GALIB_STD string &kMcaFilePath,
            ByteArray &desc_bytearray
        );

        // Get the chunk index data.
        static bool GetChunkConstIterator_(
            const ByteArray &kMcaData,
            ChunkConstIterator &desc_chunk_iterator
        );

        // Unzip the chunk binary raw data.
        static bool DecompressChunkBinaryData_(
            const ChunkConstIterator &kChunkIterator,
            ByteArray &desc_compressed_chunk_data
        );

        // Unzip the chunk NBT data.
        static bool DecompressChunkBinaryNbtData_(
            const ByteArray &kCompressedChunkData,
            ChunkNbtRoot &desc_chunk_root
        );

        //static void DecompressZlibData_(const ByteArray& compressed_data, )

    private:
        // chunk data cache
        //CacheManagerBase<CachePack, _GLIB minecraft::coord::RegionChunkCoord::NumericType> mca_cache_;
        McaManager mca_cache_;
        ChunkManager chunk_cache_;

        // region folder path
        GALIB_STD string region_folder_;
    };
}

#endif //GALIB_MINECRAFT_ANVIL_H
