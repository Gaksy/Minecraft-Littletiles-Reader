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
#ifndef GALIB_EASYXSUPPORT_WIDGETMANAGER_H
#define GALIB_EASYXSUPPORT_WIDGETMANAGER_H

#include <list>
#include "GalibNamespaceDef.h"

namespace galib::easy_x {
    class WidgetBase;
    class WidgetManager {
    public:
        WidgetManager(int width, int height);
        virtual ~WidgetManager()=default;

    public:
        void setWindowsSize(int width, int height);
        void setWindowsWidth(int width);
        void setWindowsHeight(int height);

        GALIB_NODISCARD int getWindowsWidth()const;
        GALIB_NODISCARD int getWindowsHeight()const;

        void addWidgetBase(WidgetBase* p_widget_base);
        void removeWidgetBase(WidgetBase* p_widget_base);

        void start();
    private:
        void drawAllWidgetBase_()const;

        void emitEventMouseMove_(int mouse_x, int mouse_y);

    private:
        int window_width_;
        int window_height_;
        GALIB_STD list<WidgetBase*> widget_bases_;
    };
}

#endif //GALIB_EASYXSUPPORT_WIDGETMANAGER_H
