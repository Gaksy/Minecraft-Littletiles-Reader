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
 * Date Created: 09/13/2026
 */

#ifndef GALIB_MINECRAFT_TEXTURESUPPORT_ASSETSPACKAGE_H
#define GALIB_MINECRAFT_TEXTURESUPPORT_ASSETSPACKAGE_H

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "Minecraft/TextureSupport/BlockTextureTable.h"

namespace galib::minecraft::texture_support {

// Summary of the result of opening an assets directory. The host can print it
// directly for diagnostics (lint); the library does no log output (see
// docs/assets-package.md section 4.4).
struct AssetsPackageInfo {
  std::string dir;                 // assets root (kept exactly as the host wrote it)
  bool has_manifest{false};        // whether manifest.json was read
  int format_version{1};           // format_version in the manifest (defaults to 1)
  std::size_t block_count{0};      // number of block keys in the mapping table
  std::size_t texture_ref_count{0};  // number of unique textures referenced by the mapping table
  std::size_t missing_texture_count{0};  // textures referenced by the table but absent on disk
  std::vector<std::string> missing_texture_examples;  // the first few missing texture paths
  std::size_t tint_rule_count{0};  // number of rules in tint.tsv
};

// The single entry point of an "assets package": mapping table + read-only texture
// source + (optional) tint table.
//
// Directory layout (only these items are part of the library's contract, see
// docs/assets-package.md section 2):
//   <dir>/block_textures.tsv   required: <block(+meta)> + six-face textures + six-face tintindex
//   <dir>/textures/<rel>.png   required: texture sources, read-only; <rel> is an opaque relative path
//   <dir>/manifest.json        optional: the library only reads format_version
//   <dir>/tint.tsv             optional: tint override table (the library defaults are used when absent)
//
// The library does not parse the assets source (it does not read jars / blockstates /
// extract textures) and does not judge whether the origin of this directory is
// trustworthy - that is the host's job. Only "is the format valid" is checked here.
class AssetsPackage {
 public:
  // The newest format_version this build understands; a larger value in the manifest
  // makes Open fail.
  static constexpr int kSupportedFormatVersion = 1;

  // Returns nullopt on failure, with the reason written into p_desc_error (may be
  // null). No exception crosses the library boundary.
  static std::optional<AssetsPackage> Open(const std::string& kDir,
                                           std::string* p_desc_error);

  bool is_valid() const { return valid_; }
  const AssetsPackageInfo& info() const { return info_; }

  // Block -> six-face textures + tintindex. The matching rules are the same as
  // BlockTextureTable::Lookup.
  bool Lookup(const std::string& kBlockId,
              BlockFaceTextures* p_desc_out) const;

  // Texture path -> disk path: <dir>/textures/<rel>.png
  std::string ResolveTexture(const std::string& kRel) const;
  bool HasTexture(const std::string& kRel) const;

  // The fixed colour a block should be multiplied by under a given tintindex (ARGB,
  // alpha 255). The package's tint.tsv is consulted first (exact block, then with the
  // meta stripped, then the "*" wildcard), and on a miss it falls back to the library
  // defaults (grass / foliage). Returns false when kTintIndex < 0.
  bool TintOverride(const std::string& kBlockId, int kTintIndex,
                    std::uint32_t* p_desc_argb) const;

 private:
  // Load the (optional) tint.tsv: each line is <key>\t<tintindex>\t<argb in hex>
  bool LoadTintTable(const std::string& kPath);

  std::string dir_;
  BlockTextureTable table_;
  // (key, tintindex) -> ARGB; the key is a block name or "*"
  std::map<std::pair<std::string, int>, std::uint32_t> tint_rules_;
  AssetsPackageInfo info_;
  bool valid_{false};
};

}  // namespace galib::minecraft::texture_support

#endif  // GALIB_MINECRAFT_TEXTURESUPPORT_ASSETSPACKAGE_H
