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
 * Date Created: 09/12/2026
 */

#ifndef GALIB_MINECRAFT_CGALSUPPORT_CGALWORLDBLOCKS_H
#define GALIB_MINECRAFT_CGALSUPPORT_CGALWORLDBLOCKS_H

#include <vector>

#include "GalibNamespaceDef.h"
#include "Minecraft/BlockIdTable.h"
#include "Minecraft/ChunkBlocks.h"
#include "Minecraft/CgalSupport/CgalTypeDef.h"

namespace galib::minecraft::cgal_support {

// 把一批区块里的"普通方块"（实体方块）转成完整立方体网格。
//
// 与 LittleTiles 的 tile 不同，普通方块本身就是一个 1x1x1 的立方体
// （UV 恰好是整张贴图），因此这里只做三件事：
//   1. 跳过空气与 LittleTiles 的宿主方块（后者外观由 tile 表达）；
//   2. 按 (方块 id, meta) 分组，每组生成一个网格（网格级的方块名与颜色因此仍然成立）；
//   3. 可选邻居剔除——只输出朝向空气或区域之外的面。
//
// kChunks 按行主序给出：chunk_x 从慢到快（外层是 z、内层是 x）。
void BuildWorldBlockMeshes(const std::vector<minecraft::ChunkBlocks>& kChunks, int kChunkSizeX,
                           int kChunkSizeZ, const minecraft::BlockIdTable& kBlockIdTable,
                           bool kCullHiddenFaces, std::vector<LtSurfaceMesh>* p_desc_meshes);

}  // namespace galib::minecraft::cgal_support

#endif  // GALIB_MINECRAFT_CGALSUPPORT_CGALWORLDBLOCKS_H
