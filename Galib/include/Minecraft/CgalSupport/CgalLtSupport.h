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

#include "Minecraft/CgalSupport/CgalTypeDef.h"
#include "Minecraft/LittleTilesCoord.h"
#include "Minecraft/LittleTiles.h"

namespace galib::minecraft::cgal_support {
    inline GALIB minecraft::cgal_support::LtPoint3 convertToCGALPoint(const GALIB minecraft::littletiles::LittleTilesCoord& kLtCoord) {
        return GALIB minecraft::cgal_support::LtPoint3(kLtCoord.x, kLtCoord.y, kLtCoord.z);
    }

    void createMeshFromTileEntity(LtMesh& mesh, const GALIB minecraft::littletiles::TileEntity& tileEntity) {



        // for (const GALIB minecraft::littletiles::AngleOffset& offset : tileEntity.offset_data) {
        //     // 使用顶点和偏移来创建面
        //     // 你需要根据具体的实现细节将顶点添加到 mesh 中
        //     // LtPoint3 vertex1 = applyOffsetToPoint(pos1, offset);
        //     // LtPoint3 vertex2 = applyOffsetToPoint(pos2, offset);
        //     // LtPoint3 vertex3 = applyOffsetToPoint(pos3, offset);
        //     // LtPoint3 vertex4 = applyOffsetToPoint(pos4, offset);
        //
        //     // 创建三角形并添加到 mesh
        //     // 这里需要使用正确的顶点索引进行三角形添加
        //     mesh.add_face(vertex1, vertex2, vertex3);
        //     mesh.add_face(vertex2, vertex3, vertex4);
        // }
    }


}

#endif //GALIB_INCLUDE_MINECRAFT_CGALSUPPORT_CGALLTSUPPORT_H
