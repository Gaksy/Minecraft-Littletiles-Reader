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
 * Date Created: 04/28/2025
 */

#ifndef GALIB_MINECRAFT_CGALSUPPORT_CGALTYPEDEF_H
#define GALIB_MINECRAFT_CGALSUPPORT_CGALTYPEDEF_H

#include <CGAL/Cartesian.h>
#include <CGAL/Vector_3.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Point_set_3.h>
#include <CGAL/Surface_mesh/Surface_mesh.h>

#include "GalibNamespaceDef.h"

namespace galib::minecraft::cgal_support{
#ifdef _WIN32
    using LtKernel = GALIB_CGAL Simple_cartesian<float>;
    using LtPoint3 = GALIB_CGAL Point_3<LtKernel>;
    using LtVector3 = GALIB_CGAL Vector_3<LtKernel>;
    using LtMesh = GALIB_CGAL Surface_mesh<GALIB_CGAL Point_3<GALIB_CGAL Simple_cartesian<float>>>;
#elif __APPLE__
    using LtKernel = GALIB_CGAL Simple_cartesian<double>;
    using LtPoint3 = GALIB_CGAL Point_3<LtKernel>;
    using LtVector3 = GALIB_CGAL Vector_3<LtKernel>;
    using LtMesh = GALIB_CGAL Surface_mesh<GALIB_CGAL Point_3<GALIB_CGAL Simple_cartesian<double>>>;
#endif
}

#endif //GALIB_MINECRAFT_CGALSUPPORT_CGALTYPEDEF_H
