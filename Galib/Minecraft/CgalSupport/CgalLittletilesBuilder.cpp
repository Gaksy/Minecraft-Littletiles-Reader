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
using GALIB_STD cout;
using GALIB_STD map;
using GALIB_STD size_t;

using GALIB minecraft::cgal_support::LtSurfaceMesh;
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

size_t addTilesFromBlockTilesEntities(const BlockTileEntities &kBlockTileEntities, vector<LtSurfaceMesh>& mesh_array) {
    const GridType grid_type = kBlockTileEntities.getGridType();
    size_t processed_tile_count = 0;
    // BlockTile -> BoxTile -> Tile

    // For BlockTile 遍历 Block 中的所有 boxes
    for(BlockTileEntities::const_iterator box_it = kBlockTileEntities.cbegin(); box_it != kBlockTileEntities.cend(); ++box_it) {
        // Get BoxTile entities 获取每个 Box Tile
        const BoxTileEnities& box_tile_entities = box_it->second;

        // For BoxTile 对于 Box Tile 中的每个 Tile，构建他的面
        for(BoxTileEnities::const_iterator tile_it = box_tile_entities.cbegin(); tile_it != box_tile_entities.cend(); ++tile_it) {

            // Get Tile entities
            const TileEntity& tile_lt_entity = *tile_it;

            // 将 tile entities 转换为 cgal 网格
            LtSurfaceMesh tile_cgal_mesh;
            createMeshFromTileEntity(tile_cgal_mesh, tile_lt_entity);

            // 如有偏移且超出边界
            if (tile_lt_entity.hasAnyOffsetEnable()) {
                // 创建裁剪网格体
                LtSurfaceMesh tile_cgal_mesh_aabb;
                createMeshFromTileEntity(tile_cgal_mesh_aabb, tile_lt_entity, false);

                // 预处理网格：清理退化元素
                cleanupMesh(tile_cgal_mesh);
                cleanupMesh(tile_cgal_mesh_aabb);

                // 计算裁剪
                LtSurfaceMesh tile_cgal_cut_final_mesh;
                try {
                    if (corefine_and_compute_intersection(tile_cgal_mesh.getMesh(), tile_cgal_mesh_aabb.getMesh(), tile_cgal_cut_final_mesh.getMesh())) {
                        GALIB_STD swap(tile_cgal_mesh, tile_cgal_cut_final_mesh);
                    }
#ifdef GALIB_DEBUG
                    else {
                        printf("CgalLittletilesBuilder::addTilesFromBlockTilesEntities error: intersection error\n");
                    }
#endif
                } catch (const GALIB_STD exception& e) {
#ifdef GALIB_DEBUG
                    printf("CgalLittletilesBuilder::addTilesFromBlockTilesEntities error: %s\n", e.what());
#endif
                    continue;
                }
            }

            applyGrid(tile_cgal_mesh, grid_type);
            tile_cgal_mesh.setBlockCoordInWorld(kBlockTileEntities.getBlockCoordinate());
            tile_cgal_mesh.setBlockID(box_it->first);
            tile_cgal_mesh.applyOffset(tile_cgal_mesh.getBlockCoord());
            mesh_array.push_back(tile_cgal_mesh);
            ++processed_tile_count;
        }
    }

    return processed_tile_count;
}

ChunkMesh::size_type ChunkMesh::addTilesFromChukTileEntities(const ChunkTileEntities& kChunkTileEntities) {
    size_t processed_tile_count = 0;
    for (auto block_it = kChunkTileEntities.cbegin(); block_it != kChunkTileEntities.cend(); ++block_it) {
        processed_tile_count += addTilesFromBlockTilesEntities(*block_it, this->tiles_in_world_);
    }
    return processed_tile_count;
}

const ChunkMesh::container & ChunkMesh::getMeshArray() const {
    return this->tiles_in_world_;
}

void ChunkMesh::clear() {
    this->tiles_in_world_.clear();
}

void GALIB minecraft::cgal_support::margeAndWriteToObj(const std::vector<LtSurfaceMesh>& meshes, const char* const p_filename) {
    // 创建一个空的合并网格
    LtSurfaceMesh::SurfaceMeshType merged_mesh;

    // 顶点映射，用于避免重复添加顶点
    std::unordered_map<typename LtSurfaceMesh::SurfaceMeshType::Vertex_index, typename LtSurfaceMesh::SurfaceMeshType::Vertex_index> vertex_map;

    // 遍历每个网格并将它们合并到 merged_mesh 中
    for (const auto& mesh : meshes) {
        // 复制当前网格的顶点到合并网格中
        for (const auto& v : mesh.getMesh().vertices()) {
            // 如果该顶点未出现过，添加到合并网格中
            if (vertex_map.find(v) == vertex_map.end()) {
                vertex_map[v] = merged_mesh.add_vertex(mesh.getMesh().point(v));
            }
        }

        // 复制当前网格的面到合并网格中
        for (const auto& f : mesh.getMesh().faces()) {
            std::vector<typename LtSurfaceMesh::SurfaceMeshType::Vertex_index> face_vertices;

            // 使用半边迭代器遍历面上的顶点
            for (auto h : mesh.getMesh().halfedges_around_face(mesh.getMesh().halfedge(f))) {
                face_vertices.push_back(vertex_map[mesh.getMesh().target(h)]);  // 使用已合并的顶点
            }
            merged_mesh.add_face(face_vertices); // 将面添加到合并网格
        }
    }

    // 将合并后的网格写入 obj 文件
    CGAL::IO::write_polygon_mesh(p_filename, merged_mesh);
}

void writeMeshToOff(const LtSurfaceMesh& mesh, const char* const p_filename) {
    std::ofstream out(p_filename);
    if (!out) {
        std::cerr << "Error opening file: " << p_filename << std::endl;
        return;
    }

    // Writing OFF header
    out << "OFF\n";

    // Count vertices and faces
    size_t vertex_count = mesh.getMesh().number_of_vertices();
    size_t face_count = mesh.getMesh().number_of_faces();

    // Writing number of vertices, faces, and edges
    out << vertex_count << " " << face_count << " 0\n";

    // Writing vertices
    for (auto v : mesh.getMesh().vertices()) {
        auto point = mesh.getMesh().point(v);
        out << point.x() << " " << point.y() << " " << point.z() << "\n";
    }

    // Writing faces
    for (auto f : mesh.getMesh().faces()) {
        auto halfedges = mesh.getMesh().halfedges_around_face(mesh.getMesh().halfedge(f));
        std::vector<int> vertices;
        for (auto h : halfedges) {
            vertices.push_back(mesh.getMesh().target(h));
        }
        out << vertices.size();
        for (int v : vertices) {
            out << " " << v;
        }
        out << "\n";
    }

    out.close();
}

void GALIB minecraft::cgal_support::writeToOff(const GALIB_STD vector<LtSurfaceMesh>& meshes, const char* const p_filename) {
    for (size_t i = 0; i < meshes.size(); ++i) {
        // Generate a unique filename for each mesh
        std::string mesh_filename = std::string(p_filename) + "_" + std::to_string(i) + ".off";
        writeMeshToOff(meshes[i], mesh_filename.c_str());
        std::cout << "Written mesh " << i << " to " << mesh_filename << std::endl;
    }
}