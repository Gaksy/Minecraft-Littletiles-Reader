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

#include "GalibNamespaceDef.h"

bool GALIB file::IsFolderAccessible(const char* const kPFolderPath,
                                    FileStat* const p_file_stat) {
  if (!kPFolderPath) {
    return false;
  }

  if (!p_file_stat) {
    FileStat temp;
    return GALIB file::IsFolderAccessible(kPFolderPath, &temp);
  }

  if (!GetFileStat(kPFolderPath, p_file_stat)) {
    return false;
  }

  return p_file_stat->st_mode & S_IFDIR;
}
