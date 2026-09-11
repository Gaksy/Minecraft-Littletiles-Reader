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

#include "Minecraft/BlockIdTable.h"

#include <cstdlib>
#include <fstream>
#include <sstream>

namespace galib::minecraft {

bool BlockIdTable::LoadFromTsv(const std::string& kTsvPath) {
  names_.clear();

  std::ifstream input(kTsvPath);
  if (!input) {
    return false;
  }

  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }
    const std::size_t tab = line.find('\t');
    if (tab == std::string::npos) {
      continue;
    }
    const int id = std::atoi(line.substr(0, tab).c_str());
    names_[static_cast<std::uint16_t>(id)] = line.substr(tab + 1);
  }
  return !names_.empty();
}

std::string BlockIdTable::BlockName(const std::uint16_t kBlockId,
                                    const std::uint8_t kMeta) const {
  const auto found = names_.find(kBlockId);
  if (found == names_.end()) {
    return {};
  }
  if (kMeta == 0) {
    return "minecraft:" + found->second;
  }
  return "minecraft:" + found->second + ":" + std::to_string(kMeta);
}

}  // namespace galib::minecraft
