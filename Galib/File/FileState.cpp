/*
 * This Source Code Form is subject to the terms of the GNU Lesser General Public License,
 * v. 3.0. If a copy of the LGPL was not distributed with this file, You can obtain one at
 * https://www.gnu.org/licenses/lgpl-3.0.html.
 *
 * Copyright (c) 2024 Gaksy (Fuhongren)
 *
 * Author: Gaksy
 * Date Created: 12/22/2024
 *
 * Additional Terms:
 * For commercial use of this software, you must obtain separate authorization from the original author.
 * Please contact Gaksy at gaksys@outlook.com to request commercial use authorization.
 */

#include "Galib/GalibNamespaceDef.h"
#include "Galib/File/FileState.h"

bool GALIB file::IsFolderAccessible(const char *const kPFolderPath, FileStat *const p_file_stat) {
 if (!kPFolderPath) { return false; }

 if (!p_file_stat) {
  FileStat temp;
  return GALIB file::IsFolderAccessible(kPFolderPath, &temp);
 }

 if (!GetFileStat(kPFolderPath, p_file_stat)) { return false; }

 return p_file_stat->st_mode & S_IFDIR;
}