
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

using GALIB minecraft::littletiles::LittleTilesCoord;
using GALIB minecraft::littletiles::TileEntity;
using GALIB minecraft::littletiles::TileFaceID;
using GALIB minecraft::littletiles::TileFace;
using GALIB minecraft::littletiles::AngleID;
using GALIB minecraft::littletiles::TileFaceID;

void GALIB minecraft::cgal_support::createMeshFromTileEntity(LtMesh& mesh, const TileEntity& tileEntity) {
    const LtMesh::Vertex_index eun = mesh.add_vertex(convertToCGALPoint(tileEntity.getVertices(AngleID::EUN, true)));
    const LtMesh::Vertex_index eus = mesh.add_vertex(convertToCGALPoint(tileEntity.getVertices(AngleID::EUS, true)));
    const LtMesh::Vertex_index edn = mesh.add_vertex(convertToCGALPoint(tileEntity.getVertices(AngleID::EDN, true)));
    const LtMesh::Vertex_index eds = mesh.add_vertex(convertToCGALPoint(tileEntity.getVertices(AngleID::EDS, true)));
    const LtMesh::Vertex_index wun = mesh.add_vertex(convertToCGALPoint(tileEntity.getVertices(AngleID::WUN, true)));
    const LtMesh::Vertex_index wus = mesh.add_vertex(convertToCGALPoint(tileEntity.getVertices(AngleID::WUS, true)));
    const LtMesh::Vertex_index wdn = mesh.add_vertex(convertToCGALPoint(tileEntity.getVertices(AngleID::WDN, true)));
    const LtMesh::Vertex_index wds = mesh.add_vertex(convertToCGALPoint(tileEntity.getVertices(AngleID::WDS, true)));

    const bool east_flipped = tileEntity.flipped_data.east;
    const bool west_flipped = tileEntity.flipped_data.west;
    const bool south_flipped = tileEntity.flipped_data.south;
    const bool north_flipped = tileEntity.flipped_data.north;
    const bool up_flipped = tileEntity.flipped_data.up;
    const bool down_flipped = tileEntity.flipped_data.down;

    if (east_flipped) {
        mesh.add_face(eun, eus, eds);
        mesh.add_face(eun, eds, edn);
    } else {
        mesh.add_face(eun, edn, eds);
        mesh.add_face(eun, eds, eus);
    }

    if (west_flipped) {
        mesh.add_face(wun, wus, wds);
        mesh.add_face(wun, wds, wdn);
    } else {
        mesh.add_face(wun, wdn, wds);
        mesh.add_face(wun, wds, wus);
    }

    if (south_flipped) {
        mesh.add_face(wun, wus, eun);
        mesh.add_face(wun, eun, eus);
    } else {
        mesh.add_face(wun, eun, eus);
        mesh.add_face(wun, eus, wus);
    }

    if (north_flipped) {
        mesh.add_face(wdn, wds, edn);
        mesh.add_face(wdn, edn, eds);
    } else {
        mesh.add_face(wdn, edn, eds);
        mesh.add_face(wdn, eds, wds);
    }

    if (up_flipped) {
        mesh.add_face(wus, eun, eun);
        mesh.add_face(wus, eun, eun);
    } else {
        mesh.add_face(wus, eun, eun);
        mesh.add_face(wus, eun, wus);
    }

    if (down_flipped) {
        mesh.add_face(wdn, wds, edn);
        mesh.add_face(wdn, edn, eds);
    } else {
        mesh.add_face(wdn, edn, eds);
        mesh.add_face(wdn, eds, wds);
    }
}


