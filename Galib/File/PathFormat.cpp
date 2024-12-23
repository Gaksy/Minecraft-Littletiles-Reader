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

#include <cstddef>

#include "Galib/GalibNamespaceDef.h"
#include "Galib/File/PathFormat.h"

bool GALIB file::FormatFolderPath(char *const p_desc_path, const GALIB_STD size_t kLength, const bool kLinuxStyle) {
    if (!p_desc_path) { return false; }
    GALIB_STD size_t index;
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
