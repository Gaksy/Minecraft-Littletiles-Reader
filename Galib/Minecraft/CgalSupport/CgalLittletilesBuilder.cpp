#include <Minecraft/CgalSupport/CgalLtSupport.h>
#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Polygon_mesh_processing/repair.h>
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
using GALIB minecraft::cgal_support::ChunkMesh;

using GALIB minecraft::littletiles::GridType;
using GALIB minecraft::littletiles::TileEntity;
using GALIB minecraft::littletiles::BoxTileEnities;
using GALIB minecraft::littletiles::BlockTileEntities;
using GALIB minecraft::littletiles::ChunkTileEntities;

using GALIB_CGAL SM_Vertex_index;

// Apply world coordinate offset to a mesh
LtMesh applyWorldOffset(const LtMesh& mesh, const galib::minecraft::BlockCoordinate & block_coordinate) {
    LtMesh transformed = mesh;
    using Point = LtMesh::Point;
    const double offset_x = block_coordinate.x;
    const double offset_y = block_coordinate.y;
    const double offset_z = block_coordinate.z;

    for(auto v : transformed.vertices()) {
        Point p = transformed.point(v);
        transformed.point(v) = Point(
            p.x() + offset_x,
            p.y() + offset_y,
            p.z() + offset_z
        );
    }
    return transformed;
}

LtMesh createIntersectionCube() {
    LtMesh mesh;

    using Point = LtMesh::Point;
    // Define 8 cube vertices
    const SM_Vertex_index v0 = mesh.add_vertex(Point(0, 0, 0));
    const SM_Vertex_index v1 = mesh.add_vertex(Point(1, 0, 0));
    const SM_Vertex_index v2 = mesh.add_vertex(Point(1, 1, 0));
    const SM_Vertex_index v3 = mesh.add_vertex(Point(0, 1, 0));
    const SM_Vertex_index v4 = mesh.add_vertex(Point(0, 0, 1));
    const SM_Vertex_index v5 = mesh.add_vertex(Point(1, 0, 1));
    const SM_Vertex_index v6 = mesh.add_vertex(Point(1, 1, 1));
    const SM_Vertex_index v7 = mesh.add_vertex(Point(0, 1, 1));

    // Define cube faces (each face as a quad, split into two triangles)
    mesh.add_face(v0, v1, v2);
    mesh.add_face(v0, v2, v3);

    mesh.add_face(v4, v5, v6);
    mesh.add_face(v4, v6, v7);

    mesh.add_face(v0, v1, v5);
    mesh.add_face(v0, v5, v4);

    mesh.add_face(v1, v2, v6);
    mesh.add_face(v1, v6, v5);

    mesh.add_face(v2, v3, v7);
    mesh.add_face(v2, v7, v6);

    mesh.add_face(v3, v0, v4);
    mesh.add_face(v3, v4, v7);

    return mesh;
}

void addTilesFromBlockTilesEntities(const BlockTileEntities &kBlockTileEntities, vector<LtMesh>& mesh_array) {
    const GridType grid_type = kBlockTileEntities.getGridType();

    // BlockTile -> BoxTile -> Tile

    // For BlockTile 遍历 Block 中的所有 Tile
    for(BlockTileEntities::const_iterator block_it = kBlockTileEntities.cbegin(); block_it != kBlockTileEntities.cend(); ++block_it) {
        // Get BoxTile entities 获取每个 Box Tile
        const BoxTileEnities& box_tile_entities = block_it->second;

        // For BoxTile 对于 Box Tile 中的每个 Tile，构建他的面
        for(BoxTileEnities::const_iterator tile_it = box_tile_entities.cbegin(); tile_it != box_tile_entities.cend(); ++tile_it) {

            // Get Tile entities
            const TileEntity& tile_entity = *tile_it;

            // Convert to Lt Mesh 创建面并添加到 tiles_mesh 中
            LtMesh tile_mesh = createMeshFromTileEntity(tile_entity, grid_type);
            GALIB_STD cout << tile_mesh << std::endl;
            GALIB_STD cout << "--split--" << std::endl;

            // 合并位置相同的点



            //
            // try {
            //     CGAL::Polygon_mesh_processing::remove_degenerate_faces(tile_mesh);
            // } catch (const CGAL::Assertion_exception& e) {
            //     std::cerr << "Warning: Degenerate face removal failed: " << e.what() << std::endl;
            //     continue;
            // }
            //
            // LtMesh intersection_cub_mesh = createIntersectionCube();
            //
            // // Compute intersection (assuming CGAL corefinement is available) 进行交集计算
            // LtMesh intersection_result;
            // if(CGAL::Polygon_mesh_processing::corefine_and_compute_intersection(tile_mesh, intersection_cub_mesh, intersection_result)) {
            //     // Apply world offset based on block_coordinate_ 应用世界坐标偏移
            //     LtMesh final_mesh = applyWorldOffset(intersection_result, kBlockTileEntities.getBlockCoordinate());
            //     mesh_array.push_back(final_mesh);
            // }
        }
    }
}

void ChunkMesh::addTilesFromChukTileEntities(const ChunkTileEntities& kChunkTileEntities) {
    for (auto block_it = kChunkTileEntities.cbegin(); block_it != kChunkTileEntities.cend(); ++block_it) {
        addTilesFromBlockTilesEntities(*block_it, this->tiles_in_world_);
    }
}

const std::vector<LtMesh>& ChunkMesh::getMeshArray() const {
    return this->tiles_in_world_;
}
