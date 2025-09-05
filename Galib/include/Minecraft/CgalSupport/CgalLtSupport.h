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
#include "Minecraft/LittleTilesCoord.h"
#include "Minecraft/LittleTiles.h"

namespace galib::minecraft::cgal_support {
    inline LtPoint3 convertToCGALPoint(const GALIB minecraft::littletiles::LittleTilesCoord& kLtCoord) {
        return GALIB minecraft::cgal_support::LtPoint3(kLtCoord.x, kLtCoord.y, kLtCoord.z);
    }

    void createMeshFromTileEntity(LtSurfaceMesh& mesh, const GALIB minecraft::littletiles::TileEntity& kTileEntity, bool kApplyOffset = true);
    const LtSurfaceMesh& createIntersectionCube(GALIB minecraft::littletiles::GridType kGrid);
    void applyWorldOffset(LtSurfaceMesh& mesh, const BlockCoordinate & block_coordinate);
    void applyWorldOffset(SurfaceMeshType& mesh, const BlockCoordinate & block_coordinate);
    void applyGrid(LtSurfaceMesh& mesh, GALIB minecraft::littletiles::GridType grid);
    void cleanupMesh(LtSurfaceMesh& mesh);
}

#endif //GALIB_INCLUDE_MINECRAFT_CGALSUPPORT_CGALLTSUPPORT_H
