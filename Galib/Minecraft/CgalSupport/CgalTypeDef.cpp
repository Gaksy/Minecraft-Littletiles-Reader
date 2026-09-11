/*
 * Copyright (c) 2025 Gaksy (Fuhongren)
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
 * Date Created: 9/2/2025
 */

#include "Minecraft/CgalSupport/CgalTypeDef.h"

#include "Minecraft/CgalSupport/CgalLtSupport.h"

using GALIB_STD string;
using GALIB_STD replace;

using GALIB minecraft::cgal_support::LtSurfaceMesh;
using GALIB minecraft::BlockCoordinate;
using GALIB minecraft::cgal_support::applyWorldOffset;
using GALIB minecraft::cgal_support::SurfaceMeshType;
using GALIB minecraft::cgal_support::UVData;

LtSurfaceMesh::LtSurfaceMesh(const SurfaceMeshType& mesh)
    : surface_mesh_(mesh) {}

SurfaceMeshType& LtSurfaceMesh::getMesh() { return this->surface_mesh_; }

const SurfaceMeshType& LtSurfaceMesh::getMesh() const {
  return this->surface_mesh_;
}

void LtSurfaceMesh::setBlockID(const string& str) { this->block_id_ = str; }

const string& LtSurfaceMesh::getBlockID() const { return this->block_id_; }

void LtSurfaceMesh::setTileColor(const GALIB_STD int32_t kColor,
                                 const bool kHasColor) {
  this->tile_color_ = kColor;
  this->has_tile_color_ = kHasColor;
}

bool LtSurfaceMesh::hasTileColor() const { return this->has_tile_color_; }

GALIB_STD int32_t LtSurfaceMesh::getTileColor() const {
  return this->tile_color_;
}

void LtSurfaceMesh::setBlockCoordInWorld(const BlockCoordinate& kBlockCoord) {
  this->block_coord_in_world_ = kBlockCoord;
}

const BlockCoordinate& LtSurfaceMesh::getBlockCoord() const {
  return this->block_coord_in_world_;
}

SurfaceMeshType LtSurfaceMesh::getMeshWithOffset(
    const BlockCoordinate& offset) const {
  SurfaceMeshType mesh = getMesh();
  applyWorldOffset(mesh, offset);
  return mesh;
}

void LtSurfaceMesh::applyOffset(const BlockCoordinate& offset) {
  applyWorldOffset(surface_mesh_, offset);
}

string LtSurfaceMesh::getFormatBlockID() const {
  string formated_block_id = block_id_;
  replace(formated_block_id.begin(), formated_block_id.end(), ':', '_');
  return formated_block_id;
}

UVData LtSurfaceMesh::calculateFaceUV(
    const SurfaceMeshType::face_index& kFaceIndex) {
  return {};
}
