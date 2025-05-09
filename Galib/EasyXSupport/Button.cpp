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
#include <graphics.h>

#include "GalibNamespaceDef.h"

using GALIB easy_x::Button;

Button::Button(WidgetManager *p_parent):
    WidgetBase(p_parent),
    radius_(10),
    is_mouse_inner_(true),
    origin_color_(getcolor())
{ }

void Button::setLabel(const std::string &kLabel) {
    label_ = kLabel;
}

const std::string &Button::getLabel() const {
    return label_;
}

void Button::setRoundRect(const int radius) {
    if (radius >= 0) {
        radius_ = radius;
    }
}


void Button::drawWidget() {
    const int left = getDisplayPos().x;
    const int top = getDisplayPos().z;
    const int bottom = top + getWidgetHeight();
    const int right = left + getWidgetWidth();

    // draw background
    if (is_mouse_inner_) {
        COLORREF currentColor = getfillcolor();
        setfillcolor(RGB(200, 200, 250));
        fillroundrect(left, top, right, bottom, radius_,  radius_);
        setfillcolor(currentColor);
    }
    else {
        fillroundrect(left, top, right, bottom, radius_,  radius_);
    }

    // draw font
    RECT rect {left, top, right, bottom};

    int mod = getbkmode();
    COLORREF currentColor = getcolor();

    setbkmode(TRANSPARENT);
    setcolor(RGB(50, 50, 50));

    drawtext(_T(label_.c_str()), &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    setcolor(currentColor);
    setbkmode(mod);
}

void Button::eventMouseMove(const int mouse_x, const int mouse_y) {
    const int x_min = getDisplayPos().x;
    const int x_max = getWidgetWidth() + x_min;
    const int y_min = getDisplayPos().z;
    const int y_max = getWidgetHeight() + y_min;

    if ((mouse_x >= x_min && mouse_x <= x_max) &&
        (mouse_y >= y_min && mouse_y <= y_max)) {
        is_mouse_inner_ = true;
    }
    else {
        is_mouse_inner_ = false;
    }

}


