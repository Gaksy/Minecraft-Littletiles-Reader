/*
 * This Source Code Form is subject to the terms of the GNU Lesser General Public License,
 * v. 3.0. If a copy of the LGPL was not distributed with this file, You can obtain one at
 * https://www.gnu.org/licenses/lgpl-3.0.html.
 *
 * Copyright (c) 2024 Gaksy (Fuhongren)
 *
 * Author: Gaksy
 * Date Created: 4/17/2025
 *
 * Additional Terms:
 * For commercial use of this software, you must obtain separate authorization from the original author.
 * Please contact Gaksy at gaksys@outlook.com to request commercial use authorization.
 */
#ifndef GALIB_INCLUDE_MINECRAFT_LITTLETILESCOORD_H
#define GALIB_INCLUDE_MINECRAFT_LITTLETILESCOORD_H

#include <cinttypes>

#include "GalibNamespaceDef.h"
#include "Coord/Coord3D.h"

namespace galib::minecraft::littletiles{
    using GridType = GALIB_STD int32_t;
    using OffsetType = GALIB_STD int16_t;
    using LittleTilesCoord = GALIB coord::Coordinate3D<GALIB_STD int32_t>;
}


#endif //GALIB_INCLUDE_MINECRAFT_LITTLETILESCOORD_H
