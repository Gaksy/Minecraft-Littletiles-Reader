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

#include "Minecraft/CgalSupport/CgalLtSupport.h"

#include <CGAL/Polygon_mesh_processing/repair.h>

#include "GalibNamespaceDef.h"
#include "Minecraft/CgalSupport/CgalTypeDef.h"
#include "Minecraft/LittleTiles.h"

using GALIB minecraft::littletiles::LittleTilesCoord;
using GALIB minecraft::littletiles::GridType;

using GALIB minecraft::littletiles::AngleID;
using GALIB minecraft::littletiles::TileFaceID;
using GALIB minecraft::littletiles::Flipped;
using GALIB minecraft::littletiles::TileEntity;
using GALIB minecraft::littletiles::TileFace;

using GALIB minecraft::cgal_support::LtMesh;

using GALIB_CGAL SM_Vertex_index;

void GALIB minecraft::cgal_support::createMeshFromTileEntity(
    LtMesh& mesh,
    const TileEntity& kTileEntity,
    const bool kApplyOffset
) {
    using VertexIndex = LtMesh::Vertex_index;

    const VertexIndex eun = mesh.add_vertex(
        convertToCGALPoint(kTileEntity.getVertices(AngleID::EUN, kApplyOffset))
    );

    const VertexIndex eus = mesh.add_vertex(
        convertToCGALPoint(kTileEntity.getVertices(AngleID::EUS, kApplyOffset))
    );

    const VertexIndex edn = mesh.add_vertex(
        convertToCGALPoint(kTileEntity.getVertices(AngleID::EDN, kApplyOffset))
    );

    const VertexIndex eds = mesh.add_vertex(
        convertToCGALPoint(kTileEntity.getVertices(AngleID::EDS, kApplyOffset))
    );

    const VertexIndex wun = mesh.add_vertex(
        convertToCGALPoint(kTileEntity.getVertices(AngleID::WUN, kApplyOffset))
    );

    const VertexIndex wus = mesh.add_vertex(
        convertToCGALPoint(kTileEntity.getVertices(AngleID::WUS, kApplyOffset))
    );

    const VertexIndex wdn = mesh.add_vertex(
        convertToCGALPoint(kTileEntity.getVertices(AngleID::WDN, kApplyOffset))
    );

    const VertexIndex wds = mesh.add_vertex(
        convertToCGALPoint(kTileEntity.getVertices(AngleID::WDS, kApplyOffset))
    );

    const Flipped& flipped_data = kTileEntity.getFlippedData();
    const bool east_flipped = flipped_data.east;
    const bool west_flipped = flipped_data.west;
    const bool south_flipped = flipped_data.south;
    const bool north_flipped = flipped_data.north;
    const bool up_flipped = flipped_data.up;
    const bool down_flipped = flipped_data.down;

    // East face (flipped)
    if (!east_flipped) {
        mesh.add_face(eds, edn, eus);
        mesh.add_face(eus, edn, eun);
    } else {
        mesh.add_face(eds, eun, eus);
        mesh.add_face(eds, edn, eun);
    }

    // West face (flipped)
    if (!west_flipped) {
        mesh.add_face(wdn, wds, wun);
        mesh.add_face(wun, wds, wus);
    } else {
        mesh.add_face(wdn, wus, wun);
        mesh.add_face(wdn, wds, wus);
    }

    // South face (flipped)
    if (!south_flipped) {
        mesh.add_face(wds, eds, wus);
        mesh.add_face(wus, eds, eus);
    } else {
        mesh.add_face(wds, eus, wus);
        mesh.add_face(wds, eds, eus);
    }

    // North face (flipped)
    if (!north_flipped) {
        mesh.add_face(edn, wdn, eun);
        mesh.add_face(eun, wdn, wun);
    } else {
        mesh.add_face(edn, wun, eun);
        mesh.add_face(edn, wdn, wun);
    }

    // Up face (flipped)
    if (!up_flipped) {
        mesh.add_face(wus, eus, wun);
        mesh.add_face(wun, eus, eun);
    } else {
        mesh.add_face(wus, eun, wun);
        mesh.add_face(wus, eus, eun);
    }

    // Down face (flipped)
    if (!down_flipped) {
        mesh.add_face(wdn, edn, wds);
        mesh.add_face(wds, edn, eds);
    } else {
        mesh.add_face(wdn, eds, wds);
        mesh.add_face(wdn, edn, eds);
    }
}

const LtMesh& GALIB minecraft::cgal_support::createIntersectionCube(const GridType kGrid) {
    // 静态指针，确保只在第一次调用时创建
    static LtMesh* cube = nullptr;

    // 如果cube尚未创建，则构建它
    if (!cube) {
        cube = new LtMesh();

        // 构建正方体顶点 p1(0, 0, 0) 和 p2(1, 1, 1)
        using Point = LtMesh::Point;

        // 顶点坐标
        const SM_Vertex_index eun = cube->add_vertex(Point(kGrid, kGrid, 0));
        const SM_Vertex_index eus = cube->add_vertex(Point(kGrid, kGrid, kGrid));
        const SM_Vertex_index edn = cube->add_vertex(Point(kGrid, 0, 0));
        const SM_Vertex_index eds = cube->add_vertex(Point(kGrid, 0, kGrid));
        const SM_Vertex_index wun = cube->add_vertex(Point(0, kGrid, 0));
        const SM_Vertex_index wus = cube->add_vertex(Point(0, kGrid, kGrid));
        const SM_Vertex_index wdn = cube->add_vertex(Point(0, 0, 0));
        const SM_Vertex_index wds = cube->add_vertex(Point(0, 0, kGrid));

        // 创建正方体的面（六个面，每个面由两个三角形组成）
        cube->add_face(eds, eun, eus);
        cube->add_face(eds, edn, eun);
        cube->add_face(wdn, wus, wun);
        cube->add_face(wdn, wds, wus);
        cube->add_face(wds, eus, wus);
        cube->add_face(wds, eds, eus);
        cube->add_face(edn, wun, eun);
        cube->add_face(edn, wdn, wun);
        cube->add_face(wus, eun, wun);
        cube->add_face(wus, eus, eun);
        cube->add_face(wdn, eds, wds);
        cube->add_face(wdn, edn, eds);
    }

    return *cube;  // 返回静态指针
}

const LtMesh (GALIB minecraft::cgal_support::applyWorldOffset)(LtMesh& mesh, const galib::minecraft::BlockCoordinate & block_coordinate) {
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

const LtMesh (GALIB minecraft::cgal_support::applyGrid)(const LtMesh& mesh, const GridType grid) {
    LtMesh transformed = mesh;
    using Point = LtMesh::Point;

    for(auto v : transformed.vertices()) {
        Point p = transformed.point(v);
        transformed.point(v) = Point(
            p.x() / static_cast<float>(grid),
            p.y() / static_cast<float>(grid),
            p.z() / static_cast<float>(grid)
        );
    }
    return transformed;
}

// 网格清理函数
void (GALIB minecraft::cgal_support::cleanupMesh)(LtMesh& mesh) {
    // 移除退化面
    CGAL::Polygon_mesh_processing::remove_degenerate_faces(mesh);

    // 移除孤立顶点
    CGAL::Polygon_mesh_processing::remove_isolated_vertices(mesh);

    // 确保网格是流形的
    if (!is_valid_polygon_mesh(mesh)) {
        std::cerr << "Warning: Mesh is not valid after cleanup" << std::endl;
    }
}
