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

// 极简 PNG 读写：只覆盖 Minecraft 贴图实际用到的子集。
// 读：8 位深度、非交错，颜色类型 0/2/3/4/6；统一转成 RGBA。
// 写：8 位 RGBA、非交错、filter 0。
class PngImage {
 public:
  PngImage() = default;
  PngImage(int kWidth, int kHeight);

  // 失败时返回 false，并把原因写入 p_desc_error（可为空）
  bool Load(const std::string& kPath, std::string* p_desc_error = nullptr);
  bool Save(const std::string& kPath,
            std::string* p_desc_error = nullptr) const;

  bool is_valid() const { return width_ > 0 && height_ > 0; }
  int width() const { return width_; }
  int height() const { return height_; }

  // 每像素 4 字节（RGBA）；下标越界返回 nullptr
  unsigned char* Pixel(int kX, int kY);
  const unsigned char* Pixel(int kX, int kY) const;

 private:
  int width_{0};
  int height_{0};
  std::vector<unsigned char> pixels_;
};

}  // namespace galib::minecraft::texture_support

#endif  // GALIB_MINECRAFT_TEXTURESUPPORT_PNGIMAGE_H
