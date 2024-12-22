/*
 * This Source Code Form is subject to the terms of the GNU Lesser General Public License,
 * v. 3.0. If a copy of the LGPL was not distributed with this file, You can obtain one at
 * https://www.gnu.org/licenses/lgpl-3.0.html.
 *
 * Copyright (c) 2024 Gaksy (Fuhongren)
 *
 * Author: Gaksy
 * Date Created: 12/22/2024
 *
 * Additional Terms:
 * For commercial use of this software, you must obtain separate authorization from the original author.
 * Please contact Gaksy at gaksys@outlook.com to request commercial use authorization.
 */

#ifndef GALIB_COORD_COORD2D_H
#define GALIB_COORD_COORD2D_H

#include "Galib/Coord/CoordTraits.h"

#define GALIB_STATIC_ASSERT_COORDINATE_2D(CoordinateType)                          \
 static_assert(                                                                    \
 GALIB coord::coord2d_type_if<CoordinateType>::value,                              \
 "Template argument \"" #CoordinateType "\" must be a valid 2D coordinate type. ")

namespace galib::coord {
 // 2D Coordinate (x, z)
 // NumericType represents the data type used for coordinate values (e.g., int, float, etc.)
 // SelfType refers to the actual type passed as the template parameter (i.e., CoordinateType)
 template<typename ArgNumericType>
 struct Coordinate2D {
  using NumericType = ArgNumericType;
  using SelfType    = Coordinate2D;

  NumericType x;
  NumericType z;

  bool operator==(const SelfType &kRhs) const {
   return x == kRhs.x && z == kRhs.z;
  }

  bool operator<(const SelfType &kRhs) const {
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
 template<typename CoordinateType>
 struct coord2d_type_if: GALIB_STD false_type {};

 template<typename ArgNumericType>
 struct coord2d_type_if<Coordinate2D<ArgNumericType>>: GALIB_STD true_type{};

 // Specialize a struct for checking whether a template parameter is a coordinate type.
 template<typename ArgNumericType>
 struct coord_type_if<Coordinate2D<ArgNumericType>>: GALIB_STD true_type{};

}

#endif //GALIB_COORD_COORD2D_H
