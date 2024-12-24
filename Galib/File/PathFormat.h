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

#ifndef GALIB_FILE_PATHFORMAT_H
#define GALIB_FILE_PATHFORMAT_H

#include <cstddef>

#include "Galib/GalibNamespaceDef.h"

namespace galib::file {
    bool FormatFolderPath(char *p_desc_path, GALIB_STD size_t kLength, bool kLinuxStyle = true);
}

#endif //GALIB_FILE_PATHFORMAT_H
