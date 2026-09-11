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

#include "File/PathFormat.h"

#include <cstddef>

#include "GalibNamespaceDef.h"

using GALIB_STD size_t;

bool GALIB file::FormatFolderPath(char* const p_desc_path, const size_t kLength,
                                  const bool kLinuxStyle) {
  if (!p_desc_path) {
    return false;
  }
  size_t index;
  if (kLinuxStyle) {
    for (index = 0; index < kLength; ++index) {
      if (p_desc_path[index] == '\\') {
        p_desc_path[index] = '/';
      }
    }
  } else {
    for (index = 0; index < kLength; ++index) {
      if (p_desc_path[index] == '/') {
        p_desc_path[index] = '\\';
      }
    }
  }

  if (index >= 1) {
    --index;
    if (p_desc_path[index] == '/' || p_desc_path[index] == '\\') {
      p_desc_path[index] = '\0';
    }
    return p_desc_path[0] != '\0';
  }

  return true;
}
