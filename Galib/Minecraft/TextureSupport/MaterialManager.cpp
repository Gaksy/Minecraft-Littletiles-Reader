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

#include "Minecraft/TextureSupport/MaterialManager.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>

#include "Log/GalibText.h"

namespace galib::minecraft::texture_support {

namespace {

constexpr std::uint32_t kNoTint = 0x00FFFFFFu;
constexpr std::uint32_t kNoTileColor = 0xFFFFFFFFu;

// MC's colour multiplication is a per-channel multiply; the two roundings here
// match the reference implementation
unsigned char MultiplyChannel(const unsigned char kValue,
                              const std::uint32_t kFactor) {
  return static_cast<unsigned char>(
      (static_cast<unsigned int>(kValue) * kFactor + 127) / 255);
}

// Material name = texture + tint + tile colour, for example:
//   blocks/grass_top + 0x91BD59 + 0xFFFFBE00 -> blocks_grass_top_t91bd59_cffffbe00
// The name is only used to write the MTL and file names; deduplication relies
// on the structured key, not on this name.
std::string MakeMaterialName(const MaterialKey& kKey) {
  std::string name = kKey.texture;
  std::replace(name.begin(), name.end(), '/', '_');
  std::replace(name.begin(), name.end(), ':', '_');
  char suffix[32] = {};
  if (kKey.tint_rgb != kNoTint) {
    std::snprintf(suffix, sizeof(suffix), "_t%06x", kKey.tint_rgb);
    name += suffix;
  }
  if (kKey.tile_color != kNoTileColor) {
    std::snprintf(suffix, sizeof(suffix), "_c%08x", kKey.tile_color);
    name += suffix;
  }
  return name;
}

}  // namespace

void ApplyTintAndTileColor(PngImage* const p_desc_image,
                           const std::uint32_t kTintRgb,
                           const std::uint32_t kTileColorArgb) {
  if (p_desc_image == nullptr || !p_desc_image->is_valid()) {
    return;
  }
  const std::uint32_t tint_r = (kTintRgb >> 16) & 0xFF;
  const std::uint32_t tint_g = (kTintRgb >> 8) & 0xFF;
  const std::uint32_t tint_b = kTintRgb & 0xFF;
  const std::uint32_t color_r = (kTileColorArgb >> 16) & 0xFF;
  const std::uint32_t color_g = (kTileColorArgb >> 8) & 0xFF;
  const std::uint32_t color_b = kTileColorArgb & 0xFF;
  const std::uint32_t color_a = (kTileColorArgb >> 24) & 0xFF;

  const bool apply_tint = kTintRgb != kNoTint;
  const bool apply_color =
      kTileColorArgb != 0 || color_a != 0xFF;  // 0 means there is no colour information
  if (!apply_tint && !apply_color) {
    return;
  }

  for (int y = 0; y < p_desc_image->height(); ++y) {
    for (int x = 0; x < p_desc_image->width(); ++x) {
      unsigned char* const pixel = p_desc_image->Pixel(x, y);
      if (pixel == nullptr) {
        continue;
      }
      if (apply_tint) {
        pixel[0] = MultiplyChannel(pixel[0], tint_r);
        pixel[1] = MultiplyChannel(pixel[1], tint_g);
        pixel[2] = MultiplyChannel(pixel[2], tint_b);
      }
      if (apply_color) {
        pixel[0] = MultiplyChannel(pixel[0], color_r);
        pixel[1] = MultiplyChannel(pixel[1], color_g);
        pixel[2] = MultiplyChannel(pixel[2], color_b);
        pixel[3] = MultiplyChannel(pixel[3], color_a);
      }
    }
  }
}

std::size_t MaterialManager::Claim(const std::string& kTextureRel,
                                   const std::uint32_t kTintRgb,
                                   const std::uint32_t kTileColorArgb) {
  const std::tuple<std::string, std::uint32_t, std::uint32_t> key{
      kTextureRel, kTintRgb, kTileColorArgb};
  const auto found = index_.find(key);
  if (found != index_.end()) {
    return found->second;
  }

  MaterialKey material;
  material.texture = kTextureRel;
  material.tint_rgb = kTintRgb;
  material.tile_color = kTileColorArgb;

  // When a name collides (for example the texture itself is called
  // "grass_top_t91bd59"), append a sequence number instead of merging two
  // different materials into one - the old implementation keyed on the name and
  // silently merged them here.
  std::string name = MakeMaterialName(material);
  if (used_names_.count(name) != 0) {
    const std::string base = name;
    for (int suffix = 1;; ++suffix) {
      name = base + "_" + std::to_string(suffix);
      if (used_names_.count(name) == 0) {
        break;
      }
    }
  }

  const std::size_t index = keys_.size();
  index_.emplace(key, index);
  used_names_.insert(name);
  keys_.push_back(std::move(material));
  names_.push_back(std::move(name));
  return index;
}

std::size_t MaterialManager::WriteTextures(const std::string& kOutputDir,
                                           std::string* const p_desc_error) {
  std::error_code error;
  std::filesystem::create_directories(kOutputDir, error);
  if (error) {
    if (p_desc_error) {
      *p_desc_error += "cannot create texture dir: " + kOutputDir + " : " +
                       error.message() + "\n";
    }
    return 0;
  }
  const std::filesystem::path output_dir(kOutputDir);

  // Write out grouped by source texture: tinted variants of the same texture are
  // processed consecutively, so it only needs to be decoded once and only one
  // decoded result resides in memory at a time. Material numbering and MTL order
  // are unaffected by this.
  std::vector<std::size_t> order(keys_.size());
  for (std::size_t i = 0; i < order.size(); ++i) {
    order[i] = i;
  }
  std::stable_sort(order.begin(), order.end(),
                   [this](const std::size_t kLeft, const std::size_t kRight) {
                     return keys_[kLeft].texture < keys_[kRight].texture;
                   });

  std::size_t written = 0;
  std::string current_texture;
  PngImage decoded;
  for (const std::size_t i : order) {
    const MaterialKey& key = keys_[i];
    const std::filesystem::path target = output_dir / (names_[i] + ".png");
    const std::string source = package_.ResolveTexture(key.texture);

    // When no tinting is needed, copy the source texture directly to avoid a
    // pointless extra encode/decode round trip
    const bool needs_bake =
        key.tint_rgb != kNoTint || key.tile_color != kNoTileColor;
    bool ready = false;
    if (!needs_bake) {
      std::error_code copy_error;
      if (std::filesystem::exists(source)) {
        std::filesystem::copy_file(
            source, target,
            std::filesystem::copy_options::overwrite_existing, copy_error);
        ready = !copy_error;
      }
    } else {
      if (current_texture != key.texture) {
        current_texture = key.texture;
        decoded = PngImage();
        if (decoded.Load(source)) {
          ++decode_count_;
        }
      }
      if (!decoded.is_valid()) {
        if (p_desc_error) {
          *p_desc_error += "texture failed: " + key.texture + "\n";
        }
        continue;
      }
      PngImage baked = decoded;  // always bake from the original, leaving the decoded result untouched
      ApplyTintAndTileColor(&baked, key.tint_rgb, key.tile_color);
      std::string save_error;
      ready = baked.Save(target.string(), &save_error);
      if (!ready && p_desc_error) {
        *p_desc_error += "cannot write " + target.string() + " : " + save_error +
                         "\n";
      }
    }

    if (ready) {
      ++written;
    } else if (!needs_bake && p_desc_error) {
      *p_desc_error += "texture failed: " + key.texture + "\n";
    }
  }
  return written;
}

bool MaterialManager::WriteMtl(const std::string& kMtlPath,
                               const std::string& kMapKdPrefix,
                               std::string* const p_desc_error) const {
  std::ofstream mtl(kMtlPath);
  if (!mtl) {
    if (p_desc_error) {
      *p_desc_error += "cannot open " + kMtlPath + "\n";
    }
    return false;
  }
  mtl << galib::Tr("# Generated by LittleTilesReader\n");
  for (std::size_t i = 0; i < keys_.size(); ++i) {
    mtl << "\nnewmtl " << names_[i] << "\n"
        << "Ka 1.000 1.000 1.000\n"
        << "Kd 1.000 1.000 1.000\n"
        << "d 1.0\n"
        << "map_Kd " << kMapKdPrefix << "/" << names_[i] << ".png\n";
  }
  mtl.close();
  return true;
}

}  // namespace galib::minecraft::texture_support
