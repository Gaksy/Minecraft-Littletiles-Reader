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

#include "Log/GalibLog.h"

#include <cstdarg>
#include <cstdio>

namespace galib {

namespace {

bool g_progress_enabled = true;

}  // namespace

void SetProgressEnabled(const bool kEnabled) { g_progress_enabled = kEnabled; }

bool IsProgressEnabled() { return g_progress_enabled; }

void ProgressPrintf(const char* const kFormat, ...) {
  if (!g_progress_enabled) {
    return;
  }
  va_list arguments;
  va_start(arguments, kFormat);
  std::vprintf(kFormat, arguments);
  va_end(arguments);
}

}  // namespace galib
