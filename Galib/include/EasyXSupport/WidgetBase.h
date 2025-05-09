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
#ifndef GALIB_EASYXSUPPORT_WIDGETBASE_H
#define GALIB_EASYXSUPPORT_WIDGETBASE_H

#include "GalibNamespaceDef.h"
#include "WidgetManager.h"
#include "Coord/Coord2D.h"

namespace galib::easy_x {
    class WidgetBase {
    public:
        explicit WidgetBase(WidgetManager* p_parent = nullptr);
        explicit WidgetBase(const GALIB coord::Coordinate2D<int>& kDisplayPos, WidgetManager* p_parent);
        virtual ~WidgetBase() = default;


        void setDisplayPos(const GALIB coord::Coordinate2D<int>& kDisplayPos);
        void setDisplayPos(int x, int y);
        GALIB_NODISCARD const GALIB coord::Coordinate2D<int>& getDisplayPos()const;

        void setWidgetSize(int width, int height);
        void setWidgetWidth(int width);
        void setWidgetHeight(int height);
        GALIB_NODISCARD int getWidgetWidth()const;
        GALIB_NODISCARD int getWidgetHeight()const;

        virtual void drawWidget()const=0;

        void setParent(WidgetManager* p_parent);
        void removeParent();

    protected:
        GALIB coord::Coordinate2D<int> display_pos_;
        WidgetManager* p_widget_manager_;
        int width_;
        int height_;
    };
}

#endif //GALIB_EASYXSUPPORT_WIDGETBASE_H
