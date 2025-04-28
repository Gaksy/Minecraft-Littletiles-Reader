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
 * Date Created: MM/dd/YYYY
 */



#ifndef GALIB_INCLUDE_MINECRAFT_CGALSUPPORT_CGALLITTLETILEBUIDER_H
#define GALIB_INCLUDE_MINECRAFT_CGALSUPPORT_CGALLITTLETILEBUIDER_H

#include "GalibNamespaceDef.h"
#include "Minecraft/CgalSupport/CgalTypeDef.h"
#include "Minecraft/LittleTilesCoord.h"

namespace galib::minecraft::cgal_support {
    struct TileMesh {
        LtMesh mesh;
        GALIB minecraft::littletiles::GridType grid_type = 16;
    };
}

#endif //GALIB_INCLUDE_MINECRAFT_CGALSUPPORT_CGALLITTLETILEBUIDER_H
