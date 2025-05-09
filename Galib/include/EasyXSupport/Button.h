/*
 * This Source Code Form is subject to the terms of the GNU Lesser General Public License,
 * v. 3.0. If a copy of the LGPL was not distributed with this file, You can obtain one at
 * https://www.gnu.org/licenses/lgpl-3.0.html.
 *
 * Copyright (c) 2024 Gaksy (Fuhongren)
 *
 * Author: Gaksy
 * Date Created: 5/9/2025
 *
 * Additional Terms:
 * For commercial use of this software, you must obtain separate authorization from the original author.
 * Please contact Gaksy at gaksys@outlook.com to request commercial use authorization.
 */
#ifndef GALIB_EASYXSUPPORT_BUTTON_H
#define GALIB_EASYXSUPPORT_BUTTON_H

#include <string>
#include "GalibNamespaceDef.h"

namespace galib::easy_x {
    class Button {
    private:

    public:
        int x_;
        int y_;
        GALIB_STD string label_;
    };
}

#endif //GALIB_EASYXSUPPORT_BUTTON_H
