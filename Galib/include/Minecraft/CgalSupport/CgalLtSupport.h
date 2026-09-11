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
inline LtPoint3 convertToCGALPoint(
    const GALIB minecraft::littletiles::LittleTilesCoord& kLtCoord) {
  return GALIB minecraft::cgal_support::LtPoint3(kLtCoord.x, kLtCoord.y,
                                                 kLtCoord.z);
}

void createMeshFromTileEntity(
    LtSurfaceMesh& mesh,
    const GALIB minecraft::littletiles::TileEntity& kTileEntity,
    bool kApplyOffset = true);

// 用"半空间裁剪"替代 CGAL 布尔求交。
// tile 的偏移形状是一个凸六面体，裁剪体是轴对齐包围盒，两者交集仍是凸多面体，
// 因此可以对每个面依次用 Sutherland–Hodgman 裁 6 个半空间，并为切面补上新面。
// 与布尔求交相比：新顶点只出现在切面上、不产生碎三角形、共面情况无需数值容错。
// 裁剪体为"偏移前的盒子"（与原先 corefine_and_compute_intersection 的用法一致）。
// desc_mesh 应为本 tile 新建的空网格。
// 返回 false 表示裁剪结果为空，调用方应丢弃该 tile。
bool clipTileEntityToBox(
    LtSurfaceMesh& desc_mesh,
    const GALIB minecraft::littletiles::TileEntity& kTileEntity,
    bool kApplyOffset = true);

const LtSurfaceMesh& createIntersectionCube(
    GALIB minecraft::littletiles::GridType kGrid);
void applyWorldOffset(LtSurfaceMesh& mesh,
                      const BlockCoordinate& block_coordinate);
void applyWorldOffset(SurfaceMeshType& mesh,
                      const BlockCoordinate& block_coordinate);
void applyGrid(LtSurfaceMesh& mesh,
               GALIB minecraft::littletiles::GridType grid);
void cleanupMesh(LtSurfaceMesh& mesh);
}  // namespace galib::minecraft::cgal_support

#endif  //GALIB_INCLUDE_MINECRAFT_CGALSUPPORT_CGALLTSUPPORT_H
