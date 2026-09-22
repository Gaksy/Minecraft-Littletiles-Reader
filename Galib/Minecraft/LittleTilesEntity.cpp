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
#include <vector>

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

std::vector<std::int32_t> galib::minecraft::littletiles::EncodeBoxArray(
    const TileEntity& kTile) {
  std::vector<std::int32_t> array;
  array.reserve(6 + 1 + 8 * 3 / 2);
  array.push_back(static_cast<std::int32_t>(kTile.pos_1().x));
  array.push_back(static_cast<std::int32_t>(kTile.pos_1().y));
  array.push_back(static_cast<std::int32_t>(kTile.pos_1().z));
  array.push_back(static_cast<std::int32_t>(kTile.pos_2().x));
  array.push_back(static_cast<std::int32_t>(kTile.pos_2().y));
  array.push_back(static_cast<std::int32_t>(kTile.pos_2().z));

  const Flipped& flipped = kTile.flipped_data();
  std::uint32_t indicator = 0;
  // Flip bits 24..29, in Facing order (down, up, north, south, west, east), plus
  // LittleTiles' sign marker in bit 31 so that LittleBox.create recognises the
  // array as a transformable box.
  if (flipped.down) {
    indicator |= 0x1u << 24;
  }
  if (flipped.up) {
    indicator |= 0x2u << 24;
  }
  if (flipped.north) {
    indicator |= 0x4u << 24;
  }
  if (flipped.south) {
    indicator |= 0x1u << 27;
  }
  if (flipped.west) {
    indicator |= 0x2u << 27;
  }
  if (flipped.east) {
    indicator |= 0x4u << 27;
  }

  // Offsets, in the order the decoder consumes them: corner (AngleID) ascending,
  // and inside a corner x -> y -> z. They are packed two per int, the first value
  // in the high 16 bits.
  std::vector<std::int32_t> packed_values;
  for (std::size_t corner = 0; corner < 8; ++corner) {
    const AngleOffset offset =
        kTile.GetAngleOffset(static_cast<AngleID>(corner));
    const std::uint8_t bits = static_cast<std::uint8_t>(corner * 3);
    if (offset.x_enable) {
      indicator |= 0x1u << bits;
      packed_values.push_back(offset.x_offset);
    }
    if (offset.y_enable) {
      indicator |= 0x2u << bits;
      packed_values.push_back(offset.y_offset);
    }
    if (offset.z_enable) {
      indicator |= 0x4u << bits;
      packed_values.push_back(offset.z_offset);
    }
  }

  if (packed_values.empty() && indicator == 0) {
    // A plain box: no angle data at all.
    return array;
  }
  // Both layouts start with the indicator (flip bits only: an array of 7
  // entries; with offsets: the packed values follow), and both carry the sign
  // marker in bit 31, so that reading the file back recognises the box as
  // transformable and keeps the flip bits.
  indicator |= 0x80000000u;
  array.push_back(static_cast<std::int32_t>(indicator));
  for (std::size_t index = 0; index < packed_values.size(); index += 2) {
    const std::uint32_t high =
        static_cast<std::uint32_t>(
            static_cast<std::uint16_t>(packed_values[index]))
        << 16;
    const std::uint32_t low =
        index + 1 < packed_values.size()
            ? static_cast<std::uint32_t>(
                  static_cast<std::uint16_t>(packed_values[index + 1]))
            : 0u;
    array.push_back(static_cast<std::int32_t>(high | low));
  }
  return array;
}
