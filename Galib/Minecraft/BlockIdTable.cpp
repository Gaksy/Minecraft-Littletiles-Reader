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
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <nbt_tags.h>
#include <io/stream_reader.h>

#include "File/Utf8Path.h"

namespace galib::minecraft {

bool BlockIdTable::LoadFromTsv(const std::string& kTsvPath) {
  names_.clear();

  // The table path can live under a folder the user named (a project directory, say),
  // so it is converted from UTF-8 instead of being handed to the stream as bytes.
  std::ifstream input(Utf8Path(kTsvPath));
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

bool BlockIdTable::LoadForgeRegistry(const std::string& path) {
  std::ifstream input(Utf8Path(path), std::ios::binary);
  if (!input) return false;
  boost::iostreams::filtering_istream stream;
  stream.push(boost::iostreams::gzip_decompressor());
  stream.push(input);
  nbt::io::stream_reader reader(stream);
  const auto root = reader.read_compound().second;
  if (!root->has_key("FML")) return false;
  const auto& fml = root->at("FML").as<nbt::tag_compound>();
  if (!fml.has_key("Registries")) return false;
  const auto& registries = fml.at("Registries").as<nbt::tag_compound>();
  if (!registries.has_key("minecraft:blocks")) return false;
  const auto& blocks = registries.at("minecraft:blocks").as<nbt::tag_compound>();
  if (!blocks.has_key("ids")) return false;
  bool loaded = false;
  for (const auto& item : blocks.at("ids").as<nbt::tag_list>()) {
    const auto& entry = item.as<nbt::tag_compound>();
    const auto id = entry.at("V").as<nbt::tag_int>().get();
    const auto name = entry.at("K").as<nbt::tag_string>().get();
    if (id < 0 || id > 65535 || name.empty()) continue;
    names_[static_cast<std::uint16_t>(id)] = name;
    loaded = true;
  }
  return loaded;
}

std::string BlockIdTable::BlockName(const std::uint16_t kBlockId,
                                    const std::uint8_t kMeta) const {
  const auto found = names_.find(kBlockId);
  if (found == names_.end()) {
    return {};
  }
  if (kMeta == 0) {
    return found->second.find(':') == std::string::npos ? "minecraft:" + found->second : found->second;
  }
  return (found->second.find(':') == std::string::npos ? "minecraft:" + found->second : found->second) + ":" + std::to_string(kMeta);
}

}  // namespace galib::minecraft
