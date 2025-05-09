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

#include "EasyXSupport/WidgetBase.h"

using GALIB easy_x::WidgetBase;
using GALIB coord::Coordinate2D;

WidgetBase::WidgetBase(WidgetManager* p_parent):
    p_widget_manager_(p_parent),
    display_pos_({0,0}),
    width_(0),
    height_(0)
{
    if(p_widget_manager_) {
        p_widget_manager_->addWidgetBase(this);
    }
}

WidgetBase::WidgetBase(const Coordinate2D<int>& kDisplayPos, WidgetManager* p_parent):
    p_widget_manager_(p_parent),
    display_pos_(kDisplayPos),
    width_(0),
    height_(0)
{
    if(p_widget_manager_) {
        p_widget_manager_->addWidgetBase(this);
    }
}

void WidgetBase::setDisplayPos(const Coordinate2D<int> &kDisplayPos) {
    display_pos_ = kDisplayPos;
}

void WidgetBase::setDisplayPos(const int x, const int y) {
    display_pos_ = {x, y};
}

const Coordinate2D<int> &WidgetBase::getDisplayPos()const {
    return display_pos_;
}

void WidgetBase::setWidgetWidth(const int width) {
    width_ = width;
}

void WidgetBase::setWidgetHeight(const int height) {
    height_ = height;
}

void WidgetBase::setWidgetSize(const int width, const int height) {
    width_ = width;
    height_ = height;
}

int WidgetBase::getWidgetWidth() const {
    return width_;
}

int WidgetBase::getWidgetHeight() const {
    return height_;
}

void WidgetBase::setParent(WidgetManager *p_parent) {
    if (p_widget_manager_) {
        p_widget_manager_->removeWidgetBase(this);
    }
    p_widget_manager_ = p_parent;
    if(p_widget_manager_) {
        p_widget_manager_->addWidgetBase(this);
    }
}

void WidgetBase::removeParent() {
    if(p_widget_manager_) {
        p_widget_manager_->removeWidgetBase(this);
    }
    p_widget_manager_ = nullptr;
}

