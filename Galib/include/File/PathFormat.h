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

#ifndef GALIB_FILE_PATHFORMAT_H
#define GALIB_FILE_PATHFORMAT_H

#include <cstddef>

#include "GalibNamespaceDef.h"

namespace galib::file {
    bool FormatFolderPath(char *p_desc_path, GALIB_STD size_t kLength, bool kLinuxStyle = true);
}

#endif //GALIB_FILE_PATHFORMAT_H
