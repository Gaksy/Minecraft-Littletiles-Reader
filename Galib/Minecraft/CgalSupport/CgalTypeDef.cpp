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

using std::replace;
using std::string;

using galib::minecraft::BlockCoordinate;
using galib::minecraft::cgal_support::ApplyWorldOffset;
using galib::minecraft::cgal_support::LtSurfaceMesh;
using galib::minecraft::cgal_support::SurfaceMeshType;
using galib::minecraft::cgal_support::UVData;

LtSurfaceMesh::LtSurfaceMesh(const SurfaceMeshType& mesh)
    : surface_mesh_(mesh) {}

SurfaceMeshType& LtSurfaceMesh::surface_mesh() { return this->surface_mesh_; }

const SurfaceMeshType& LtSurfaceMesh::surface_mesh() const {
  return this->surface_mesh_;
}

void LtSurfaceMesh::set_block_id(const string& str) { this->block_id_ = str; }

const string& LtSurfaceMesh::block_id() const { return this->block_id_; }

void LtSurfaceMesh::set_tile_color(const std::int32_t kColor,
                                   const bool kHasColor) {
  this->tile_color_ = kColor;
  this->has_tile_color_ = kHasColor;
}

bool LtSurfaceMesh::has_tile_color() const { return this->has_tile_color_; }

std::int32_t LtSurfaceMesh::tile_color() const { return this->tile_color_; }

void LtSurfaceMesh::set_block_coord_in_world(
    const BlockCoordinate& kBlockCoord) {
  this->block_coord_in_world_ = kBlockCoord;
}

const BlockCoordinate& LtSurfaceMesh::block_coord_in_world() const {
  return this->block_coord_in_world_;
}

SurfaceMeshType LtSurfaceMesh::GetMeshWithOffset(
    const BlockCoordinate& offset) const {
  SurfaceMeshType mesh = surface_mesh();
  ApplyWorldOffset(mesh, offset);
  return mesh;
}

void LtSurfaceMesh::ApplyOffset(const BlockCoordinate& offset) {
  ApplyWorldOffset(surface_mesh_, offset);
}

string LtSurfaceMesh::GetFormatBlockId() const {
  string formated_block_id = block_id_;
  replace(formated_block_id.begin(), formated_block_id.end(), ':', '_');
  return formated_block_id;
}

UVData LtSurfaceMesh::CalculateFaceUv(
    const SurfaceMeshType::face_index& kFaceIndex) {
  return {};
}
