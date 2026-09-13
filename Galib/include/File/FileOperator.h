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
#include <filesystem>
#include <fstream>

#include "GalibNamespaceDef.h"

namespace galib::file {
using ByteType = std::uint8_t;

template <typename CharType>
bool ReadFileBasic(const std::filesystem::path& kFilePath, CharType* buffer,
                   const std::size_t kBufferSize) {
  // A plain `std::ifstream` is used deliberately: it takes a
  // std::filesystem::path, so non-ASCII file names work on Windows, and the
  // bytes are then reinterpreted into the caller's buffer. Instantiating
  // std::basic_ifstream<CharType> for an arbitrary CharType is a portability
  // trap, hence the indirection.
  std::ifstream read_filestream(kFilePath, std::ios::binary);
  if (!read_filestream.is_open()) {
    return false;
  }

  // Read file
  read_filestream.read(reinterpret_cast<char*>(buffer),
                       static_cast<std::streamsize>(kBufferSize));

  // A short read means the file is smaller than the caller expected.
  return read_filestream.gcount() ==
         static_cast<std::streamsize>(kBufferSize);
}
}  // namespace galib::file

#endif  //GALIB_FILE_FILEOPERATOR_H
