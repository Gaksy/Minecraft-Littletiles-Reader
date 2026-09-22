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
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>

#include "File/Utf8Path.h"
#include "Log/GalibLog.h"
#include "Log/GalibText.h"
#include "Minecraft/BlockStateMap.h"

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

// Block names that a pack may still carry in an older spelling than the save /
// structure uses (or the other way around). The packs are built from a 1.12.2
// resource pack, so a few families kept their 1.12 name even though everything
// else is flattened ("silver_concrete" instead of "light_gray_concrete",
// "*_stained_hardened_clay" instead of "*_terracotta", "grass" instead of
// "grass_block", ...). These are only tried **after** the exact lookups, so a
// pack that does have the modern name always wins.
enum class AliasKind { kExact, kPrefix, kSuffix };

struct NameAlias {
  AliasKind kind;
  const char* from;
  const char* to;
};

constexpr NameAlias kNameAliases[] = {
    // 1.13 renamed silver to light_gray; old packs still say silver
    {AliasKind::kPrefix, "minecraft:light_gray_", "minecraft:silver_"},
    // 1.13 renamed *_stained_hardened_clay to *_terracotta
    {AliasKind::kSuffix, "_terracotta", "_stained_hardened_clay"},
    {AliasKind::kExact, "minecraft:terracotta", "minecraft:hardened_clay"},
    {AliasKind::kExact, "minecraft:grass_block", "minecraft:grass"},
    {AliasKind::kExact, "minecraft:dirt_path", "minecraft:grass_path"},
    {AliasKind::kExact, "minecraft:bricks", "minecraft:brick_block"},
    {AliasKind::kExact, "minecraft:nether_bricks", "minecraft:nether_brick"},
    {AliasKind::kExact, "minecraft:red_nether_bricks",
     "minecraft:red_nether_brick"},
    {AliasKind::kExact, "minecraft:end_stone_bricks", "minecraft:end_bricks"},
    {AliasKind::kExact, "minecraft:slime_block", "minecraft:slime"},
    {AliasKind::kExact, "minecraft:melon", "minecraft:melon_block"},
    {AliasKind::kExact, "minecraft:jack_o_lantern", "minecraft:lit_pumpkin"},
    {AliasKind::kExact, "minecraft:snow_block", "minecraft:snow"},
};

// Every way this one name could be spelled in the other era
void AppendAliases(const std::string& kName,
                   std::vector<std::string>* const p_desc_out) {
  for (const NameAlias& alias : kNameAliases) {
    const std::size_t length = std::strlen(alias.from);
    switch (alias.kind) {
      case AliasKind::kExact:
        if (kName == alias.from) {
          p_desc_out->push_back(alias.to);
        }
        break;
      case AliasKind::kPrefix:
        if (kName.size() > length &&
            kName.compare(0, length, alias.from) == 0) {
          p_desc_out->push_back(std::string(alias.to) + kName.substr(length));
        }
        break;
      case AliasKind::kSuffix:
        if (kName.size() > length &&
            kName.compare(kName.size() - length, length, alias.from) == 0) {
          p_desc_out->push_back(kName.substr(0, kName.size() - length) +
                                alias.to);
        }
        break;
    }
  }
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

  // One spelling of the name: the exact key, then "<name>:0" (blocks with a meta
  // family such as wool only have meta-qualified keys, while blockstate files are
  // named by colour), then the meta-less base name.
  const auto try_name = [this,
                         p_desc_textures](const std::string& kName) -> bool {
    auto found = entries_.find(kName);
    if (found == entries_.end()) {
      const std::size_t first = kName.find(':');
      if (first != std::string::npos &&
          kName.find(':', first + 1) == std::string::npos) {
        found = entries_.find(kName + ":0");
      }
    }
    if (found == entries_.end()) {
      found = entries_.find(StripBlockMeta(kName));
    }
    if (found == entries_.end()) {
      return false;
    }
    *p_desc_textures = found->second;
    return true;
  };

  if (try_name(kBlockId)) {
    return true;
  }

  /*
   * Cross-version fallbacks. A save / structure of one generation may name a
   * block differently from the era the pack was built in:
   *
   *   1.20 "minecraft:polished_granite"  <-> 1.12.2 "minecraft:stone:2"
   *   1.20 "minecraft:light_gray_concrete" <-> old pack "minecraft:silver_concrete"
   *
   * The block table that LittleTiles ships translates the first case, a small
   * alias table (above) covers the second. Both are fallbacks only.
   */
  const BlockStateMap& map = BlockStateMap::Instance();
  const std::string legacy = map.ToLegacy(kBlockId);
  std::vector<std::string> candidates;
  if (legacy != kBlockId) {
    candidates.push_back(legacy);
  }
  AppendAliases(kBlockId, &candidates);
  AppendAliases(legacy, &candidates);
  // One more round, so combinations work: "light_gray_terracotta" needs both the
  // colour rename and the terracotta rename.
  const std::size_t first_round = candidates.size();
  for (std::size_t i = 0; i < first_round; ++i) {
    AppendAliases(candidates[i], &candidates);
  }

  for (const std::string& candidate : candidates) {
    if (try_name(candidate)) {
      ProgressPrintf(Tr("[texture] %s -> %s\n"), kBlockId.c_str(),
                     candidate.c_str());
      return true;
    }
  }
  return false;
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
