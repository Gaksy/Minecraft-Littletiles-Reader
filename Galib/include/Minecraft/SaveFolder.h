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

#ifndef GALIB_MINECRAFT_SAVEFOLDER_H
#define GALIB_MINECRAFT_SAVEFOLDER_H

#include <optional>
#include <string>

namespace galib::minecraft {

// A save dimension. The directory names are the 1.12 layout ("region" for the
// overworld, "DIM-1" / "DIM1" for the other two).
enum class Dimension { kOverworld, kNether, kEnd };

std::optional<Dimension> ParseDimension(const std::string& kName);
const char* DimensionName(Dimension kDimension);

struct RegionFolderResolution {
  std::string region_folder;  // absolute-ish path to the folder holding *.mca
  std::string rule;           // which rule matched; handy for logs and bug reports
};

// Resolve "<world root> + dimension" to the folder that actually holds *.mca.
//
// Callers hand us a *save* folder (the one with level.dat / region / DIM-1 ...),
// not the region folder itself — the UI picks a save in a file dialog, and a save
// is the thing a user recognises.
//
// Rules, in order:
//   1. <root>/<dimension dir>            (overworld: <root>/region)
//   2. <root> itself, if it holds *.mca  (pointing straight at a region folder,
//      or at test data that has no level.dat)
//   3. fail with a readable reason
//
// Returns nullopt when nothing matched; the reason goes to p_desc_error.
std::optional<RegionFolderResolution> ResolveRegionFolder(
    const std::string& kWorldRoot, Dimension kDimension,
    std::string* p_desc_error);

}  // namespace galib::minecraft

#endif  // GALIB_MINECRAFT_SAVEFOLDER_H
