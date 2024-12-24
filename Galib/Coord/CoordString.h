/*
 * This Source Code Form is subject to the terms of the GNU Lesser General Public License,
 * v. 3.0. If a copy of the LGPL was not distributed with this file, You can obtain one at
 * https://www.gnu.org/licenses/lgpl-3.0.html.
 *
 * Copyright (c) 2024 Gaksy (Fuhongren)
 *
 * Author: Gaksy
 * Date Created: 12/23/2024
 *
 * Additional Terms:
 * For commercial use of this software, you must obtain separate authorization from the original author.
 * Please contact Gaksy at gaksys@outlook.com to request commercial use authorization.
 */
#ifndef GALIB_COORD_COORDSTRING_H
#define GALIB_COORD_COORDSTRING_H

#include <string>

#include "Galib/GalibNamespaceDef.h"
#include "Galib/Coord/Coord2D.h"
#include "Galib/Coord/Coord3D.h"

namespace galib::coord {
    template<typename Coord2DType>
    GALIB_STD string Coord2DToString(const Coord2DType &kDescCoord) {
        GALIB_STATIC_ASSERT_COORDINATE_2D(Coord2DType);
        return GALIB_STD string("{") +
               GALIB_STD to_string(kDescCoord.x) + ", " +
               GALIB_STD to_string(kDescCoord.z) + " }";
    }

    template<typename Coord3DType>
    GALIB_STD string Coord3DToString(const Coord3DType &kDescCoord) {
        GALIB_STATIC_ASSERT_COORDINATE_3D(Coord3DType);
        return GALIB_STD string("{") +
               GALIB_STD to_string(kDescCoord.x) + ", " +
               GALIB_STD to_string(kDescCoord.y) + ", " +
               GALIB_STD to_string(kDescCoord.z) + " }";
    }
}

#endif //GALIB_COORD_COORDSTRING_H
