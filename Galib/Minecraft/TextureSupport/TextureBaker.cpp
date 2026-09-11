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

#include "Minecraft/TextureSupport/TextureBaker.h"

#include "Minecraft/TextureSupport/PngImage.h"

#include <utility>

namespace galib::minecraft::texture_support {

namespace {

// 1.12 平原生物群系的默认草/树叶颜色。
//
// 为什么不用 textures/colormap：LittleTiles 的存档**不记录每个 tile 的生物群系**，
// 因此只能用固定默认色（参考的 Java 模组也是用一个固定坐标去取默认生物群系色）。
// colormap 本身还是三角形布局（无效区是白色），索引约定也各版本有别，
// 用默认常量更简单也更可预测；将来若要按生物群系上色，需要额外提供生物群系数据。
constexpr std::uint32_t kDefaultGrassColor = 0x0091BD59;
constexpr std::uint32_t kDefaultFoliageColor = 0x0079C05A;

std::uint32_t MakeArgb(const unsigned int kR, const unsigned int kG, const unsigned int kB) {
  return 0xFF000000u | (kR << 16) | (kG << 8) | kB;
}

// MC 的乘色是"按分量相乘"，这里用和参考实现一致的两次四舍五入
unsigned char MultiplyChannel(const unsigned char kValue, const std::uint32_t kFactor) {
  return static_cast<unsigned char>((static_cast<unsigned int>(kValue) * kFactor + 127) / 255);
}

// 方块名 -> 用哪张 colormap；返回空表示该 tintindex 不需要染色
std::string ColormapForBlock(const std::string& kBlockName) {
  if (kBlockName == "grass" || kBlockName == "tallgrass" || kBlockName == "double_plant" ||
      kBlockName == "waterlily" || kBlockName == "lily_pad") {
    return "grass";
  }
  if (kBlockName == "leaves" || kBlockName == "leaves2" || kBlockName == "vine" ||
      kBlockName == "vine_1") {
    return "foliage";
  }
  return {};
}

std::string BlockNameOf(const std::string& kBlockId) {
  const std::size_t colon = kBlockId.find(':');
  const std::string without_namespace =
      colon == std::string::npos ? kBlockId : kBlockId.substr(colon + 1);
  const std::size_t second = without_namespace.find(':');
  return second == std::string::npos ? without_namespace : without_namespace.substr(0, second);
}

}  // namespace

TextureBaker::TextureBaker(std::string kAssetsRoot) : assets_root_(std::move(kAssetsRoot)) {}

bool TextureBaker::ResolveTintColor(const std::string& kBlockId, const int kTintIndex,
                                    std::uint32_t* const p_desc_argb) const {
  if (kTintIndex < 0) {
    return false;
  }
  // MC 里 tintindex 的含义由方块类型决定；这里覆盖最常见的草/树叶两类，
  // 其余（红石、作物茎等）暂时按草色处理，详见 docs/texture-mapping.md。
  const std::string kind = ColormapForBlock(BlockNameOf(kBlockId));
  const std::uint32_t color = kind == "foliage" ? kDefaultFoliageColor : kDefaultGrassColor;
  if (p_desc_argb) {
    *p_desc_argb = color;
  }
  return true;
}

bool TextureBaker::Bake(const std::string& kTexturePath, const std::uint32_t kTintRgb,
                        const std::uint32_t kTileColorArgb, const std::string& kOutputPath,
                        std::string* const p_desc_error) const {
  PngImage image;
  if (!image.Load(assets_root_ + "/textures/" + kTexturePath + ".png", p_desc_error)) {
    return false;
  }

  const std::uint32_t tint_r = (kTintRgb >> 16) & 0xFF;
  const std::uint32_t tint_g = (kTintRgb >> 8) & 0xFF;
  const std::uint32_t tint_b = kTintRgb & 0xFF;
  const std::uint32_t color_r = (kTileColorArgb >> 16) & 0xFF;
  const std::uint32_t color_g = (kTileColorArgb >> 8) & 0xFF;
  const std::uint32_t color_b = kTileColorArgb & 0xFF;
  const std::uint32_t color_a = (kTileColorArgb >> 24) & 0xFF;

  const bool apply_tint = kTintRgb != 0x00FFFFFFu;
  const bool apply_color =
      kTileColorArgb != 0 || color_a != 0xFF;  // 0 表示没有颜色信息

  for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x) {
      unsigned char* const pixel = image.Pixel(x, y);
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

  return image.Save(kOutputPath, p_desc_error);
}

}  // namespace galib::minecraft::texture_support
