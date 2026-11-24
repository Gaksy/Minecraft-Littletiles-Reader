// Copyright (c) 2026 Gaksy (Fuhongren)
// SPDX-License-Identifier: LGPL-3.0-only
#include "Minecraft/WorldProfile.h"

#include <algorithm>
#include <map>
#include <sstream>
#include <stdexcept>

namespace galib::minecraft {
std::string BlockStateBaseName(const std::string& state) {
  return state.substr(0, state.find('['));
}

std::string CanonicalBlockState(const std::string& state) {
  const auto start = state.find('[');
  if (start == std::string::npos) return state;
  if (state.empty() || state.back() != ']')
    throw std::runtime_error("invalid block state: " + state);
  std::map<std::string, std::string> properties;
  std::istringstream stream(state.substr(start + 1, state.size() - start - 2));
  std::string token;
  while (std::getline(stream, token, ',')) {
    const auto equals = token.find('=');
    if (equals == std::string::npos || !equals || equals + 1 == token.size() ||
        !properties.emplace(token.substr(0, equals), token.substr(equals + 1))
             .second)
      throw std::runtime_error("invalid block state properties: " + state);
  }
  std::string result = state.substr(0, start);
  if (!properties.empty()) {
    result += '[';
    for (const auto& property : properties) {
      if (result.back() != '[') result += ',';
      result += property.first + '=' + property.second;
    }
    result += ']';
  }
  return result;
}

bool IsAirBlockState(const std::string& state) {
  const auto name = BlockStateBaseName(state);
  return name == "minecraft:air" || name == "minecraft:cave_air" ||
         name == "minecraft:void_air";
}

WorldProfile DetectWorldProfile(const nbt::tag_compound& root,
                                const std::string& requested) {
  if (requested != "auto" && requested != "1.12.2" && requested != "1.20.1")
    throw std::runtime_error("unsupported Minecraft profile: " + requested);
  const auto& level =
      root.has_key("Level") ? root.at("Level").as<nbt::tag_compound>() : root;
  WorldProfile result;
  if (root.has_key("DataVersion"))
    result.data_version = root.at("DataVersion").as<nbt::tag_int>().get();
  else if (level.has_key("DataVersion"))
    result.data_version = level.at("DataVersion").as<nbt::tag_int>().get();
  if (result.data_version == 1343)
    result.minecraft_version = "1.12.2";
  else if (result.data_version == 3465)
    result.minecraft_version = "1.20.1";
  else if (result.data_version)
    throw std::runtime_error("unsupported chunk DataVersion: " +
                             std::to_string(result.data_version));
  else {
    result.inferred = true;
    if (level.has_key("sections"))
      result.minecraft_version = "1.20.1";
    else if (level.has_key("Sections"))
      result.minecraft_version = "1.12.2";
    else
      throw std::runtime_error(
          "cannot infer chunk storage layout without DataVersion");
  }
  if ((result.minecraft_version == "1.20.1" && level.has_key("Sections")) ||
      (result.minecraft_version == "1.12.2" && level.has_key("sections")))
    throw std::runtime_error(
        "chunk version/layout conflict; migrated saves are unsupported");
  if (requested != "auto" && requested != result.minecraft_version)
    throw std::runtime_error(
        "chunk version does not match selected native environment " +
        requested);
  return result;
}
}  // namespace galib::minecraft
