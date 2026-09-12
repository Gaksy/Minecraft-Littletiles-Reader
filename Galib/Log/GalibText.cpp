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

#include "Log/GalibText.h"

namespace galib {

namespace {

Language g_language = Language::kZhCn;

}  // namespace

void SetLanguage(const Language kLanguage) { g_language = kLanguage; }

Language GetLanguage() { return g_language; }

bool IsEnglish() { return g_language == Language::kEnUs; }

const char* Tr(const char* const kZhCn, const char* const kEnUs) {
  return g_language == Language::kEnUs ? kEnUs : kZhCn;
}

}  // namespace galib
