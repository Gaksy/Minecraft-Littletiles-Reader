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

#include "Version.h"

#include <string>

namespace galib {

namespace {

// Anything other than exactly "stable" counts as a pre-release build, so a typo
// in the channel never silently produces a "stable" label.
constexpr const char* kStableChannel = "stable";

std::string NumberString() {
  return std::to_string(GALIB_VERSION_MAJOR) + "." +
         std::to_string(GALIB_VERSION_MINOR) + "." +
         std::to_string(GALIB_VERSION_PATCH);
}

}  // namespace

Version CurrentVersion() {
  return Version{GALIB_VERSION_MAJOR, GALIB_VERSION_MINOR, GALIB_VERSION_PATCH,
                 GALIB_VERSION_CHANNEL};
}

std::string VersionString() {
  return NumberString() + "-" + GALIB_VERSION_CHANNEL;
}

bool IsStableRelease() {
  return std::string(GALIB_VERSION_CHANNEL) == kStableChannel;
}

std::string VersionLine() {
  return std::string("LittleTiles Reader ") + VersionString() +
         (IsStableRelease() ? " (stable release)" : " (test build)");
}

}  // namespace galib
