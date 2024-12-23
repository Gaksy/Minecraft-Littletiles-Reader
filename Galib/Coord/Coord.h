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

#ifndef GALIB_COORD_COORD_H
#define GALIB_COORD_COORD_H

#include "CoordTraits.h"
#include "Coord2D.h"
#include "Coord3D.h"

namespace galib::coord {
 template<typename Coord2DType, typename Coord3DType>
 Coord2DType Coord3DToCoord2D(const Coord3DType& kCoord3D) {
  GALIB_STATIC_ASSERT_COORDINATE_2D(Coord2DType);
  GALIB_STATIC_ASSERT_COORDINATE_3D(Coord3DType);
  return {kCoord3D.x, kCoord3D.z};
 }

 template<typename Coord3DType>
 Coordinate2D<typename coord_type_traits<Coord3DType>::NumericType>
 Coord3DToCoord2D(const Coord3DType& kCoord3D){
  using Coord2DType = Coordinate2D<typename coord_type_traits<Coord3DType>::NumericType>;
  return Coord3DToCoord2D<Coord2DType, Coord3DType>(kCoord3D);
 }
}

#endif //GALIB_COORD_COORD_H
