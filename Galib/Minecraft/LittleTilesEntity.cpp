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

#include <algorithm>
#include <cinttypes>

#include "Exception/LittleTilesException.h"
#include "GalibNamespaceDef.h"
#include "Minecraft/LittleTiles.h"
#include "Minecraft/LittleTilesCoord.h"

using std::any_of;
using std::begin;
using std::end;

using galib::minecraft::littletiles::AngleID;
using galib::minecraft::littletiles::AngleOffset;
using galib::minecraft::littletiles::LittleTilesCoord;
using galib::minecraft::littletiles::TileEntity;
using galib::minecraft::littletiles::TileFace;

TileEntity::TileEntity() : pos_1_({0, 0, 0}), pos_2_({0, 0, 0}) { ; }

bool TileEntity::has_any_offset_enable() const {
  return any_of(begin(offset_data_), end(offset_data_),
                [](const AngleOffset& data) { return data.has_any_enable(); });
}

LittleTilesCoord TileEntity::ApplyAngleOffset(const AngleID kAngleId) const {
  LittleTilesCoord angle_coord = GetVertices(kAngleId);
  const AngleOffset& angle_offset = offset_data_[ConvertAngleIdToInt(kAngleId)];
  // If unoffset, then offset is 0
  angle_coord.x += angle_offset.x_offset;
  angle_coord.y += angle_offset.y_offset;
  angle_coord.z += angle_offset.z_offset;
  return angle_coord;
}

AngleOffset TileEntity::GetAngleOffset(const AngleID kAngleID) const {
  return offset_data_[ConvertAngleIdToInt(kAngleID)];
}

LittleTilesCoord TileEntity::GetVertices(const AngleID kAngleId) const {
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
      throw exception::LittleTilesException(
          exception::LittleTilesErrorCode::lt_unknow_angle);
  }
}

LittleTilesCoord TileEntity::GetVertices(const AngleID kAngleId,
                                         const bool kWithOffset) const {
  if (kWithOffset && has_any_offset_enable()) {
    return ApplyAngleOffset(kAngleId);
  }
  return GetVertices(kAngleId);
}

LittleTilesCoord TileEntity::GetVerticesApplyGrid(AngleID kAngleId,
                                                  GridType kGridType,
                                                  bool kWithOffset) const {
  LittleTilesCoord temp = GetVertices(kAngleId, kWithOffset);
  temp.x /= kGridType;
  temp.y /= kGridType;
  temp.z /= kGridType;
  return temp;
}

TileFace TileEntity::GetTileFace(const TileFaceID kTileFaceID,
                                 const bool kWithOffset) const {
  switch (kTileFaceID) {
    case TileFaceID::EAST:
      return {GetVertices(AngleID::EUN, kWithOffset),
              GetVertices(AngleID::EUS, kWithOffset),
              GetVertices(AngleID::EDS, kWithOffset),
              GetVertices(AngleID::EDN, kWithOffset)};
    case TileFaceID::WEST:
      return {GetVertices(AngleID::WUN, kWithOffset),
              GetVertices(AngleID::WUS, kWithOffset),
              GetVertices(AngleID::WDS, kWithOffset),
              GetVertices(AngleID::WDN, kWithOffset)};
    case TileFaceID::SOUTH:
      return {GetVertices(AngleID::EUS, kWithOffset),
              GetVertices(AngleID::WUS, kWithOffset),
              GetVertices(AngleID::WDS, kWithOffset),
              GetVertices(AngleID::EDS, kWithOffset)};
    case TileFaceID::NORTH:
      return {GetVertices(AngleID::EUN, kWithOffset),
              GetVertices(AngleID::EUS, kWithOffset),
              GetVertices(AngleID::WUN, kWithOffset),
              GetVertices(AngleID::WUS, kWithOffset)};
    case TileFaceID::UP:
      return {GetVertices(AngleID::EUN, kWithOffset),
              GetVertices(AngleID::EUS, kWithOffset),
              GetVertices(AngleID::WUS, kWithOffset),
              GetVertices(AngleID::WUN, kWithOffset)};
    case TileFaceID::DOWN:
      return {GetVertices(AngleID::EDN, kWithOffset),
              GetVertices(AngleID::EDS, kWithOffset),
              GetVertices(AngleID::WDS, kWithOffset),
              GetVertices(AngleID::WDN, kWithOffset)};
    default:
      throw exception::LittleTilesException(
          exception::LittleTilesErrorCode::lt_unknow_face);
  }
}

const galib::minecraft::littletiles::Flipped& TileEntity::flipped_data() const {
  return flipped_data_;
}

bool TileEntity::has_color() const { return has_color_; }

std::int32_t TileEntity::color() const { return color_; }

const LittleTilesCoord& TileEntity::pos_1() const { return pos_1_; }

const LittleTilesCoord& TileEntity::pos_2() const { return pos_2_; }

void TileEntity::set_pos(const LittleTilesCoord& kPos1,
                         const LittleTilesCoord& kPos2) {
  pos_1_ = kPos1;
  pos_2_ = kPos2;
}

void TileEntity::set_flipped_data(const Flipped& kFlippedData) {
  flipped_data_ = kFlippedData;
}

void TileEntity::set_color(const std::int32_t kColor, const bool kHasColor) {
  color_ = kColor;
  has_color_ = kHasColor;
}

void TileEntity::set_offset_data(const AngleID kAngleId,
                                 const AngleOffset& kAngleOffsetData) {
  offset_data_[ConvertAngleIdToInt(kAngleId)] = kAngleOffsetData;
}

void TileEntity::set_offset_data(const AngleOffset kOffsetData[8]) {
  offset_data_[0] = kOffsetData[0];
  offset_data_[1] = kOffsetData[1];
  offset_data_[2] = kOffsetData[2];
  offset_data_[3] = kOffsetData[3];
  offset_data_[4] = kOffsetData[4];
  offset_data_[5] = kOffsetData[5];
  offset_data_[6] = kOffsetData[6];
  offset_data_[7] = kOffsetData[7];
}

bool TileEntity::is_offset_off_boundary() const {
  if (has_any_offset_enable()) {
    const LittleTilesCoord::NumericType x_min = pos_1_.x;
    const LittleTilesCoord::NumericType x_max = pos_2_.x;

    const LittleTilesCoord::NumericType y_min = pos_1_.y;
    const LittleTilesCoord::NumericType y_max = pos_2_.y;

    const LittleTilesCoord::NumericType z_min = pos_1_.z;
    const LittleTilesCoord::NumericType z_max = pos_2_.z;

    for (size_t index = 0; index < 8; ++index) {
      const LittleTilesCoord current_coord =
          GetVertices(static_cast<AngleID>(index), true);
      if (!((current_coord.x >= x_min && current_coord.x <= x_max) &&
            (current_coord.y >= y_min && current_coord.y <= y_max) &&
            (current_coord.z >= z_min && current_coord.z <= z_max))) {
        return true;
      }
    }
  }
  return false;
}
