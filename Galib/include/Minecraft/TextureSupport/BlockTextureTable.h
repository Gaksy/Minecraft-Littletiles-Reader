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

#ifndef GALIB_MINECRAFT_TEXTURESUPPORT_BLOCKTEXTURETABLE_H
#define GALIB_MINECRAFT_TEXTURESUPPORT_BLOCKTEXTURETABLE_H

#include <array>
#include <cstddef>
#include <string>
#include <unordered_map>

namespace galib::minecraft::texture_support {

// The six faces of a block; the names match the Minecraft model specification.
enum class FaceDirection : int {
  kDown = 0,
  kUp,
  kNorth,
  kSouth,
  kWest,
  kEast,
};

inline constexpr int kFaceCount = 6;

// Texture paths used by the six faces of one block.
// Paths are relative to assets/textures and exclude the .png suffix, for example
// "blocks/stone_diorite".
struct BlockFaceTextures {
  std::array<std::string, kFaceCount> paths{};
  // tintindex of that face; -1 means no tinting (see docs/texture-mapping.md)
  std::array<int, kFaceCount> tints{};

  bool empty() const;
  const std::string& Path(FaceDirection direction) const;
  int Tint(FaceDirection direction) const;
};

// Block -> six-face textures mapping table.
//
// The data is produced by the generator side's
// tools/resolve_block_textures.py --table (by parsing vanilla blockstates +
// models; that script is not distributed with the library).
//
// A plain TSV rather than JSON: it is tens of thousands of rows here, and reading
// it needs no parsing code at all. (JSON is available - Boost.JSON is already a
// dependency and is used for the manifest and job files - this is purely about
// what suits bulk tabular data.)
// Each TSV line:
//   <block or block:meta>  <down>  <up>  <north>  <south>  <west>  <east>
class BlockTextureTable {
 public:
  BlockTextureTable() = default;

  // On load failure the table is empty (is_loaded() is false) and the caller should
  // skip texture export accordingly.
  bool LoadFromTsv(const std::string& kTsvPath);

  bool is_loaded() const { return loaded_; }
  std::size_t size() const { return entries_.size(); }

  // All entries, for lint / statistics iteration (keys are "<block>" or "<block>:<meta>").
  const std::unordered_map<std::string, BlockFaceTextures>& entries() const {
    return entries_;
  }

  // First match the original name exactly (e.g. "minecraft:stone:3"); on a miss,
  // fall back to the name with the meta stripped ("minecraft:stone").
  bool Lookup(const std::string& kBlockId,
              BlockFaceTextures* p_desc_textures) const;

 private:
  std::unordered_map<std::string, BlockFaceTextures> entries_;
  bool loaded_{false};
};

// Compute the texture coordinate of an "in-block normalized coordinate" on the
// given face.
// x/y/z is the normalized coordinate of the point inside the block (x west->east,
// y down->up, z north->south); the returned v = 0 is at the top of the texture, so
// it must be flipped once more when writing OBJ vt (vt_v = 1 - v).
void ComputeFaceUv(FaceDirection direction, double x, double y, double z,
                   double* p_desc_u, double* p_desc_v);

// Determine the direction from a face normal (the axis with the largest absolute
// value wins; the normal need not be normalized).
FaceDirection FaceDirectionFromNormal(double nx, double ny, double nz);

// Name corresponding to a FaceDirection (the table file's column names), used for
// debug output.
const char* FaceDirectionName(FaceDirection direction);

// Strip ":meta" from "namespace:name:meta"; returns the input unchanged when there
// is no meta.
std::string StripBlockMeta(const std::string& kBlockId);

}  // namespace galib::minecraft::texture_support

#endif  // GALIB_MINECRAFT_TEXTURESUPPORT_BLOCKTEXTURETABLE_H
