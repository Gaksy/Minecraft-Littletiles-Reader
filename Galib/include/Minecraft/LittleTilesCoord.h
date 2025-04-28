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

#include "GalibNamespaceDef.h"
#include "Coord/Coord3D.h"

namespace galib::minecraft::littletiles{
    using GridType = GALIB_STD int32_t;
    using OffsetType = GALIB_STD int16_t;
    using LittleTilesCoord = GALIB coord::Coordinate3D<GALIB_STD int32_t>;
}


#endif //GALIB_MINECRAFT_LITTLETILESCOORD_H
