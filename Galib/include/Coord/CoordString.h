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
 * Date Created: 12/23/2024
 */

#ifndef GALIB_COORD_COORDSTRING_H
#define GALIB_COORD_COORDSTRING_H

#include <string>

#include "Coord/Coord2D.h"
#include "Coord/Coord3D.h"
#include "GalibNamespaceDef.h"

namespace galib::coord {
// Converts a 2D coordinate (kDescCoord) to a formatted string. {x, y}
template <typename Coord2DType>
std::string Coord2DToString(const Coord2DType& kDescCoord) {
  // Verifies that the template parameter Coord2DType represents a valid 2D coordinate type
  GALIB_STATIC_ASSERT_COORDINATE_2D(Coord2DType);
  // return formatted string
  return std::string("{") + std::to_string(kDescCoord.x) + ", " +
         std::to_string(kDescCoord.z) + " }";
}

// Converts a 3D coordinate (kDescCoord) to a formatted string. {x, y, z}
template <typename Coord3DType>
std::string Coord3DToString(const Coord3DType& kDescCoord) {
  // Verifies that the template parameter Coord2DType represents a valid 3D coordinate type
  GALIB_STATIC_ASSERT_COORDINATE_3D(Coord3DType);
  // return formatted string
  return std::string("{") + std::to_string(kDescCoord.x) + ", " +
         std::to_string(kDescCoord.y) + ", " + std::to_string(kDescCoord.z) +
         " }";
}
}  // namespace galib::coord

#endif  //GALIB_COORD_COORDSTRING_H
