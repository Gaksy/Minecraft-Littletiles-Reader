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

#include "Minecraft/CgalSupport/CgalLtSupport.h"

using GALIB_STD string;

using GALIB minecraft::cgal_support::LtSurfaceMesh;
using GALIB minecraft::BlockCoordinate;
using GALIB minecraft::cgal_support::applyWorldOffset;

LtSurfaceMesh::LtSurfaceMesh(const SurfaceMeshType& mesh):
    surface_mesh_(mesh)
{ }

LtSurfaceMesh::SurfaceMeshType& LtSurfaceMesh::getMesh() {
    return this->surface_mesh_;
}

const LtSurfaceMesh::SurfaceMeshType& LtSurfaceMesh::getMesh()const {
    return this->surface_mesh_;
}

void LtSurfaceMesh::setBlockID(const string& str) {
    this->block_id_ = str;
}

const string& LtSurfaceMesh::getBlockID() const {
    return this->block_id_;
}

void LtSurfaceMesh::setBlockCoordInWorld(const BlockCoordinate& kBlockCoord) {
    this->block_coord_in_world_ = kBlockCoord;
}


const BlockCoordinate& LtSurfaceMesh::getBlockCoord() const {
    return this->block_coord_in_world_;
}

LtSurfaceMesh::SurfaceMeshType LtSurfaceMesh::getMesWithOffset(const BlockCoordinate& offset)const {
    SurfaceMeshType mesh = getMesh();
    applyWorldOffset(mesh, offset);
    return mesh;
}

void LtSurfaceMesh::applyOffset(const BlockCoordinate& offset) {
    applyWorldOffset(surface_mesh_, offset);
}

