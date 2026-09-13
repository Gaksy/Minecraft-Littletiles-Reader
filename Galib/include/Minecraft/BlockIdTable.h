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

// 1.12 chunk data (Level.Sections[].Blocks) stores numeric IDs, whereas the
// texture table and the models are indexed by block name, so this "ID -> block
// name" table is required.
// The table is produced by the generator side's
// tools/generate_block_id_table.py (that script is not distributed with the library).
class BlockIdTable {
 public:
  BlockIdTable() = default;

  bool LoadFromTsv(const std::string& kTsvPath);

  bool is_loaded() const { return !names_.empty(); }
  std::size_t size() const { return names_.size(); }

  // id + meta -> "minecraft:<name>"; when meta is non-zero, ":<meta>" is appended
  // (matching the block string format LittleTiles writes into NBT).
  // Returns an empty string for an unknown id.
  std::string BlockName(std::uint16_t kBlockId, std::uint8_t kMeta) const;

 private:
  std::map<std::uint16_t, std::string> names_;
};

}  // namespace galib::minecraft

#endif  // GALIB_MINECRAFT_BLOCKIDTABLE_H
