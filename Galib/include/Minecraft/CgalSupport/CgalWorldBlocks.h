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
#include "Minecraft/CgalSupport/CgalTypeDef.h"
#include "Minecraft/ChunkBlocks.h"

namespace galib::minecraft::cgal_support {

// Convert the "plain blocks" (solid blocks) of a batch of chunks into full cube
// meshes.
//
// Unlike LittleTiles tiles, a plain block is itself a 1x1x1 cube (its UVs are
// exactly the whole texture), so only three things are done here:
//   1. Skip air and the LittleTiles host blocks (whose appearance is expressed
//      by the tiles);
//   2. Group by (block id, meta) and build one mesh per group (so the mesh-level
//      block name and colour still hold);
//   3. Optionally cull neighbours - only faces pointing at air or outside the
//      region are emitted.
//
// kChunks is given in row-major order: chunk_x varies fastest (the outer loop is
// z, the inner loop is x).
// kWorldOriginX/kWorldOriginZ is the world block coordinate of the lower-left
// corner of this region - the output must use world coordinates, otherwise it
// cannot be aligned with the LittleTiles tiles.
void BuildWorldBlockMeshes(int kWorldOriginX, int kWorldOriginZ,
                           const std::vector<minecraft::ChunkBlocks>& kChunks,
                           int kChunkSizeX, int kChunkSizeZ,
                           const minecraft::BlockIdTable& kBlockIdTable,
                           bool kCullHiddenFaces,
                           std::vector<LtSurfaceMesh>* p_desc_meshes);

}  // namespace galib::minecraft::cgal_support

#endif  // GALIB_MINECRAFT_CGALSUPPORT_CGALWORLDBLOCKS_H
