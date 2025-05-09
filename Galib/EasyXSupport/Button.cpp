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

#include "EasyXSupport/Button.h"

#include <easyx.h>
#include "GalibNamespaceDef.h"

using GALIB easy_x::Button;

Button::Button(WidgetManager *p_parent):
    WidgetBase(p_parent)
{ }

void Button::setLabel(const std::string &kLabel) {
    label_ = kLabel;
}

const std::string &Button::getLabel() const {
    return label_;
}


void Button::drawWidget() const {
    const int x = getDisplayPos().x;
    const int y = getDisplayPos().z;

    const int width = getWidgetWidth();
    const int height = getWidgetHeight();

    fillroundrect(x, y, x + width, y + height, 5,  5);
}


