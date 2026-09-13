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
 * Date Created: 09/12/2026
 */

#ifndef GALIB_MINECRAFT_TEXTURESUPPORT_PNGIMAGE_H
#define GALIB_MINECRAFT_TEXTURESUPPORT_PNGIMAGE_H

#include <cstddef>
#include <string>
#include <vector>

namespace galib::minecraft::texture_support {

// Minimal PNG reader/writer: it only covers the subset that Minecraft textures
// actually use.
// Read: 8-bit depth, non-interlaced, colour types 0/2/3/4/6; always converted to RGBA.
// Write: 8-bit RGBA, non-interlaced, filter 0.
class PngImage {
 public:
  PngImage() = default;
  PngImage(int kWidth, int kHeight);

  // Returns false on failure and writes the reason into p_desc_error (may be null)
  bool Load(const std::string& kPath, std::string* p_desc_error = nullptr);
  bool Save(const std::string& kPath,
            std::string* p_desc_error = nullptr) const;

  bool is_valid() const { return width_ > 0 && height_ > 0; }
  int width() const { return width_; }
  int height() const { return height_; }

  // 4 bytes per pixel (RGBA); returns nullptr when the index is out of range
  unsigned char* Pixel(int kX, int kY);
  const unsigned char* Pixel(int kX, int kY) const;

 private:
  int width_{0};
  int height_{0};
  std::vector<unsigned char> pixels_;
};

}  // namespace galib::minecraft::texture_support

#endif  // GALIB_MINECRAFT_TEXTURESUPPORT_PNGIMAGE_H
