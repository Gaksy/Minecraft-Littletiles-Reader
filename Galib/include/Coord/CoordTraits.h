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

#ifndef GALIB_COORD_COORDTRAITS_H
#define GALIB_COORD_COORDTRAITS_H

#include <type_traits>

#include "GalibNamespaceDef.h"

#define GALIB_STATIC_ASSERT_COORDINATE(CoordinateType)              \
  static_assert(galib::coord::coord_type_if<CoordinateType>::value, \
                "Template argument \"" #CoordinateType              \
                "\" must be a valid coordinate type.")

#define GALIB_STATIC_ASSERT_NUMERICTYPE(NumericType)    \
  static_assert(std::is_arithmetic<NumericType>::value, \
                "Template argument \"" #NumericType "\" must be arithmetic.")

namespace galib::coord {
// Define a struct for checking whether a template parameter is a coordinate type.
template <typename CoordinateType>
struct coord_type_if : std::false_type {};

// Traits, used to extract the numeric type and the self type from a Coord type.
// NumericType is the data type used for coordinate values (e.g., int, float, etc.)
// SelfType is the template parameter CoordinateType itself (the actual type passed in)
template <typename CoordinateType>
struct coord_type_traits {
  // Verify whether the template parameter CoordinateType qualifies as a coordinate type
  GALIB_STATIC_ASSERT_COORDINATE(CoordinateType);

  // The numeric type used for coordinates
  using NumericType = typename CoordinateType::NumericType;
  using SelfType = CoordinateType;
};
}  // namespace galib::coord

#endif  //GALIB_COORD_COORDTRAITS_H
