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

#ifndef GALIB_LOG_GALIBTEXT_H
#define GALIB_LOG_GALIBTEXT_H

#include "GalibNamespaceDef.h"

namespace galib {

// The library emits English-only messages, to avoid mojibake caused by terminal
// encodings. This used to be the bilingual hook taking a Chinese and an English
// string; the Chinese argument has been dropped.
//
// Every user-visible string still goes through this one function, so restoring
// another language later means changing this entry point only.
[[nodiscard]] const char* Tr(const char* kEnUs);

}  // namespace galib

#endif  // GALIB_LOG_GALIBTEXT_H
