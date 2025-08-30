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
 * Date Created: 12/24/2024
 */



#ifndef GALIB_INCLUDE_MINECRAFT_CGALSUPPORT_CGALLITTLETILEBUIDER_H
#define GALIB_INCLUDE_MINECRAFT_CGALSUPPORT_CGALLITTLETILEBUIDER_H

#include <vector>

#include "GalibNamespaceDef.h"
#include "Minecraft/CgalSupport/CgalTypeDef.h"
#include "Minecraft/LittleTiles.h"

namespace galib::minecraft::cgal_support {
    class ChunkMesh {
    public:
        using container = GALIB_STD vector<LtMesh>;
        using const_iterator = GALIB_STD vector<LtMesh>::const_iterator;
        using iterator = GALIB_STD vector<LtMesh>::iterator;

    public:
        ChunkMesh()=default;
        ~ChunkMesh()=default;

    public:
        void addTilesFromChukTileEntities(const GALIB minecraft::littletiles::ChunkTileEntities& kChunkTileEntities, bool kApplyWorldOffset = true);
        GALIB_NODISCARD const container & getMeshArray()const;
        void clear();

    private:
        container tiles_in_world_;
    };

    void margeAndWriteToObj(const GALIB_STD vector<LtMesh>& meshes, const char* const p_filename);
    void margeAndWriteToOff(const GALIB_STD vector<LtMesh>& meshes, const char* const p_filename);
}

#endif //GALIB_INCLUDE_MINECRAFT_CGALSUPPORT_CGALLITTLETILEBUIDER_H
