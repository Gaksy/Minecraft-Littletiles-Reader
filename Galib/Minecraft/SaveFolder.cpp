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
 * Date Created: 09/14/2026
 */

#include "Minecraft/SaveFolder.h"

#include <filesystem>

namespace galib::minecraft {

namespace {

// The sub-folder of a save that holds this dimension, relative to the save root.
const char* DimensionSubFolder(const Dimension kDimension) {
  switch (kDimension) {
    case Dimension::kOverworld:
      return "region";
    case Dimension::kNether:
      return "DIM-1/region";
    case Dimension::kEnd:
      return "DIM1/region";
  }
  return "region";
}

// A folder "holds region data" if any *.mca sits directly in it.
bool HasRegionFiles(const std::filesystem::path& kDir) {
  std::error_code error;
  if (!std::filesystem::is_directory(kDir, error)) {
    return false;
  }
  for (std::filesystem::directory_iterator it(kDir, error), end; it != end;
       it.increment(error)) {
    if (error) {
      return false;
    }
    if (it->is_regular_file(error) && it->path().extension() == ".mca") {
      return true;
    }
  }
  return false;
}

}  // namespace

std::optional<Dimension> ParseDimension(const std::string& kName) {
  if (kName == "overworld" || kName == "minecraft:overworld" || kName.empty()) {
    return Dimension::kOverworld;
  }
  if (kName == "nether" || kName == "the_nether" ||
      kName == "minecraft:the_nether") {
    return Dimension::kNether;
  }
  if (kName == "end" || kName == "the_end" || kName == "minecraft:the_end") {
    return Dimension::kEnd;
  }
  return std::nullopt;
}

const char* DimensionName(const Dimension kDimension) {
  switch (kDimension) {
    case Dimension::kOverworld:
      return "overworld";
    case Dimension::kNether:
      return "nether";
    case Dimension::kEnd:
      return "the_end";
  }
  return "overworld";
}

std::optional<RegionFolderResolution> ResolveRegionFolder(
    const std::string& kWorldRoot, const Dimension kDimension,
    std::string* const p_desc_error) {
  const auto fail = [p_desc_error](const std::string& kReason) {
    if (p_desc_error) {
      *p_desc_error = kReason;
    }
    return std::nullopt;
  };

  if (kWorldRoot.empty()) {
    return fail("save folder is empty");
  }
  std::error_code error;
  const std::filesystem::path root(kWorldRoot);
  if (!std::filesystem::is_directory(root, error)) {
    return fail("save folder not found: " + kWorldRoot);
  }

  // Rule 1: the dimension sub-folder inside a save
  const std::filesystem::path dimension_dir =
      root / std::filesystem::path(DimensionSubFolder(kDimension));
  if (!HasRegionFiles(dimension_dir)) {
    // Rule 2: the caller handed us a region folder (or test data without level.dat)
    if (HasRegionFiles(root)) {
      return RegionFolderResolution{root.string(), "root-is-region-folder"};
    }
    return fail("no region files under " + dimension_dir.string() +
                " (dimension " + DimensionName(kDimension) + ")");
  }
  return RegionFolderResolution{dimension_dir.string(),
                                std::string("save/") +
                                    DimensionSubFolder(kDimension)};
}

}  // namespace galib::minecraft
