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
 * Date Created: 09/20/2026
 */

#include "Minecraft/BlockStateMap.h"

namespace galib::minecraft {

namespace {

// Index of the ':' that separates the block name from the metadata, counted from
// the start of a 1.12.2 name: "minecraft:wool:5" -> position of the second ':'.
// std::string::npos when the name carries no metadata.
std::size_t MetadataSeparator(const std::string& kLegacyName) {
  const std::size_t namespace_separator = kLegacyName.find(':');
  if (namespace_separator == std::string::npos) {
    return std::string::npos;
  }
  return kLegacyName.find(':', namespace_separator + 1);
}

}  // namespace

std::string StripBlockStates(const std::string& kName) {
  const std::size_t bracket = kName.find('[');
  if (bracket == std::string::npos) {
    return kName;
  }
  return kName.substr(0, bracket);
}

const BlockStateMap& BlockStateMap::Instance() {
  // Function-local static: built once, on first use, and safe to use from
  // several threads afterwards (C++11 magic statics).
  static const BlockStateMap instance;
  return instance;
}

BlockStateMap::BlockStateMap() {
  const char* const* const legacy_names = block_map_data::LegacyNames();
  const char* const* const modern_states = block_map_data::ModernStates();
  const std::size_t count = block_map_data::Count();
  for (std::size_t i = 0; i < count; ++i) {
    const std::string legacy = legacy_names[i];
    const std::string modern = modern_states[i];
    legacy_to_modern_.emplace(legacy, modern);
    // The first entry wins if two old names share the same modern state (several
    // 1.12.2 blocks collapsed into one in 1.13+, e.g. stone variants): the
    // reverse direction then produces the oldest/most generic name, and the
    // forward direction stays lossless because it is keyed by the old name.
    modern_to_legacy_.emplace(modern, legacy);
  }
}

std::string BlockStateMap::ToModern(const std::string& kLegacyName,
                                    bool* const p_desc_mapped) const {
  const auto found = legacy_to_modern_.find(kLegacyName);
  if (found != legacy_to_modern_.end()) {
    if (p_desc_mapped != nullptr) {
      *p_desc_mapped = true;
    }
    return found->second;
  }
  if (p_desc_mapped != nullptr) {
    *p_desc_mapped = false;
  }
  // Same fallback as LittleTiles' OldLittleTilesDataParser.resolveBlockState():
  // a name that is not in the table keeps its base id and loses the metadata,
  // because the flattened name of a mod block usually has no metadata at all.
  const std::size_t separator = MetadataSeparator(kLegacyName);
  return separator == std::string::npos ? kLegacyName
                                        : kLegacyName.substr(0, separator);
}

std::string BlockStateMap::ToLegacy(const std::string& kModernState,
                                    bool* const p_desc_mapped) const {
  const auto found = modern_to_legacy_.find(kModernState);
  if (found != modern_to_legacy_.end()) {
    if (p_desc_mapped != nullptr) {
      *p_desc_mapped = true;
    }
    return found->second;
  }
  if (p_desc_mapped != nullptr) {
    *p_desc_mapped = false;
  }
  // 1.12.2 has no block states, so the best effort is the bare name: blocks that
  // did not exist back then render as LittleTiles' missing block in game, which
  // the caller reports.
  return StripBlockStates(kModernState);
}

}  // namespace galib::minecraft
