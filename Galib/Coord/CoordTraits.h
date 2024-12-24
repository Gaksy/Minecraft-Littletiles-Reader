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

#ifndef GALIB_COORD_COORDTRAITS_H
#define GALIB_COORD_COORDTRAITS_H

#include <type_traits>
#include "Galib/GalibNamespaceDef.h"

#define GALIB_STATIC_ASSERT_COORDINATE(CoordinateType)                          \
 static_assert(                                                                 \
 GALIB coord::coord_type_if<CoordinateType>::value,                             \
 "Template argument \"" #CoordinateType "\" must be a valid coordinate type. ")


namespace galib::coord {
    // Define a struct for checking whether a template parameter is a coordinate type.
    template<typename CoordinateType>
    struct coord_type_if : GALIB_STD false_type {
    };

    // Traits, used to extract the numeric type and the self type from a Coord type.
    // NumericType is the data type used for coordinate values (e.g., int, float, etc.)
    // SelfType is the template parameter CoordinateType itself (the actual type passed in)
    template<typename CoordinateType>
    struct coord_type_traits {
        GALIB_STATIC_ASSERT_COORDINATE(CoordinateType);

        using NumericType = typename CoordinateType::NumericType;
        using SelfType = CoordinateType;
    };
}

#endif //GALIB_COORD_COORDTRAITS_H
