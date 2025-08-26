#include <Minecraft/CgalSupport/CgalLtSupport.h>
;/*
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

#include "Minecraft/CgalSupport/CgalLittletilesBuilder.h"

using GALIB_STD vector;

using GALIB minecraft::cgal_support::LtMesh;
using GALIB minecraft::cgal_support::createMeshFromTileEntity;
using GALIB minecraft::cgal_support::BlockMesh;

using GALIB minecraft::littletiles::GridType;
using GALIB minecraft::littletiles::TileEntity;
using GALIB minecraft::littletiles::BoxTileEnities;
using GALIB minecraft::littletiles::BlockTileEntities;

BlockMesh::BlockMesh():
 grid_type_(16)
{ ; }

void BlockMesh::addTilesFromBlockTileEntities(const BlockTileEntities &kBlockTileEntities) {
    // tiles mesh array
    container tiles_mesh;
    const GridType grid_type = kBlockTileEntities.getGridType();

    // BlockTile -> BoxTile -> Tile

    // For BlockTile
    for(auto block_it = kBlockTileEntities.cbegin(); block_it != kBlockTileEntities.cend(); ++block_it) {
        // Get BoxTile entities
        const BoxTileEnities& box_tile_entities = block_it->second;

        // For BoxTile
        for(auto tile_it = box_tile_entities.begin(); tile_it != box_tile_entities.end(); ++tile_it) {

            // Get Tile entities
            const TileEntity& tile_entity = *tile_it;

            // Convert to Lt Mesh
            LtMesh mesh = createMeshFromTileEntity(tile_entity, grid_type);
            tiles_mesh.push_back(mesh);
        }
    }

    // Get GridType
    grid_type_ = grid_type;
    this->tiles_.swap(tiles_mesh);
}

const vector<LtMesh> & BlockMesh::getTilesMesh() const {
    return this->tiles_;
}
