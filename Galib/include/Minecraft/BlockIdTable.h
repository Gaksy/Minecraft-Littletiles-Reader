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

#ifndef GALIB_MINECRAFT_BLOCKIDTABLE_H
#define GALIB_MINECRAFT_BLOCKIDTABLE_H

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

namespace galib::minecraft {

// 1.12 的区块数据（Level.Sections[].Blocks）存的是数字 ID，
// 而贴图表与模型都用方块名索引，因此需要这张 "ID -> 方块名" 表。
// 表由 tools/generate_block_id_table.py 生成。
class BlockIdTable {
 public:
  BlockIdTable() = default;

  bool LoadFromTsv(const std::string& kTsvPath);

  bool is_loaded() const { return !names_.empty(); }
  std::size_t size() const { return names_.size(); }

  // id + meta -> "minecraft:<name>"；meta 非 0 时追加 ":<meta>"
  // （与 LittleTiles 在 NBT 里写的方块字符串格式一致）。
  // 未知 id 返回空串。
  std::string BlockName(std::uint16_t kBlockId, std::uint8_t kMeta) const;

 private:
  std::map<std::uint16_t, std::string> names_;
};

}  // namespace galib::minecraft

#endif  // GALIB_MINECRAFT_BLOCKIDTABLE_H
