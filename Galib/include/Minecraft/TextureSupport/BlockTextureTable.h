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

// 方块的六个面，命名与 Minecraft 模型规范一致。
enum class FaceDirection : int {
  kDown = 0,
  kUp,
  kNorth,
  kSouth,
  kWest,
  kEast,
};

inline constexpr int kFaceCount = 6;

// 一个方块六个面所用的贴图路径。
// 路径相对 assets/textures，且不含 .png 后缀，例如 "blocks/stone_diorite"。
struct BlockFaceTextures {
  std::array<std::string, kFaceCount> paths{};

  bool empty() const;
  const std::string& Path(FaceDirection direction) const;
};

// 方块 -> 六面贴图 的映射表。
//
// 数据由 tools/resolve_block_textures.py --table 生成（解析 vanilla 的
// blockstates + models 得到），C++ 侧只读表，不引入 JSON 依赖。
// TSV 每行:
//   <block 或 block:meta>  <down>  <up>  <north>  <south>  <west>  <east>
class BlockTextureTable {
 public:
  BlockTextureTable() = default;

  // 加载失败时表为空（is_loaded() 为 false），调用方应据此跳过贴图导出。
  bool LoadFromTsv(const std::string& kTsvPath);

  bool is_loaded() const { return loaded_; }
  std::size_t size() const { return entries_.size(); }

  // 先按原名（如 "minecraft:stone:3"）精确匹配，
  // 未命中时退回去掉 meta 的名字（"minecraft:stone"）。
  bool Lookup(const std::string& kBlockId,
              BlockFaceTextures* p_desc_textures) const;

 private:
  std::unordered_map<std::string, BlockFaceTextures> entries_;
  bool loaded_{false};
};

// 计算"方块内归一化坐标"在指定面上的贴图坐标。
// x/y/z 为该点在方块内的归一化坐标（x 西→东，y 下→上，z 北→南），
// 返回的 v = 0 在贴图顶部；写 OBJ 的 vt 时需要再翻转一次（vt_v = 1 - v）。
void ComputeFaceUv(FaceDirection direction, double x, double y, double z,
                   double* p_desc_u, double* p_desc_v);

// 由面法线判断朝向（取绝对值最大的那个轴；法线不必归一化）。
FaceDirection FaceDirectionFromNormal(double nx, double ny, double nz);

// 与 FaceDirection 对应的名字（表文件的列名），用于调试输出。
const char* FaceDirectionName(FaceDirection direction);

}  // namespace galib::minecraft::texture_support

#endif  // GALIB_MINECRAFT_TEXTURESUPPORT_BLOCKTEXTURETABLE_H
