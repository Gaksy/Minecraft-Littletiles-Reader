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
 *
 */

#ifndef GALIB_MINECRAFT_MINECRAFTCOORD_H
#define GALIB_MINECRAFT_MINECRAFTCOORD_H

#include <cstdint>

#include "GalibNamespaceDef.h"
#include "Coord/Coord2D.h"
#include "Coord/Coord3D.h"

namespace galib::minecraft {
    // Entity coordinates: Used to store positions such as the player’s location.
    // This type is a 32-bit float, specifically the C++ built-in float type.
    using EntityCoordinate = GALIB coord::Coordinate3D<float>;

    // Block coordinates: These coordinates are in the world reference system.
    // This type is a signed 32-bit integer, specifically int32_t from the standard library.
    using BlockCoordinate = GALIB coord::Coordinate3D<GALIB_STD int32_t>;

    // Region coordinates (r.x,y.mca): This type is a signed 32-bit integer, specifically
    // int32_t from the standard library.
    using RegionCoordinate = GALIB coord::Coordinate2D<GALIB_STD int32_t>;

    // Chunk coordinates relative to the Region: This type is a signed 32-bit integer,
    // specifically int32_t from the standard library.
    using RegionChunkCoordinate = GALIB coord::Coordinate2D<GALIB_STD int32_t>;

    // Chunk coordinates: These coordinates are in the world reference system.
    // This type is a signed 32-bit integer, specifically int32_t from the standard library.
    using ChunkCoordinate = GALIB coord::Coordinate2D<GALIB_STD int32_t>;

    // Single Chunk coordinates: These coordinates are based on chunk coordinates but include
    // a height coordinate. The height is divided so that every 16 blocks make up one chunk.
    // This type is a signed 32-bit integer, specifically int32_t from the standard library.
    using SingleChunkCoordinate = GALIB coord::Coordinate2D<GALIB_STD int32_t>;

    // coordinate transformation
    // Please refer to Chapter 1, Section 3 of the document "Minecraft and LittleTiles Export Obj Basic Technology Research."
    template<typename NumericType>
    NumericType CoordSwap(const NumericType &ore_x, const NumericType &base) {
        NumericType desc_x;
        if (ore_x < 0) {
            desc_x = -ore_x - 1;
            desc_x %= base;
            desc_x = (base - 1) - desc_x;
        } else {
            desc_x = ore_x % base;
        }
        return desc_x;
    }

    // coordinate transformation
    // Please refer to Chapter 1, Section 3 of the document "Minecraft and LittleTiles Export Obj Basic Technology Research."
    template<
        typename Coord2dType,
        typename NumericType = typename GALIB coord::coord_type_traits<Coord2dType>::NumericType>
    Coord2dType CoordSwap2D(const Coord2dType &kOreCoord, const NumericType &base) {
        return {CoordSwap<NumericType>(kOreCoord.x, base), CoordSwap<NumericType>(kOreCoord.z, base)};
    };

    inline bool IsValidCheckForRegionChunkCoord(const RegionChunkCoordinate &kRegionChunkCoord) {
        return (kRegionChunkCoord.x < 32 && kRegionChunkCoord.x >= 0) &&
               (kRegionChunkCoord.z < 32 && kRegionChunkCoord.z >= 0);
    }

    RegionCoordinate ChunkCoordToRegionCoord(const ChunkCoordinate &kOreChunkCoord);

    RegionChunkCoordinate ChunkCoordToRegionChunkCoord(const ChunkCoordinate &kOreChunkCoord);

    ChunkCoordinate BlockCoordToChunkCoord(const BlockCoordinate &kOreBlockCoord);
}

#endif //GALIB_MINECRAFT_MINECRAFTCOORD_H
