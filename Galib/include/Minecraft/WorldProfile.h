// Copyright (c) 2026 Gaksy (Fuhongren)
// SPDX-License-Identifier: LGPL-3.0-only
#ifndef GALIB_MINECRAFT_WORLDPROFILE_H
#define GALIB_MINECRAFT_WORLDPROFILE_H

#include <nbt_tags.h>

#include <string>

namespace galib::minecraft {
struct WorldProfile {
  std::string minecraft_version;
  int data_version{0};
  bool inferred{false};
};

// Only the two native environments under development are accepted. Missing
// metadata may infer a layout, but never proves a world's creation history.
WorldProfile DetectWorldProfile(const nbt::tag_compound& root,
                                const std::string& requested = "auto");
std::string CanonicalBlockState(const std::string& state);
std::string BlockStateBaseName(const std::string& state);
bool IsAirBlockState(const std::string& state);
}  // namespace galib::minecraft
#endif
