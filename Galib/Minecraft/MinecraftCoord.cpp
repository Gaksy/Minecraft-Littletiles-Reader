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

#include <cmath>

#include "Galib/Minecraft/MinecraftCoord.h"
#include "Galib/Coord/Coord.h"

using GALIB minecraft::RegionCoordinate;
using GALIB minecraft::ChunkCoordinate;
using GALIB minecraft::RegionChunkCoordinate;
using GALIB minecraft::BlockCoordinate;

using GALIB coord::Coord3DToCoord2D;

RegionCoordinate (GALIB minecraft::ChunkCoordToRegionCoord)(const ChunkCoordinate &kOreChunkCoord) {
 return {
  static_cast<int>(::floor(kOreChunkCoord.x / 32.0)),
  static_cast<int>(::floor(kOreChunkCoord.z / 32.0))
 };
}

RegionChunkCoordinate (GALIB minecraft::ChunkCoordToRegionChunkCoord)(const ChunkCoordinate &kOreChunkCoord) {
 return CoordSwap2D(kOreChunkCoord, 32);
}

ChunkCoordinate (GALIB minecraft::BlockCoordToChunkCoord)(const BlockCoordinate &kOreBlockCoord) {
 return CoordSwap2D(Coord3DToCoord2D(kOreBlockCoord), 16);
}