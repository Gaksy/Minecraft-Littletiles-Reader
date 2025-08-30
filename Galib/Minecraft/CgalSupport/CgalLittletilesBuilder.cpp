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

#include "Minecraft/CgalSupport/CgalLittletilesBuilder.h"

#include <Minecraft/CgalSupport/CgalLtSupport.h>
#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Polygon_mesh_processing/repair.h>

using GALIB_STD vector;
using GALIB_STD ofstream;
using GALIB_STD distance;
using GALIB_STD find;
using GALIB_STD endl;
using GALIB_STD cerr;
using GALIB_STD map;

using GALIB minecraft::cgal_support::LtMesh;
using GALIB minecraft::cgal_support::createMeshFromTileEntity;
using GALIB minecraft::cgal_support::ChunkMesh;
using GALIB minecraft::cgal_support::applyGrid;
using GALIB minecraft::cgal_support::applyWorldOffset;
using GALIB minecraft::cgal_support::cleanupMesh;

using GALIB minecraft::littletiles::GridType;
using GALIB minecraft::littletiles::TileEntity;
using GALIB minecraft::littletiles::BoxTileEnities;
using GALIB minecraft::littletiles::BlockTileEntities;
using GALIB minecraft::littletiles::ChunkTileEntities;

using GALIB_CGAL SM_Vertex_index;
using GALIB_CGAL Polygon_mesh_processing::corefine_and_compute_intersection;

void addTilesFromBlockTilesEntities(const BlockTileEntities &kBlockTileEntities, vector<LtMesh>& mesh_array, const bool kApplyWorldOffset) {
    const GridType grid_type = kBlockTileEntities.getGridType();
    // BlockTile -> BoxTile -> Tile

    // For BlockTile 遍历 Block 中的所有 Tile
    for(BlockTileEntities::const_iterator block_it = kBlockTileEntities.cbegin(); block_it != kBlockTileEntities.cend(); ++block_it) {
        // Get BoxTile entities 获取每个 Box Tile
        const BoxTileEnities& box_tile_entities = block_it->second;

        // For BoxTile 对于 Box Tile 中的每个 Tile，构建他的面
        for(BoxTileEnities::const_iterator tile_it = box_tile_entities.cbegin(); tile_it != box_tile_entities.cend(); ++tile_it) {

            // Get Tile entities
            const TileEntity& tile_lt_entity = *tile_it;

            // 将 tile entities 转换为 cgal 网格
            LtMesh tile_cgal_mesh;
            createMeshFromTileEntity(tile_cgal_mesh, tile_lt_entity);

            // 如有偏移
            if (tile_lt_entity.hasAnyOffsetEnable()) {
                // 创建裁剪网格体
                LtMesh tile_cgal_mehs_aabb;
                createMeshFromTileEntity(tile_cgal_mehs_aabb, tile_lt_entity, false);

                // 预处理网格：清理退化元素
                cleanupMesh(tile_cgal_mesh);
                cleanupMesh(tile_cgal_mehs_aabb);

                // 计算裁剪
                LtMesh tile_cgal_final_mesh;
                try {
                    if (corefine_and_compute_intersection(tile_cgal_mesh, tile_cgal_mehs_aabb, tile_cgal_final_mesh)) {
                        std::swap(tile_cgal_mesh, tile_cgal_final_mesh);
                    }
                } catch ( ... ) {
                    continue;
                }
            }

            tile_cgal_mesh = applyGrid(tile_cgal_mesh, grid_type);
            if (kApplyWorldOffset) {
                tile_cgal_mesh = applyWorldOffset(tile_cgal_mesh, kBlockTileEntities.getBlockCoordinate());
            }

            mesh_array.push_back(tile_cgal_mesh);
        }
    }
}

void ChunkMesh::addTilesFromChukTileEntities(const ChunkTileEntities& kChunkTileEntities, const bool kApplyWorldOffset) {
    for (auto block_it = kChunkTileEntities.cbegin(); block_it != kChunkTileEntities.cend(); ++block_it) {
        addTilesFromBlockTilesEntities(*block_it, this->tiles_in_world_, kApplyWorldOffset);
    }
}

const ChunkMesh::container & ChunkMesh::getMeshArray() const {
    return this->tiles_in_world_;
}

void ChunkMesh::clear() {
    this->tiles_in_world_.clear();
}


void  GALIB minecraft::cgal_support::margeAndWriteToObj(const vector<LtMesh>& meshes, const char* const p_filename) {
    // 创建一个新的网格用于合并所有网格
    LtMesh merged_mesh;

    // 用于追踪顶点的索引
    map<typename LtMesh::Point, typename LtMesh::Vertex_index> vertex_map;

    // 遍历每个网格
    for (const auto& mesh : meshes) {
        // 遍历网格中的每个顶点
        for (auto v : mesh.vertices()) {
            const auto& point = mesh.point(v);

            // 如果顶点已存在，则跳过
            if (vertex_map.find(point) == vertex_map.end()) {
                vertex_map[point] = merged_mesh.add_vertex(point);
            }
        }

        // 遍历网格中的每个面
        for (auto f : mesh.faces()) {
            vector<typename LtMesh::Vertex_index> face_vertices;
            // 获取面上的顶点
            for (auto v : mesh.vertices_around_face(mesh.halfedge(f))) {
                face_vertices.push_back(vertex_map[mesh.point(v)]);
            }

            // 添加合并后的面
            merged_mesh.add_face(face_vertices);
        }
    }

    // 将合并后的网格写入 .obj 文件
    ofstream obj_file(p_filename);
    if (!obj_file) {
        cerr << "can't open file " << p_filename << endl;
        return;
    }

    // 写入顶点
    for (const auto& v : merged_mesh.vertices()) {
        const auto& point = merged_mesh.point(v);
        obj_file << "v " << point.x() << " " << point.y() << " " << point.z() << endl;
    }

    // 写入面
    for (const auto& f : merged_mesh.faces()) {
        vector<int> face_indices;
        for (auto v : merged_mesh.vertices_around_face(merged_mesh.halfedge(f))) {
            face_indices.push_back(distance(merged_mesh.vertices().begin(), find(merged_mesh.vertices().begin(), merged_mesh.vertices().end(), v)) + 1);
        }

        obj_file << "f";
        for (const auto& idx : face_indices) {
            obj_file << " " << idx;
        }
        obj_file << endl;
    }

    obj_file.close();
}

void writeMeshToOff(const LtMesh& mesh, const char* const p_filename) {
    std::ofstream out(p_filename);
    if (!out) {
        std::cerr << "Error opening file: " << p_filename << std::endl;
        return;
    }

    // Writing OFF header
    out << "OFF\n";

    // Count vertices and faces
    size_t vertex_count = mesh.number_of_vertices();
    size_t face_count = mesh.number_of_faces();

    // Writing number of vertices, faces, and edges
    out << vertex_count << " " << face_count << " 0\n";

    // Writing vertices
    for (auto v : mesh.vertices()) {
        auto point = mesh.point(v);
        out << point.x() << " " << point.y() << " " << point.z() << "\n";
    }

    // Writing faces
    for (auto f : mesh.faces()) {
        auto halfedges = mesh.halfedges_around_face(mesh.halfedge(f));
        std::vector<int> vertices;
        for (auto h : halfedges) {
            vertices.push_back(mesh.target(h));
        }
        out << vertices.size();
        for (int v : vertices) {
            out << " " << v;
        }
        out << "\n";
    }

    out.close();
}

void GALIB minecraft::cgal_support::margeAndWriteToOff(const GALIB_STD vector<LtMesh>& meshes, const char* const p_filename) {
    for (size_t i = 0; i < meshes.size(); ++i) {
        // Generate a unique filename for each mesh
        std::string mesh_filename = std::string(p_filename) + "_" + std::to_string(i) + ".off";
        writeMeshToOff(meshes[i], mesh_filename.c_str());
        std::cout << "Written mesh " << i << " to " << mesh_filename << std::endl;
    }
}