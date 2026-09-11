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

#ifndef GALIB_MINECRAFT_TEXTURESUPPORT_TEXTUREBAKER_H
#define GALIB_MINECRAFT_TEXTURESUPPORT_TEXTUREBAKER_H

#include <cstdint>
#include <map>
#include <string>

namespace galib::minecraft::texture_support {

// 把"贴图 × 生物群系染色 × tile 颜色"烘焙成一张 PNG。
//
// Minecraft 的原生渲染会对带 tintindex 的面乘上生物群系颜色（草方块顶面、树叶等），
// 并对 LittleTiles 的染色 tile 乘上 tile 颜色。OBJ/MTL 的多数导入器（含 Blender）
// 不会把 Kd 与 map_Kd 相乘，因此必须先把颜色乘进像素。
class TextureBaker {
 public:
  explicit TextureBaker(std::string kAssetsRoot);

  // 求某个方块在某个 tintindex 下应乘的生物群系颜色（ARGB，alpha 恒为 255）。
  // 无法判断时返回 false（调用方可以选择不染色）。
  bool ResolveTintColor(const std::string& kBlockId, int kTintIndex,
                        std::uint32_t* p_desc_argb) const;

  // 烘焙并写出 PNG。kTexturePath 是相对 assets/textures 的路径（不含 .png）。
  // kTintRgb 为 0xRRGGBB；kTintRgb == 0xFFFFFF 表示不染色。
  // kTileColorArgb 为 LT 的 tile 颜色；alpha < 255 会同时调制透明度。
  bool Bake(const std::string& kTexturePath, std::uint32_t kTintRgb,
            std::uint32_t kTileColorArgb, const std::string& kOutputPath,
            std::string* p_desc_error = nullptr) const;

 private:
  std::string assets_root_;
};

}  // namespace galib::minecraft::texture_support

#endif  // GALIB_MINECRAFT_TEXTURESUPPORT_TEXTUREBAKER_H
