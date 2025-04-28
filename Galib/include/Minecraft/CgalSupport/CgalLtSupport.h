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

#include "Minecraft/CgalSupport/CgalTypeDef.h"
#include "Minecraft/LittleTilesCoord.h"
#include "Minecraft/LittleTiles.h"

namespace galib::minecraft::cgal_support {
    inline GALIB minecraft::cgal_support::LtPoint3 convertToCGALPoint(const GALIB minecraft::littletiles::LittleTilesCoord& kLtCoord) {
        return GALIB minecraft::cgal_support::LtPoint3(kLtCoord.x, kLtCoord.y, kLtCoord.z);
    }

    void createMeshFromTileEntity(LtMesh& mesh, const GALIB minecraft::littletiles::TileEntity& tileEntity);
}

#endif //GALIB_INCLUDE_MINECRAFT_CGALSUPPORT_CGALLTSUPPORT_H
