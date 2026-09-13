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

#ifndef GALIB_LOG_GALIBLOG_H
#define GALIB_LOG_GALIBLOG_H

#include "GalibNamespaceDef.h"

#if defined(__clang__) || defined(__GNUC__)
#define GALIB_PRINTF_LIKE(kFormatIndex, kFirstArgumentIndex) \
  __attribute__((format(printf, kFormatIndex, kFirstArgumentIndex)))
#else
#define GALIB_PRINTF_LIKE(kFormatIndex, kFirstArgumentIndex)
#endif

namespace galib {

// Runtime switch for progress output.
//
// The library prints progress per chunk and per block (which chunk is being
// parsed, which block is being built, ...). That is acceptable for a local CLI,
// but as a server-side core library it must be switchable off entirely - it
// pollutes the logs and turns stdout into a bottleneck under high concurrency.
// The CLI's "print progress" option controls it.
//
// Note: this is global state and, like the other global state in the library, it
// is not thread-safe; before threading it should become an explicitly passed
// logger parameter.
void SetProgressEnabled(bool kEnabled);

[[nodiscard]] bool IsProgressEnabled();

// Progress output: prints nothing when the switch is off.
// Only for progress/debug information; errors and warnings still go straight to
// std::printf / std::cerr.
void ProgressPrintf(const char* kFormat, ...) GALIB_PRINTF_LIKE(1, 2);

}  // namespace galib

#endif  // GALIB_LOG_GALIBLOG_H
