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

#ifndef GALIB_FILE_FILESTATE_H
#define GALIB_FILE_FILESTATE_H

#include <sys/stat.h>

#include "Galib/GalibNamespaceDef.h"

namespace galib::file {
 using FileStat = struct GALIB_CSTD stat;

 // On success, true is returned. On error, false is returned and errno is set to indicare the error
 inline bool GetFileStat(const char *const kPFilePath, FileStat *const p_file_stat) {
  // On success, zero is returned. On error, -1 is returned
  // and errno is set to indicate the error

  // On success, (0) + 1 = 1 is true. On error, (-1) + 1 = 0, is false
  return GALIB_CSTD stat(kPFilePath, p_file_stat) + 1;
 }

 inline bool IsFileAccessible(const char* const kPFilePath) {
  FileStat temp;
  return stat(kPFilePath, &temp) == 0;
 }

 inline bool IsFileAccessible(const char* const kPFilePath, FileStat *const p_file_stat) {
  return stat(kPFilePath, p_file_stat) == 0;
 }

 bool IsFolderAccessible(const char* kPFolderPath, FileStat *p_file_stat);

 inline bool IsFolderAccessible(const char* kPFolderPath) {
  return IsFolderAccessible(kPFolderPath, nullptr);
 }

}

#endif //GALIB_FILE_FILESTATE_H
