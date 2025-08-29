
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
#include "GalibNamespaceDef.h"
#include "Minecraft/CgalSupport/CgalTypeDef.h"
#include "Minecraft/LittleTiles.h"
#include <CGAL/Polygon_mesh_processing/stitch_borders.h>
#include <CGAL/Polygon_mesh_processing/repair.h>

using GALIB minecraft::littletiles::LittleTilesCoord;
using GALIB minecraft::littletiles::GridType;

using GALIB minecraft::littletiles::AngleID;
using GALIB minecraft::littletiles::TileFaceID;
using GALIB minecraft::littletiles::Flipped;
using GALIB minecraft::littletiles::TileEntity;
using GALIB minecraft::littletiles::TileFace;

using GALIB minecraft::cgal_support::LtMesh;

void GALIB minecraft::cgal_support::createMeshFromTileEntity(
    LtMesh& mesh,
    const TileEntity& tileEntity,
    const GridType kGridType
) {
    using VertexIndex = LtMesh::Vertex_index;

    const VertexIndex eun = mesh.add_vertex(
        convertToCGALPoint(tileEntity.getVerticesApplyGrid(AngleID::EUN, kGridType, true))
    );

    const VertexIndex eus = mesh.add_vertex(
        convertToCGALPoint(tileEntity.getVerticesApplyGrid(AngleID::EUS, kGridType, true))
    );

    const VertexIndex edn = mesh.add_vertex(
        convertToCGALPoint(tileEntity.getVerticesApplyGrid(AngleID::EDN, kGridType, true))
    );

    const VertexIndex eds = mesh.add_vertex(
        convertToCGALPoint(tileEntity.getVerticesApplyGrid(AngleID::EDS, kGridType, true))
    );

    const VertexIndex wun = mesh.add_vertex(
        convertToCGALPoint(tileEntity.getVerticesApplyGrid(AngleID::WUN, kGridType, true))
    );

    const VertexIndex wus = mesh.add_vertex(
        convertToCGALPoint(tileEntity.getVerticesApplyGrid(AngleID::WUS, kGridType, true))
    );

    const VertexIndex wdn = mesh.add_vertex(
        convertToCGALPoint(tileEntity.getVerticesApplyGrid(AngleID::WDN, kGridType, true))
    );

    const VertexIndex wds = mesh.add_vertex(
        convertToCGALPoint(tileEntity.getVerticesApplyGrid(AngleID::WDS, kGridType, true))
    );

    const Flipped& flipped_data = tileEntity.getFlippedData();
    const bool east_flipped = flipped_data.east;
    const bool west_flipped = flipped_data.west;
    const bool south_flipped = flipped_data.south;
    const bool north_flipped = flipped_data.north;
    const bool up_flipped = flipped_data.up;
    const bool down_flipped = flipped_data.down;

    // East face (flipped)
    if (east_flipped) {
        mesh.add_face(eus, edn, eds);
        mesh.add_face(eus, eun, edn);
    } else {
        mesh.add_face(eus, eun, eds);
        mesh.add_face(eds, eun, edn);
    }

    // West face (flipped)
    if (west_flipped) {
        mesh.add_face(wun, wds, wdn);
        mesh.add_face(wun, wus, wds);
    } else {
        mesh.add_face(wun, wus, wdn);
        mesh.add_face(wdn, wus, wds);
    }

    // South face (flipped)
    if (south_flipped) {
        mesh.add_face(wus, eds, wds);
        mesh.add_face(wus, eus, eds);
    } else {
        mesh.add_face(wus, eus, wds);
        mesh.add_face(wds, eus, eds);
    }

    // North face (flipped)
    if (north_flipped) {
        mesh.add_face(eun, wdn, edn);
        mesh.add_face(eun, wun, wdn);
    } else {
        mesh.add_face(eun, wun, edn);
        mesh.add_face(edn, wun, wdn);
    }

    // Up face (flipped)
    if (up_flipped) {
        mesh.add_face(wun, eus, wus);
        mesh.add_face(wun, eun, eus);
    } else {
        mesh.add_face(wun, eun, wus);
        mesh.add_face(wus, eun, eus);
    }

    // Down face (flipped)
    if (down_flipped) {
        mesh.add_face(wds, edn, wdn);
        mesh.add_face(wds, eds, edn);
    } else {
        mesh.add_face(wds, eds, wdn);
        mesh.add_face(wdn, eds, edn);
    }


    // Attempt to stitch and repair to ensure mesh is closed
    namespace PMP = CGAL::Polygon_mesh_processing;
    PMP::stitch_borders(mesh);
    PMP::remove_isolated_vertices(mesh);
    PMP::remove_degenerate_faces(mesh);
    // Note: full closure is not guaranteed, but this improves chances
}

LtMesh (GALIB minecraft::cgal_support::createMeshFromTileEntity)(const TileEntity &tileEntity, const GridType kGridType) {
    LtMesh mesh;
    createMeshFromTileEntity(mesh, tileEntity, kGridType);
    return mesh;
}
