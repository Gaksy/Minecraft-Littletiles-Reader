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
using GALIB minecraft::littletiles::TileFace;
using GALIB minecraft::littletiles::TileEntity;

TileEntity::TileEntity():
    pos_1_({0,0,0}),
    pos_2_({0,0,0})
{ ; }

bool TileEntity::hasAnyOffsetEnable() const {
    return any_of(
        begin(offset_data_),
        end(offset_data_),
        [](const AngleOffset& data){return data.hasAnyEnable();}
    );
}

LittleTilesCoord TileEntity::applyAngleOffset(const AngleID kAngleId) const {
    LittleTilesCoord angle_coord = getVertices(kAngleId);
    const AngleOffset& angle_offset = offset_data_[convertAngleIdToInt(kAngleId)];
    // If unoffset, then offset is 0
    angle_coord.x += angle_offset.x_offset;
    angle_coord.y += angle_offset.y_offset;
    angle_coord.z += angle_offset.z_offset;
    return angle_coord;
}

AngleOffset TileEntity::getAngleOffset(const AngleID kAngleID) const {
    return offset_data_[convertAngleIdToInt(kAngleID)];
}

LittleTilesCoord TileEntity::getVertices(const AngleID kAngleId) const {
    switch (kAngleId) {
        case AngleID::WDS:
            return {pos_1_.x, pos_1_.y, pos_2_.z};
        case AngleID::WDN:
            return {pos_1_.x, pos_1_.y, pos_1_.z};
        case AngleID::EDN:
            return {pos_2_.x, pos_1_.y, pos_1_.z};
        case AngleID::EDS:
            return {pos_2_.x, pos_1_.y, pos_2_.z};
        case AngleID::WUN:
            return {pos_1_.x, pos_2_.y, pos_1_.z};
        case AngleID::WUS:
            return {pos_1_.x, pos_2_.y, pos_2_.z};
        case AngleID::EUS:
            return {pos_2_.x, pos_2_.y, pos_2_.z};
        case AngleID::EUN:
            return {pos_2_.x, pos_2_.y, pos_1_.z};
        default:
            throw exception::LittleTilesException(exception::LittleTilesErrorCode::lt_unknow_angle);
    }
}

LittleTilesCoord TileEntity::getVertices(
    const AngleID kAngleId,
    const bool kWithOffset
) const {
    if (kWithOffset && hasAnyOffsetEnable()) { return applyAngleOffset(kAngleId); }
    return getVertices(kAngleId);
}

LittleTilesCoord TileEntity::getVerticesApplyGrid(AngleID kAngleId, GridType kGridType, bool kWithOffset) const {
    LittleTilesCoord temp = getVertices(kAngleId, kWithOffset);
    temp.x /= kGridType;
    temp.y /= kGridType;
    temp.z /= kGridType;
    return temp;
}

TileFace TileEntity::getTileFace(const TileFaceID kTileFaceID, const bool kWithOffset) const {
     switch (kTileFaceID) {
     case TileFaceID::EAST:
         return {
             getVertices(AngleID::EUN, kWithOffset),
             getVertices(AngleID::EUS, kWithOffset),
             getVertices(AngleID::EDS, kWithOffset),
             getVertices(AngleID::EDN, kWithOffset)
         };
     case TileFaceID::WEST:
         return {
             getVertices(AngleID::WUN, kWithOffset),
             getVertices(AngleID::WUS, kWithOffset),
             getVertices(AngleID::WDS, kWithOffset),
             getVertices(AngleID::WDN, kWithOffset)
         };
     case TileFaceID::SOUTH:
         return {
             getVertices(AngleID::EUS, kWithOffset),
             getVertices(AngleID::WUS, kWithOffset),
             getVertices(AngleID::WDS, kWithOffset),
             getVertices(AngleID::EDS, kWithOffset)
         };
     case TileFaceID::NORTH:
         return {
             getVertices(AngleID::EUN, kWithOffset),
             getVertices(AngleID::EUS, kWithOffset),
             getVertices(AngleID::WUN, kWithOffset),
             getVertices(AngleID::WUS, kWithOffset)
         };
     case TileFaceID::UP:
         return {
             getVertices(AngleID::EUN, kWithOffset),
             getVertices(AngleID::EUS, kWithOffset),
             getVertices(AngleID::WUS, kWithOffset),
             getVertices(AngleID::WUN, kWithOffset)
         };
     case TileFaceID::DOWN:
         return {
             getVertices(AngleID::EDN, kWithOffset),
             getVertices(AngleID::EDS, kWithOffset),
             getVertices(AngleID::WDS, kWithOffset),
             getVertices(AngleID::WDN, kWithOffset)
         };
     default:
         throw exception::LittleTilesException(exception::LittleTilesErrorCode::lt_unknow_face);
 }
}

const galib::minecraft::littletiles::Flipped & TileEntity::getFlippedData()const {
    return flipped_data_;
}

void TileEntity::setPos(const LittleTilesCoord &kPos1, const LittleTilesCoord &kPos2) {
    pos_1_ = kPos1;
    pos_2_ = kPos2;
}

void TileEntity::setFlippedData(const Flipped &kFlippedData) {
    flipped_data_ = kFlippedData;
}

void TileEntity::setOffsetData(const AngleID kAngleId, const AngleOffset &kAngleOffsetData) {
    offset_data_[convertAngleIdToInt(kAngleId)] = kAngleOffsetData;
}

void TileEntity::setOffsetData(const AngleOffset kOffsetData[8]) {
    offset_data_[0] = kOffsetData[0];
    offset_data_[1] = kOffsetData[1];
    offset_data_[2] = kOffsetData[2];
    offset_data_[3] = kOffsetData[3];
    offset_data_[4] = kOffsetData[4];
    offset_data_[5] = kOffsetData[5];
    offset_data_[6] = kOffsetData[6];
    offset_data_[7] = kOffsetData[7];
}

bool TileEntity::isOffsetOffBoundary()const {
    if (hasAnyOffsetEnable()) {
        const LittleTilesCoord::NumericType x_min = pos_1_.x;
        const LittleTilesCoord::NumericType x_max = pos_2_.x;

        const LittleTilesCoord::NumericType y_min = pos_1_.y;
        const LittleTilesCoord::NumericType y_max = pos_2_.y;

        const LittleTilesCoord::NumericType z_min = pos_1_.z;
        const LittleTilesCoord::NumericType z_max = pos_2_.z;

        for (size_t index = 0; index < 8; ++index) {
            const LittleTilesCoord current_coord = getVertices(static_cast<AngleID>(index), true);
            if (!((current_coord.x >= x_min && current_coord.x <= x_max) &&
                (current_coord.y >= y_min && current_coord.y <= y_max) &&
                (current_coord.z >= z_min && current_coord.z <= z_max))
            ){
                return true;
            }
        }
    }
    return false;
}