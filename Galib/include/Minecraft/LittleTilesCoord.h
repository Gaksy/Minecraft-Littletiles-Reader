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
 * Date Created: 4/17/2025
 */

#ifndef GALIB_MINECRAFT_LITTLETILESCOORD_H
#define GALIB_MINECRAFT_LITTLETILESCOORD_H

#include <cinttypes>

#include "Coord/Coord3D.h"
#include "GalibNamespaceDef.h"

namespace galib::minecraft::littletiles {
using GridType = std::int32_t;
using OffsetType = std::int16_t;
using LittleTilesCoord = galib::coord::Coordinate3D<double>;
}  // namespace galib::minecraft::littletiles

#endif  //GALIB_MINECRAFT_LITTLETILESCOORD_H
