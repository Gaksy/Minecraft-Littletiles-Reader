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

#ifndef GALIB_COORD_COORD_H
#define GALIB_COORD_COORD_H

#include "CoordTraits.h"
#include "Coord2D.h"
#include "Coord3D.h"

namespace galib::coord {
    template<typename Coord2DType, typename Coord3DType>
    Coord2DType Coord3DToCoord2D(const Coord3DType &kCoord3D) {
        GALIB_STATIC_ASSERT_COORDINATE_2D(Coord2DType);
        GALIB_STATIC_ASSERT_COORDINATE_3D(Coord3DType);
        return {kCoord3D.x, kCoord3D.z};
    }

    template<typename Coord3DType>
    Coordinate2D<typename coord_type_traits<Coord3DType>::NumericType>
    Coord3DToCoord2D(const Coord3DType &kCoord3D) {
        using Coord2DType = Coordinate2D<typename coord_type_traits<Coord3DType>::NumericType>;
        return Coord3DToCoord2D<Coord2DType, Coord3DType>(kCoord3D);
    }
}

#endif //GALIB_COORD_COORD_H
