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

#include "GalibNamespaceDef.h"
#include "Coord/Coord2D.h"
#include "Coord/Coord3D.h"

namespace galib::coord {
    // Converts a 2D coordinate (kDescCoord) to a formatted string. {x, y}
    template<typename Coord2DType>
    GALIB_STD string Coord2DToString(const Coord2DType &kDescCoord) {
        // Verifies that the template parameter Coord2DType represents a valid 2D coordinate type
        GALIB_STATIC_ASSERT_COORDINATE_2D(Coord2DType);
        // return formatted string
        return GALIB_STD string("{") +
               GALIB_STD to_string(kDescCoord.x) + ", " +
               GALIB_STD to_string(kDescCoord.z) + " }";
    }

    // Converts a 3D coordinate (kDescCoord) to a formatted string. {x, y, z}
    template<typename Coord3DType>
    GALIB_STD string Coord3DToString(const Coord3DType &kDescCoord) {
        // Verifies that the template parameter Coord2DType represents a valid 3D coordinate type
        GALIB_STATIC_ASSERT_COORDINATE_3D(Coord3DType);
        // return formatted string
        return GALIB_STD string("{") +
               GALIB_STD to_string(kDescCoord.x) + ", " +
               GALIB_STD to_string(kDescCoord.y) + ", " +
               GALIB_STD to_string(kDescCoord.z) + " }";
    }
}

#endif //GALIB_COORD_COORDSTRING_H
