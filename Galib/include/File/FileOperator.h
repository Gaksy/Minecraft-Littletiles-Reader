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

#ifndef GALIB_FILE_FILEOPERATOR_H
#define GALIB_FILE_FILEOPERATOR_H

#include <cstddef>
#include <cstdint>
#include <fstream>

#include "GalibNamespaceDef.h"

namespace galib::file {
using ByteType = GALIB_STD uint8_t;

template <typename CharType>
bool ReadFileBasic(const char* const kPFilePath, CharType* buffer,
                   const GALIB_STD size_t kBufferSize) {
  // Create input file stream
  GALIB_STD basic_ifstream<CharType> read_filestream;

  // Open file
  read_filestream.open(kPFilePath, GALIB_STD ios::binary);
  if (!read_filestream.is_open()) {
    return false;
  }

  // Read file
  read_filestream.read(buffer, kBufferSize);

  // Close file stream
  read_filestream.close();

  // Return !read_filestream.fail()
  return read_filestream.operator bool();
}
}  // namespace galib::file

#endif  //GALIB_FILE_FILEOPERATOR_H
