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

#ifndef GALIB_MINECRAFT_TEXTURESUPPORT_MATERIALMANAGER_H
#define GALIB_MINECRAFT_TEXTURESUPPORT_MATERIALMANAGER_H

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Minecraft/TextureSupport/AssetsPackage.h"
#include "Minecraft/TextureSupport/PngImage.h"

namespace galib::minecraft::texture_support {

// The three dimensions of one material.
//   texture    : relative path of the source texture (without .png)
//   tint_rgb   : biome tint; 0x00FFFFFF means no tint
//   tile_color : the LittleTiles tile colour; 0xFFFFFFFF means no colour information
struct MaterialKey {
  std::string texture;
  std::uint32_t tint_rgb{0x00FFFFFFu};
  std::uint32_t tile_color{0xFFFFFFFFu};
};

// Material registration and baking.
//
// Two key points (see docs/assets-package.md section 4.2):
//   1. The deduplication key is the structured (texture, tint, tile_color), not a
//      concatenated string name; the name is only used to write the MTL / file
//      names, and on a name collision a suffix is appended instead of merging two
//      materials.
//   2. One source texture is decoded only once: on write, materials are grouped by
//      texture so the tinted variants of a group are produced consecutively, and
//      only one decoded result resides in memory at a time (it does not grow with
//      the total number of textures).
//      In measurements a single blocks/glowstone texture was turned into 70
//      variants, so this step saves 69 decodes outright.
//
// The library only writes images into the directory given by the host; the assets
// directory is always read-only.
class MaterialManager {
 public:
  explicit MaterialManager(const AssetsPackage& kPackage) : package_(kPackage) {}

  // Register a material and return its index (the same
  // (texture,tint,tile_color) reuses the same index).
  std::size_t Claim(const std::string& kTextureRel, std::uint32_t kTintRgb,
                    std::uint32_t kTileColorArgb);

  std::size_t size() const { return keys_.size(); }
  const std::string& Name(std::size_t kIndex) const { return names_[kIndex]; }
  const MaterialKey& Key(std::size_t kIndex) const { return keys_[kIndex]; }

  // Write every material into kOutputDir: materials that need no tinting are copied
  // from the source PNG directly (byte-identical to the source), while tinted ones
  // are baked from the decode cache and multiplied by the colour. Returns the number
  // successfully written; failure reasons are appended to error.
  std::size_t WriteTextures(const std::string& kOutputDir,
                            std::string* p_desc_error);

  // Write the .mtl. kMapKdPrefix is the relative path from the MTL's directory to
  // the texture directory (separated by '/').
  bool WriteMtl(const std::string& kMtlPath, const std::string& kMapKdPrefix,
                std::string* p_desc_error) const;

  // For diagnostics: the number of source-texture decodes actually performed
  // (independent of the material count means the cache works).
  std::size_t decode_count() const { return decode_count_; }

 private:
  const AssetsPackage& package_;
  std::map<std::tuple<std::string, std::uint32_t, std::uint32_t>, std::size_t>
      index_;
  std::vector<MaterialKey> keys_;
  std::vector<std::string> names_;
  std::unordered_set<std::string> used_names_;  // material names already taken
  std::size_t decode_count_{0};
};

// Multiply the tint and tile colour into a decoded image (per-channel multiply, two
// roundings). When the tile colour's alpha < 255 the opacity is modulated as well;
// the tint carries no alpha.
void ApplyTintAndTileColor(PngImage* p_desc_image, std::uint32_t kTintRgb,
                           std::uint32_t kTileColorArgb);

}  // namespace galib::minecraft::texture_support

#endif  // GALIB_MINECRAFT_TEXTURESUPPORT_MATERIALMANAGER_H
