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

#ifndef GALIB_COORD_COORD2D_H
#define GALIB_COORD_COORD2D_H

#include "Coord/CoordTraits.h"

#define GALIB_STATIC_ASSERT_COORDINATE_2D(CoordinateType)            \
  static_assert(GALIB coord::coord2d_type_if<CoordinateType>::value, \
                "Template argument \"" #CoordinateType               \
                "\" must be a valid 2D coordinate type. ")

namespace galib::coord {
// 2D Coordinate (x, z)
// NumericType represents the data type used for coordinate values (e.g., int, float, etc.)
// SelfType refers to the actual type passed as the template parameter (i.e., CoordinateType)
template <typename ArgNumericType>
struct Coordinate2D {
  GALIB_STATIC_ASSERT_NUMERICTYPE(ArgNumericType);

  using NumericType = ArgNumericType;
  using SelfType = Coordinate2D;

  NumericType x;
  NumericType z;

  bool operator==(const SelfType& kRhs) const GALIB_NOEXCEPT {
    return x == kRhs.x && z == kRhs.z;
  }

  bool operator<(const SelfType& kRhs) const {
    if (x != kRhs.x) {
      return x < kRhs.x;
    }
    return z < kRhs.z;
  }

  bool operator>(const SelfType& kRhs) const {
    if (x != kRhs.x) {
      return x > kRhs.x;
    }
    return z > kRhs.z;
  }
};

// Define a struct for checking whether a template parameter is of the Coordinate2D type, along with its specialization.
template <typename CoordinateType>
struct coord2d_type_if : GALIB_STD false_type {};

template <typename ArgNumericType>
struct coord2d_type_if<Coordinate2D<ArgNumericType>> : GALIB_STD true_type {};

// Specialize a struct for checking whether a template parameter is a coordinate type.
template <typename ArgNumericType>
struct coord_type_if<Coordinate2D<ArgNumericType>> : GALIB_STD true_type {};
}  // namespace galib::coord

#endif  //GALIB_COORD_COORD2D_H
