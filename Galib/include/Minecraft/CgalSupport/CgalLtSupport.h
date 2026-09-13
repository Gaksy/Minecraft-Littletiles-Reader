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

#ifndef GALIB_INCLUDE_MINECRAFT_CGALSUPPORT_CGALLTSUPPORT_H
#define GALIB_INCLUDE_MINECRAFT_CGALSUPPORT_CGALLTSUPPORT_H

#include <CGAL/Surface_mesh/Surface_mesh.h>

#include "GalibNamespaceDef.h"
#include "Minecraft/CgalSupport/CgalTypeDef.h"
#include "Minecraft/LittleTiles.h"
#include "Minecraft/LittleTilesCoord.h"

namespace galib::minecraft::cgal_support {
inline LtPoint3 ConvertToCgalPoint(
    const galib::minecraft::littletiles::LittleTilesCoord& kLtCoord) {
  return galib::minecraft::cgal_support::LtPoint3(kLtCoord.x, kLtCoord.y,
                                                  kLtCoord.z);
}

void CreateMeshFromTileEntity(
    LtSurfaceMesh& mesh,
    const galib::minecraft::littletiles::TileEntity& kTileEntity,
    bool kApplyOffset = true);

// Use "half-space clipping" instead of a CGAL boolean intersection.
// The offset shape of a tile is a convex hexahedron and the clipping volume is
// an axis-aligned bounding box, so their intersection is still a convex
// polyhedron; therefore each face can be clipped against the 6 half-spaces in
// turn with Sutherland-Hodgman, adding new faces for the cut planes.
// Compared with a boolean intersection: new vertices only appear on the cut
// planes, no sliver triangles are produced, and coplanar cases need no numeric
// tolerance.
// The clipping volume is the "box before the offset" (same usage as the former
// corefine_and_compute_intersection).
// desc_mesh should be an empty mesh newly created for this tile.
// Returning false means the clipping result is empty and the caller should
// discard the tile.
bool ClipTileEntityToBox(
    LtSurfaceMesh& desc_mesh,
    const galib::minecraft::littletiles::TileEntity& kTileEntity,
    bool kApplyOffset = true);

const LtSurfaceMesh& CreateIntersectionCube(
    galib::minecraft::littletiles::GridType kGrid);
void ApplyWorldOffset(LtSurfaceMesh& mesh,
                      const BlockCoordinate& block_coordinate);
void ApplyWorldOffset(SurfaceMeshType& mesh,
                      const BlockCoordinate& block_coordinate);
void ApplyGrid(LtSurfaceMesh& mesh,
               galib::minecraft::littletiles::GridType grid);
void CleanupMesh(LtSurfaceMesh& mesh);
}  // namespace galib::minecraft::cgal_support

#endif  //GALIB_INCLUDE_MINECRAFT_CGALSUPPORT_CGALLTSUPPORT_H
