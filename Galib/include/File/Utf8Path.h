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

#ifndef GALIB_FILE_UTF8PATH_H
#define GALIB_FILE_UTF8PATH_H

#include <filesystem>
#include <string>

namespace galib {

// Build a filesystem path from a **UTF-8** string.
//
// Why this exists: on Windows, `std::filesystem::path(std::string)` interprets the
// bytes using the ANSI code page (cp936 on a Chinese system, cp1252 elsewhere).
// Paths we receive are UTF-8 — they come from JSON (a save folder the user picked,
// a texture directory) — so a Chinese folder name turns into mojibake and every
// file operation fails with "not found". On Windows we widen explicitly; elsewhere
// UTF-8 already is the native encoding.
std::filesystem::path Utf8Path(const std::string& kUtf8);

// Same, for a `const char*` (nullptr yields an empty path).
std::filesystem::path Utf8Path(const char* const kUtf8);

// The other direction: a path as a **UTF-8** string.
//
// Use this instead of `path.string()`: on Windows that converts to the ANSI code
// page and **throws** when the name has characters it cannot represent — which is
// exactly what a Chinese save folder does.
std::string Utf8String(const std::filesystem::path& kPath);

// Same as Utf8String, but with forward slashes - the form that goes *inside* an
// OBJ/MTL file, where a backslash is not a portable separator.
std::string Utf8GenericString(const std::filesystem::path& kPath);

#ifdef _WIN32
// UTF-16 as UTF-8. A Windows command line is UTF-16, and that is where the CLI's
// arguments (a job file path) come from.
std::string Utf8String(const wchar_t* kWide);
#endif

}  // namespace galib

#endif  // GALIB_FILE_UTF8PATH_H
