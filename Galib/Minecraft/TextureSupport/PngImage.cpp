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

#include "Minecraft/TextureSupport/PngImage.h"

#include <zlib.h>

#include <cstdint>
#include <cstring>
#include <fstream>

namespace galib::minecraft::texture_support {

namespace {

constexpr int kPixelsPerByte = 4;

std::uint32_t ReadBigEndian32(const unsigned char* const kData) {
  return (static_cast<std::uint32_t>(kData[0]) << 24) |
         (static_cast<std::uint32_t>(kData[1]) << 16) |
         (static_cast<std::uint32_t>(kData[2]) << 8) |
         static_cast<std::uint32_t>(kData[3]);
}

void AppendBigEndian32(std::vector<unsigned char>* const p_desc_out, const std::uint32_t kValue) {
  p_desc_out->push_back(static_cast<unsigned char>((kValue >> 24) & 0xFF));
  p_desc_out->push_back(static_cast<unsigned char>((kValue >> 16) & 0xFF));
  p_desc_out->push_back(static_cast<unsigned char>((kValue >> 8) & 0xFF));
  p_desc_out->push_back(static_cast<unsigned char>(kValue & 0xFF));
}

void AppendChunk(std::vector<unsigned char>* const p_desc_out, const char* const kType,
                 const std::vector<unsigned char>& kData) {
  AppendBigEndian32(p_desc_out, static_cast<std::uint32_t>(kData.size()));
  const std::size_t type_offset = p_desc_out->size();
  p_desc_out->insert(p_desc_out->end(), kType, kType + 4);
  p_desc_out->insert(p_desc_out->end(), kData.begin(), kData.end());
  const std::uint32_t crc =
      static_cast<std::uint32_t>(crc32(0, p_desc_out->data() + type_offset,
                                       static_cast<uInt>(4 + kData.size())));
  AppendBigEndian32(p_desc_out, crc);
}

int ChannelsOfColorType(const int kColorType) {
  switch (kColorType) {
    case 0:
      return 1;  // 灰度
    case 2:
      return 3;  // RGB
    case 3:
      return 1;  // 调色板（索引）
    case 4:
      return 2;  // 灰度 + alpha
    case 6:
      return 4;  // RGBA
    default:
      return 0;
  }
}

// 反解一行 PNG 滤波器，输出到 kLine（就地修改）
void UnfilterLine(const int kFilter, const int kBytesPerPixel, const int kStride,
                  const unsigned char* const kPrev, unsigned char* const kLine) {
  for (int i = 0; i < kStride; ++i) {
    const int a = i >= kBytesPerPixel ? kLine[i - kBytesPerPixel] : 0;
    const int b = kPrev ? kPrev[i] : 0;
    const int c = (kPrev && i >= kBytesPerPixel) ? kPrev[i - kBytesPerPixel] : 0;
    switch (kFilter) {
      case 1:
        kLine[i] = static_cast<unsigned char>(kLine[i] + a);
        break;
      case 2:
        kLine[i] = static_cast<unsigned char>(kLine[i] + b);
        break;
      case 3:
        kLine[i] = static_cast<unsigned char>(kLine[i] + (a + b) / 2);
        break;
      case 4: {
        const int pa = b - c < 0 ? c - b : b - c;
        const int pb = a - c < 0 ? c - a : a - c;
        const int pc = a + b - 2 * c;
        const int abs_pc = pc < 0 ? -pc : pc;
        int predictor = a;
        if (pa > pb || pa > abs_pc) {
          predictor = pb <= abs_pc ? b : c;
        }
        kLine[i] = static_cast<unsigned char>(kLine[i] + predictor);
        break;
      }
      default:
        break;  // filter 0：原样
    }
  }
}

}  // namespace

PngImage::PngImage(const int kWidth, const int kHeight)
    : width_(kWidth), height_(kHeight),
      pixels_(static_cast<std::size_t>(kWidth) * kHeight * kPixelsPerByte, 0) {}

unsigned char* PngImage::Pixel(const int kX, const int kY) {
  if (kX < 0 || kY < 0 || kX >= width_ || kY >= height_) {
    return nullptr;
  }
  return pixels_.data() + (static_cast<std::size_t>(kY) * width_ + kX) * kPixelsPerByte;
}

const unsigned char* PngImage::Pixel(const int kX, const int kY) const {
  return const_cast<PngImage*>(this)->Pixel(kX, kY);
}

bool PngImage::Load(const std::string& kPath, std::string* const p_desc_error) {
  const auto fail = [p_desc_error](const char* const kMessage) {
    if (p_desc_error) {
      *p_desc_error = kMessage;
    }
    return false;
  };

  std::ifstream input(kPath, std::ios::binary);
  if (!input) {
    return fail("无法打开文件");
  }
  const std::vector<unsigned char> data((std::istreambuf_iterator<char>(input)),
                                        std::istreambuf_iterator<char>());
  static const unsigned char kSignature[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
  if (data.size() < 8 || std::memcmp(data.data(), kSignature, 8) != 0) {
    return fail("不是合法的 PNG");
  }

  int width = 0;
  int height = 0;
  int bit_depth = 0;
  int color_type = 0;
  int interlace = 0;
  std::vector<unsigned char> palette;
  std::vector<unsigned char> transparency;
  std::vector<unsigned char> compressed;

  std::size_t offset = 8;
  while (offset + 12 <= data.size()) {
    const std::uint32_t length = ReadBigEndian32(data.data() + offset);
    const char* const type = reinterpret_cast<const char*>(data.data() + offset + 4);
    const unsigned char* const payload = data.data() + offset + 8;
    if (offset + 12 + length > data.size()) {
      break;
    }
    if (std::strncmp(type, "IHDR", 4) == 0 && length >= 13) {
      width = static_cast<int>(ReadBigEndian32(payload));
      height = static_cast<int>(ReadBigEndian32(payload + 4));
      bit_depth = payload[8];
      color_type = payload[9];
      interlace = payload[12];
    } else if (std::strncmp(type, "PLTE", 4) == 0) {
      palette.assign(payload, payload + length);
    } else if (std::strncmp(type, "tRNS", 4) == 0) {
      transparency.assign(payload, payload + length);
    } else if (std::strncmp(type, "IDAT", 4) == 0) {
      compressed.insert(compressed.end(), payload, payload + length);
    } else if (std::strncmp(type, "IEND", 4) == 0) {
      break;
    }
    offset += 12 + length;
  }

  if (width <= 0 || height <= 0) {
    return fail("IHDR 缺失或尺寸非法");
  }
  if (bit_depth != 8) {
    return fail("只支持 8 位深度");
  }
  if (interlace != 0) {
    return fail("不支持交错 PNG");
  }
  const int channels = ChannelsOfColorType(color_type);
  if (channels == 0) {
    return fail("不支持的颜色类型");
  }

  const std::size_t stride = static_cast<std::size_t>(width) * channels;
  std::vector<unsigned char> raw(stride * height + height);
  uLongf raw_size = static_cast<uLongf>(raw.size());
  if (uncompress(raw.data(), &raw_size, compressed.data(), static_cast<uLong>(compressed.size())) !=
      Z_OK) {
    return fail("zlib 解压失败");
  }
  raw.resize(raw_size);

  pixels_.assign(static_cast<std::size_t>(width) * height * kPixelsPerByte, 0);
  width_ = width;
  height_ = height;

  std::vector<unsigned char> prev(stride, 0);
  std::vector<unsigned char> line(stride, 0);
  std::size_t raw_offset = 0;
  for (int y = 0; y < height; ++y) {
    if (raw_offset + 1 + stride > raw.size()) {
      return fail("像素数据不完整");
    }
    const int filter = raw[raw_offset++];
    std::memcpy(line.data(), raw.data() + raw_offset, stride);
    raw_offset += stride;
    UnfilterLine(filter, channels, static_cast<int>(stride), y > 0 ? prev.data() : nullptr,
                 line.data());

    for (int x = 0; x < width; ++x) {
      const unsigned char* const source = line.data() + static_cast<std::size_t>(x) * channels;
      unsigned char* const target = Pixel(x, y);
      switch (color_type) {
        case 0:
          target[0] = target[1] = target[2] = source[0];
          target[3] = 255;
          break;
        case 2:
          target[0] = source[0];
          target[1] = source[1];
          target[2] = source[2];
          target[3] = 255;
          break;
        case 3: {
          const std::size_t index = static_cast<std::size_t>(source[0]) * 3;
          if (index + 2 < palette.size()) {
            target[0] = palette[index];
            target[1] = palette[index + 1];
            target[2] = palette[index + 2];
          }
          target[3] = source[0] < transparency.size() ? transparency[source[0]] : 255;
          break;
        }
        case 4:
          target[0] = target[1] = target[2] = source[0];
          target[3] = source[1];
          break;
        default:  // 6: RGBA
          std::memcpy(target, source, kPixelsPerByte);
          break;
      }
    }
    prev.swap(line);
  }
  return true;
}

bool PngImage::Save(const std::string& kPath, std::string* const p_desc_error) const {
  if (!is_valid()) {
    if (p_desc_error) {
      *p_desc_error = "图像为空";
    }
    return false;
  }

  std::vector<unsigned char> raw;
  raw.reserve((static_cast<std::size_t>(width_) * kPixelsPerByte + 1) * height_);
  for (int y = 0; y < height_; ++y) {
    raw.push_back(0);  // filter 0
    const unsigned char* const row = pixels_.data() + static_cast<std::size_t>(y) * width_ * 4;
    raw.insert(raw.end(), row, row + static_cast<std::size_t>(width_) * kPixelsPerByte);
  }

  uLongf compressed_size = compressBound(static_cast<uLong>(raw.size()));
  std::vector<unsigned char> compressed(compressed_size);
  if (compress2(compressed.data(), &compressed_size, raw.data(), static_cast<uLong>(raw.size()),
                Z_BEST_COMPRESSION) != Z_OK) {
    if (p_desc_error) {
      *p_desc_error = "zlib 压缩失败";
    }
    return false;
  }
  compressed.resize(compressed_size);

  std::vector<unsigned char> output;
  static const unsigned char kSignature[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
  output.insert(output.end(), kSignature, kSignature + 8);

  std::vector<unsigned char> header;
  AppendBigEndian32(&header, static_cast<std::uint32_t>(width_));
  AppendBigEndian32(&header, static_cast<std::uint32_t>(height_));
  header.push_back(8);  // 位深
  header.push_back(6);  // 颜色类型 RGBA
  header.push_back(0);  // 压缩方法
  header.push_back(0);  // 滤波方法
  header.push_back(0);  // 非交错
  AppendChunk(&output, "IHDR", header);
  AppendChunk(&output, "IDAT", compressed);
  AppendChunk(&output, "IEND", {});

  std::ofstream out(kPath, std::ios::binary);
  if (!out) {
    if (p_desc_error) {
      *p_desc_error = "无法写出文件";
    }
    return false;
  }
  out.write(reinterpret_cast<const char*>(output.data()),
            static_cast<std::streamsize>(output.size()));
  return static_cast<bool>(out);
}

}  // namespace galib::minecraft::texture_support
