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

#include "File/Utf8Path.h"

#ifdef _WIN32
#include <windows.h>

#include <vector>
#endif

namespace galib {

std::filesystem::path Utf8Path(const std::string& kUtf8) {
#ifdef _WIN32
  if (kUtf8.empty()) {
    return {};
  }
  const int length = MultiByteToWideChar(CP_UTF8, 0, kUtf8.c_str(),
                                         static_cast<int>(kUtf8.size()), nullptr, 0);
  if (length <= 0) {
    return std::filesystem::path(kUtf8);   // 转换失败就退回原样，至少行为可预期
  }
  std::wstring wide(static_cast<std::size_t>(length), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, kUtf8.c_str(), static_cast<int>(kUtf8.size()),
                      wide.data(), length);
  return std::filesystem::path(wide);
#else
  return std::filesystem::path(kUtf8);     // 类 Unix 上 UTF-8 就是本地编码
#endif
}

std::filesystem::path Utf8Path(const char* const kUtf8) {
  return kUtf8 == nullptr ? std::filesystem::path() : Utf8Path(std::string(kUtf8));
}

std::string Utf8String(const std::filesystem::path& kPath) {
#ifdef _WIN32
  const std::wstring wide = kPath.wstring();
  if (wide.empty()) {
    return {};
  }
  const int length = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(),
                                         static_cast<int>(wide.size()), nullptr, 0,
                                         nullptr, nullptr);
  if (length <= 0) {
    return {};
  }
  std::string utf8(static_cast<std::size_t>(length), '\0');
  WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()),
                      utf8.data(), length, nullptr, nullptr);
  return utf8;
#else
  return kPath.string();
#endif
}

}  // namespace galib
