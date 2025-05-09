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
#include "WidgetBase.h"

namespace galib::easy_x {
    class Button: public WidgetBase {
    public:
        explicit Button(WidgetManager* p_parent = nullptr);
        ~Button() override = default;

        void setLabel(const GALIB_STD string& kLabel);
        GALIB_NODISCARD const GALIB_STD string& getLabel()const;

        void drawWidget() const override;

    private:
        GALIB_STD string label_;
    };
}

#endif //GALIB_EASYXSUPPORT_BUTTON_H
