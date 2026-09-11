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
 * Date Created: 12/22/2024
 */

#ifndef GALIB_COORD_COORD3D_H
#define GALIB_COORD_COORD3D_H

#include "Coord/CoordTraits.h"

#define GALIB_STATIC_ASSERT_COORDINATE_3D(CoordinateType)             \
  static_assert(galib::coord::coord3d_type_if<CoordinateType>::value, \
                "Template argument \"" #CoordinateType                \
                "\" must be a valid 3D coordinate type. ")

namespace galib::coord {
// 3D Coordinate (x, y, z)
// NumericType represents the data type used for coordinate values (e.g., int, float, etc.)
// SelfType refers to the actual type passed as the template parameter (i.e., CoordinateType)
template <typename ArgNumericType>
struct Coordinate3D {
  GALIB_STATIC_ASSERT_NUMERICTYPE(ArgNumericType);

  using NumericType = ArgNumericType;
  using SelfType = Coordinate3D;

  NumericType x;
  NumericType y;
  NumericType z;

  bool operator==(const SelfType& kRhs) const {
    return x == kRhs.x && y == kRhs.y && z == kRhs.z;
  }

  bool operator<(const SelfType& kRhs) const {
    if (x != kRhs.x) {
      return x < kRhs.x;
    }
    if (y != kRhs.y) {
      return y < kRhs.y;
    }
    return z < kRhs.z;
  }

  bool operator>(const SelfType& kRhs) const {
    if (x != kRhs.x) {
      return x > kRhs.x;
    }
    if (y != kRhs.y) {
      return y > kRhs.y;
    }
    return z > kRhs.z;
  }
};

// Define a struct for checking whether a template parameter is of the Coordinate3D type, along with its specialization.
template <typename CoordinateType>
struct coord3d_type_if : std::false_type {};

template <typename ArgNumericType>
struct coord3d_type_if<Coordinate3D<ArgNumericType>> : std::true_type {};

// Specialize a struct for checking whether a template parameter is a coordinate type.
template <typename ArgNumericType>
struct coord_type_if<Coordinate3D<ArgNumericType>> : std::true_type {};
}  // namespace galib::coord

#endif  //GALIB_COORD_COORD3D_H
