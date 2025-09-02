/*
 * This Source Code Form is subject to the terms of the GNU Lesser General Public License,
 * v. 3.0. If a copy of the LGPL was not distributed with this file, You can obtain one at
 * https://www.gnu.org/licenses/lgpl-3.0.html.
 *
 * Copyright (c) 2024 Gaksy (Fuhongren)
 *
 * Author: Gaksy
 * Date Created: 2025/9/2
 *
 * Additional Terms:
 * For commercial use of this software, you must obtain separate authorization from the original author.
 * Please contact Gaksy at gaksys@outlook.com to request commercial use authorization.
 */

#include "Minecraft/CgalSupport/CgalTypeDef.h"

galib::minecraft::cgal_support::LtSurfaceMesh::LtSurfaceMesh(const SurfaceMeshType& mesh):
    surface_mesh_(mesh)
{ }

galib::minecraft::cgal_support::LtSurfaceMesh::SurfaceMeshType& galib::minecraft::cgal_support::LtSurfaceMesh::getMesh() {
    return this->surface_mesh_;
}

const galib::minecraft::cgal_support::LtSurfaceMesh::SurfaceMeshType& galib::minecraft::cgal_support::LtSurfaceMesh::getMesh()const {
    return this->surface_mesh_;
}

