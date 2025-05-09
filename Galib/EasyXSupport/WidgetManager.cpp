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

#include "EasyXSupport/WidgetManager.h"

#include <easyx.h>
#include "EasyXSupport/WidgetBase.h"

using GALIB easy_x::WidgetManager;

using GALIB_STD list;
using GALIB_STD find;

WidgetManager::WidgetManager(int width, int height):
    window_width_(width),
    window_height_(height)
{
    if (window_width_ <= 0)
        window_width_ = 100;
    if (window_height_ <= 0)
        window_height_ = 100;
    initgraph(window_width_, window_height_);
}

void WidgetManager::setWindowsSize(const int width, const int height) {
    if (width <= 0 || height <= 0) { return; }
    window_width_ = width;
    window_height_ = height;
    initgraph(window_width_, window_height_);
}

void WidgetManager::setWindowsWidth(const int width) {
    setWindowsSize(width, window_height_);
}

void WidgetManager::setWindowsHeight(const int height) {
    setWindowsSize(window_width_, height);
}

int WidgetManager::getWindowsWidth() const {
    return window_width_;
}

int WidgetManager::getWindowsHeight() const {
    return window_height_;
}

void WidgetManager::addWidgetBase(WidgetBase *p_widget_base) {
    const list<WidgetBase*>::iterator it = find(widget_bases_.begin(), widget_bases_.end(), p_widget_base);
    if(it == widget_bases_.end()) {
        this->widget_bases_.push_back(p_widget_base);
    }
}

void WidgetManager::removeWidgetBase(WidgetBase *p_widget_base) {
    const list<WidgetBase*>::iterator it = find(widget_bases_.begin(), widget_bases_.end(), p_widget_base);
    if(it != widget_bases_.end()) {
        widget_bases_.remove(p_widget_base);
    }
}

void WidgetManager::start() {
    BeginBatchDraw();
    ExMessage easy_x_messager{};

    while (true) {
        DWORD start_time = GetTickCount();
        while (peekmessage(&easy_x_messager)) {
            if(easy_x_messager.message == WM_MOUSEMOVE) {
                this->emitEventMouseMove_(easy_x_messager.x, easy_x_messager.y);
            }
        }

        cleardevice();
        drawAllWidgetBase_();
        FlushBatchDraw();

        DWORD end_time = GetTickCount();
        DWORD delta_time = end_time - start_time;

        if (delta_time < 1000 / 60) {//(60fps)  游戏一帧刷新一次
            Sleep(1000 / 60 - delta_time);
        }
    }
}

void WidgetManager::drawAllWidgetBase_()const {
    for(list<WidgetBase*>::const_iterator it = widget_bases_.cbegin(); it != widget_bases_.cend(); ++it) {
        (*it)->drawWidget();
    }
}

void WidgetManager::emitEventMouseMove_(const int mouse_x, const int mouse_y) {
    for(list<WidgetBase*>::iterator it = widget_bases_.begin(); it != widget_bases_.end(); ++it) {
        (*it)->eventMouseMove(mouse_x, mouse_y);
    }
}
