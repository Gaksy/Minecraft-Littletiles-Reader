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
 * Date Created: 09/11/2026
 */

#include "Minecraft/TextureSupport/BlockTextureTable.h"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>

#include "File/Utf8Path.h"

namespace galib::minecraft::texture_support {

namespace {

// Column order of the table (matching FACES in the generator side's
// tools/resolve_block_textures.py)
constexpr FaceDirection kColumnOrder[kFaceCount] = {
    FaceDirection::kDown,  FaceDirection::kUp,   FaceDirection::kNorth,
    FaceDirection::kSouth, FaceDirection::kWest, FaceDirection::kEast,
};

std::vector<std::string> SplitTab(const std::string& kLine) {
  std::vector<std::string> fields;
  std::string field;
  std::istringstream stream(kLine);
  while (std::getline(stream, field, '\t')) {
    fields.push_back(field);
  }
  return fields;
}

}  // namespace

std::string StripBlockMeta(const std::string& kBlockId) {
  const std::size_t first = kBlockId.find(':');
  if (first == std::string::npos) {
    return kBlockId;
  }
  const std::size_t second = kBlockId.find(':', first + 1);
  return second == std::string::npos ? kBlockId : kBlockId.substr(0, second);
}

bool BlockFaceTextures::empty() const {
  for (const std::string& path : paths) {
    if (!path.empty() && path != "-") {
      return false;
    }
  }
  return true;
}

const std::string& BlockFaceTextures::Path(
    const FaceDirection direction) const {
  return paths[static_cast<std::size_t>(direction)];
}

int BlockFaceTextures::Tint(const FaceDirection direction) const {
  return tints[static_cast<std::size_t>(direction)];
}

bool BlockTextureTable::LoadFromTsv(const std::string& kTsvPath) {
  entries_.clear();
  loaded_ = false;

  // UTF-8 path: the table may sit in a folder the user named.
  std::ifstream input(galib::Utf8Path(kTsvPath));
  if (!input) {
    return false;
  }

  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }
    const std::vector<std::string> fields = SplitTab(line);
    if (fields.size() < kFaceCount + 1) {
      continue;
    }
    BlockFaceTextures textures;
    for (int i = 0; i < kFaceCount; ++i) {
      const std::string& value = fields[static_cast<std::size_t>(i) + 1];
      textures.paths[static_cast<std::size_t>(kColumnOrder[i])] =
          value == "-" ? "" : value;
    }
    // The tintindex columns are optional: older tables only have 7 columns, in
    // which case everything is treated as untinted
    for (int i = 0; i < kFaceCount; ++i) {
      const std::size_t column = static_cast<std::size_t>(i) + 1 + kFaceCount;
      int tint = -1;
      if (column < fields.size() && !fields[column].empty()) {
        tint = std::atoi(fields[column].c_str());
      }
      textures.tints[static_cast<std::size_t>(kColumnOrder[i])] = tint;
    }
    entries_[fields[0]] = std::move(textures);
  }

  loaded_ = !entries_.empty();
  return loaded_;
}

bool BlockTextureTable::Lookup(const std::string& kBlockId,
                               BlockFaceTextures* const p_desc_textures) const {
  if (!p_desc_textures) {
    return false;
  }

  auto found = entries_.find(kBlockId);
  if (found == entries_.end()) {
    // The block name in NBT may carry no meta. Blocks with a meta family
    // (wool, stained glass, ...) only have "<name>:<meta>" keys in the table,
    // while blockstate files are named by colour (white_stained_glass etc.), so
    // one extra ":0" attempt is made here.
    if (kBlockId.find(':') != std::string::npos &&
        kBlockId.find(':', kBlockId.find(':') + 1) == std::string::npos) {
      found = entries_.find(kBlockId + ":0");
    }
  }
  if (found == entries_.end()) {
    // When the table has no meta-qualified key, fall back to the block's base name
    found = entries_.find(StripBlockMeta(kBlockId));
  }
  if (found == entries_.end()) {
    return false;
  }

  *p_desc_textures = found->second;
  return true;
}

void ComputeFaceUv(const FaceDirection direction, const double x,
                   const double y, const double z, double* const p_desc_u,
                   double* const p_desc_v) {
  // The basis for the table below is documented in docs/texture-mapping.md: the
  // u/v axes and direction of each face in the MC model specification.
  double u = 0.0;
  double v = 0.0;
  switch (direction) {
    case FaceDirection::kUp:
      u = x;
      v = z;
      break;
    case FaceDirection::kDown:
      u = 1.0 - x;
      v = z;
      break;
    case FaceDirection::kEast:
      u = 1.0 - z;
      v = 1.0 - y;
      break;
    case FaceDirection::kWest:
      u = z;
      v = 1.0 - y;
      break;
    case FaceDirection::kNorth:
      u = 1.0 - x;
      v = 1.0 - y;
      break;
    case FaceDirection::kSouth:
      u = x;
      v = 1.0 - y;
      break;
  }
  if (p_desc_u) {
    *p_desc_u = u;
  }
  if (p_desc_v) {
    *p_desc_v = v;
  }
}

FaceDirection FaceDirectionFromNormal(const double nx, const double ny,
                                      const double nz) {
  const double ax = nx < 0 ? -nx : nx;
  const double ay = ny < 0 ? -ny : ny;
  const double az = nz < 0 ? -nz : nz;

  if (ax >= ay && ax >= az) {
    return nx >= 0 ? FaceDirection::kEast : FaceDirection::kWest;
  }
  if (ay >= az) {
    return ny >= 0 ? FaceDirection::kUp : FaceDirection::kDown;
  }
  return nz >= 0 ? FaceDirection::kSouth : FaceDirection::kNorth;
}

const char* FaceDirectionName(const FaceDirection direction) {
  switch (direction) {
    case FaceDirection::kDown:
      return "down";
    case FaceDirection::kUp:
      return "up";
    case FaceDirection::kNorth:
      return "north";
    case FaceDirection::kSouth:
      return "south";
    case FaceDirection::kWest:
      return "west";
    case FaceDirection::kEast:
      return "east";
  }
  return "unknown";
}

}  // namespace galib::minecraft::texture_support
