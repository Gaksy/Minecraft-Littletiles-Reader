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
#include "Coord/Coord2D.h"

namespace galib::easy_x {
    class WidgetBase {
    public:
        WidgetBase();
        ~WidgetBase();

        void setDisplayPos(const GALIB coord::Coordinate2D<int>& kDisplayPos);
        const GALIB coord::Coordinate2D<int>& getDisplayPos();

        void setWidgetSize(unsigned int width, unsigned int height);
        void setWidgetWidth(unsigned int width);
        void setWidgetHeight(unsigned int height);
        void getWidgetWidth();
        void getWidgetHeight();

        virtual void drawWidget()=0;
        virtual void getWidgetSize()=0;

    protected:
        GALIB coord::Coordinate2D<int> display_pos_;
    };
}

#endif //GALIB_EASYXSUPPORT_WIDGETBASE_H
