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

using galib::minecraft::BlockCoordinate;
using galib::minecraft::ChunkCoordinate;
using galib::minecraft::RegionChunkCoordinate;
using galib::minecraft::RegionCoordinate;

using galib::coord::Coord3DToCoord2D;

using std::floor;
using std::int32_t;

RegionCoordinate(galib::minecraft::ChunkCoordToRegionCoord)(
    const ChunkCoordinate& kOreChunkCoord) {
  return {static_cast<int32_t>(floor(kOreChunkCoord.x / 32.0)),
          static_cast<int32_t>(floor(kOreChunkCoord.z / 32.0))};
}

RegionChunkCoordinate(galib::minecraft::ChunkCoordToRegionChunkCoord)(
    const ChunkCoordinate& kOreChunkCoord) {
  return CoordSwap2D(kOreChunkCoord, 32);
}

ChunkCoordinate(galib::minecraft::BlockCoordToChunkCoord)(
    const BlockCoordinate& kOreBlockCoord) {
  return CoordSwap2D(Coord3DToCoord2D(kOreBlockCoord), 16);
}
