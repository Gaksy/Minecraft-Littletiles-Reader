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

#include "Minecraft/MinecraftCoord.h"

#include <cmath>

#include "Coord/Coord.h"

using GALIB minecraft::RegionCoordinate;
using GALIB minecraft::ChunkCoordinate;
using GALIB minecraft::RegionChunkCoordinate;
using GALIB minecraft::BlockCoordinate;

using GALIB coord::Coord3DToCoord2D;

using GALIB_STD floor;
using GALIB_STD int32_t;

RegionCoordinate(GALIB minecraft::ChunkCoordToRegionCoord)(
    const ChunkCoordinate& kOreChunkCoord) {
  return {static_cast<int32_t>(floor(kOreChunkCoord.x / 32.0)),
          static_cast<int32_t>(floor(kOreChunkCoord.z / 32.0))};
}

RegionChunkCoordinate(GALIB minecraft::ChunkCoordToRegionChunkCoord)(
    const ChunkCoordinate& kOreChunkCoord) {
  return CoordSwap2D(kOreChunkCoord, 32);
}

ChunkCoordinate(GALIB minecraft::BlockCoordToChunkCoord)(
    const BlockCoordinate& kOreBlockCoord) {
  return CoordSwap2D(Coord3DToCoord2D(kOreBlockCoord), 16);
}
