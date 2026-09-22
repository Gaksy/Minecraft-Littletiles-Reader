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

#ifndef GALIB_VERSION_H
#define GALIB_VERSION_H

#include <string>

// The library version. **This header is the single source of truth** - the CLI,
// the job/progress events and any host read it from here, so there is nothing to
// keep in sync by hand.
//
// Bump MAJOR for a breaking change to the public headers or the job/package
// contracts, MINOR for new functionality, PATCH for fixes.
#define GALIB_VERSION_MAJOR 0
#define GALIB_VERSION_MINOR 3
#define GALIB_VERSION_PATCH 0

// Release channel: exactly one of "stable" or "beta". It is part of the version
// string ("0.2.0-beta"), so a log or an export record always says which kind of
// build produced it.
//
// The default is "beta" **on purpose**: a plain checkout is a development build,
// and a random build must never be able to call itself stable by accident.
// Shipping a stable release is an explicit act:
//   cmake -DGALIB_VERSION_CHANNEL=stable ..
//
// The other direction (defaulting to stable, opting into beta) is what a build
// with no release process ends up doing: everything claims to be stable, and the
// label stops carrying information.
#ifndef GALIB_VERSION_CHANNEL
#define GALIB_VERSION_CHANNEL "beta"
#endif

namespace galib {

struct Version {
  int major;
  int minor;
  int patch;
  const char* channel;  // "stable" or "beta"
};

Version CurrentVersion();

// "0.2.0-stable" / "0.2.0-beta".
std::string VersionString();

// True when the channel is "stable". Hosts use it to decide whether to warn
// ("you are running a test build") or to gate features.
bool IsStableRelease();

// One line for humans, e.g.
//   "LittleTiles Reader 0.2.0-beta (test build)"
std::string VersionLine();

}  // namespace galib

#endif  // GALIB_VERSION_H
