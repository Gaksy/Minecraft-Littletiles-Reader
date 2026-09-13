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
 * Date Created: 12/22/2024
 */

#include "File/FileState.h"

#include <filesystem>

#include "GalibNamespaceDef.h"

bool galib::file::IsFileAccessible(const std::filesystem::path& kPath) {
  std::error_code error;
  return std::filesystem::is_regular_file(kPath, error) && !error;
}

bool galib::file::IsFolderAccessible(const std::filesystem::path& kPath) {
  std::error_code error;
  return std::filesystem::is_directory(kPath, error) && !error;
}

bool galib::file::GetFileSize(const std::filesystem::path& kPath,
                              std::uintmax_t* const p_desc_size) {
  std::error_code error;
  const std::uintmax_t size = std::filesystem::file_size(kPath, error);
  if (error) {
    return false;
  }
  if (p_desc_size) {
    *p_desc_size = size;
  }
  return true;
}

bool galib::file::IsFolderAccessible(const char* const kPFolderPath,
                                     FileStat* const p_file_stat) {
  if (!kPFolderPath) {
    return false;
  }

  if (!p_file_stat) {
    FileStat temp;
    return galib::file::IsFolderAccessible(kPFolderPath, &temp);
  }

  if (!GetFileStat(kPFolderPath, p_file_stat)) {
    return false;
  }

  return p_file_stat->st_mode & S_IFDIR;
}
