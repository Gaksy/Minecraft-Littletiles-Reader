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

#include <cinttypes>
#include <algorithm>

#include "GalibNamespaceDef.h"
#include "Minecraft/LittleTilesCoord.h"
#include "Minecraft/LittleTiles.h"
#include "Exception/LittleTilesException.h"

using GALIB_STD any_of;
using GALIB_STD begin;
using GALIB_STD end;

using GALIB minecraft::littletiles::LittleTilesCoord;
using GALIB minecraft::littletiles::AngleOffset;
using GALIB minecraft::littletiles::AngleID;



bool GALIB minecraft::littletiles::TileEntity::hasAnyOffsetEnable() const  {
    return any_of(
        begin(offset_data),
        end(offset_data),
        [](const AngleOffset& data){return data.hasAnyEnable();}
    );
}

LittleTilesCoord galib::minecraft::littletiles::TileEntity::applyAngleOffset(AngleID angle_id) const {
    LittleTilesCoord angle_coord = getVertices(angle_id);
    const AngleOffset& angle_offset = offset_data[static_cast<size_t>(angle_id)];
    // If unoffset, then offset is 0
    angle_coord.x += angle_offset.x_offset;
    angle_coord.y += angle_offset.y_offset;
    angle_coord.z += angle_offset.z_offset;
    return angle_coord;
}

LittleTilesCoord galib::minecraft::littletiles::TileEntity::getVertices(const AngleID kAngleId) const {
    switch (kAngleId) {
        case AngleID::WDS:
            return {pos_1.x, pos_1.y, pos_2.z};
        case AngleID::WDN:
            return {pos_1.x, pos_1.y, pos_1.z};
        case AngleID::EDN:
            return {pos_2.x, pos_1.y, pos_1.z};
        case AngleID::EDS:
            return {pos_2.x, pos_1.y, pos_2.z};
        case AngleID::WUN:
            return {pos_1.x, pos_2.y, pos_1.z};
        case AngleID::WUS:
            return {pos_1.x, pos_2.y, pos_2.z};
        case AngleID::EUS:
            return {pos_2.x, pos_2.y, pos_2.z};
        case AngleID::EUN:
            return {pos_2.x, pos_2.y, pos_1.z};
        default:
            throw exception::LittleTilesException(exception::LittleTilesErrorCode::lt_unknow_angle);
    }
}

galib::minecraft::littletiles::TileFace galib::minecraft::littletiles::TileEntity::getTileFace(
    const TileFaceID kTileFaceID,
    const bool kWithOffst
    ) const
{
    switch (kTileFaceID) {
        case TileFaceID::EAST:
            return {
                getVertices(AngleID::EUN, kWithOffst),
                getVertices(AngleID::EUS, kWithOffst),
                getVertices(AngleID::EDS, kWithOffst),
                getVertices(AngleID::EDN, kWithOffst)
            };
        case TileFaceID::WEST:
            return {
                getVertices(AngleID::WUN, kWithOffst),
                getVertices(AngleID::WUS, kWithOffst),
                getVertices(AngleID::WDS, kWithOffst),
                getVertices(AngleID::WDN, kWithOffst)
            };
        case TileFaceID::SOUTH:
            return {
                getVertices(AngleID::EUS, kWithOffst),
                getVertices(AngleID::WUS, kWithOffst),
                getVertices(AngleID::WDS, kWithOffst),
                getVertices(AngleID::EDS, kWithOffst)
            };
        case TileFaceID::NORTH:
            return {
                getVertices(AngleID::EUN, kWithOffst),
                getVertices(AngleID::EUS, kWithOffst),
                getVertices(AngleID::WUN, kWithOffst),
                getVertices(AngleID::WUS, kWithOffst)
            };
        case TileFaceID::UP:
            return {
                getVertices(AngleID::EUN, kWithOffst),
                getVertices(AngleID::EUS, kWithOffst),
                getVertices(AngleID::WUS, kWithOffst),
                getVertices(AngleID::WUN, kWithOffst)
            };
        case TileFaceID::DOWN:
            return {
                getVertices(AngleID::EDN, kWithOffst),
                getVertices(AngleID::EDS, kWithOffst),
                getVertices(AngleID::WDS, kWithOffst),
                getVertices(AngleID::WDN, kWithOffst)
            };
        default:
            throw exception::LittleTilesException(exception::LittleTilesErrorCode::lt_unknow_face);
    }
}