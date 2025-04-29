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

#include <vector>

#include "GalibNamespaceDef.h"
#include "Minecraft/CgalSupport/CgalTypeDef.h"
#include "Minecraft/LittleTilesCoord.h"
#include "Minecraft/LittleTiles.h"

namespace galib::minecraft::cgal_support {
    class BlockMesh {
    public:
        using container = GALIB_STD vector<LtMesh>;
        using const_iterator = GALIB_STD vector<LtMesh>::const_iterator;
        using iterator = GALIB_STD vector<LtMesh>::iterator;

    public:
        BlockMesh();
        ~BlockMesh()=default;

    public:
        void addTilesFromBlockTileEntities(const GALIB minecraft::littletiles::BlockTileEntities& kBlockTileEntities);
        GALIB_NODISCARD const GALIB_STD vector<LtMesh>& getTilesMesh()const;

    private:
        container tiles_;
        GALIB minecraft::littletiles::GridType grid_type_;
    };


}

#endif //GALIB_INCLUDE_MINECRAFT_CGALSUPPORT_CGALLITTLETILEBUIDER_H
