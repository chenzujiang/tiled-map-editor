/*
 * clickablelabel.cpp
 * Copyright 2016, Ava Brumfield <alturos@gmail.com>
 *
 * This file is part of Tiled.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "clickablelabel.h"

using namespace Tiled;
using namespace Tiled::Internal;

ClickableLabel::ClickableLabel(QWidget *parent) :
    QLabel(parent)
{
}

void ClickableLabel::mouseMoveEvent(QMouseEvent *event)
{
    emit mouseMoved(event);
}

void ClickableLabel::mousePressEvent(QMouseEvent *event)
{
    emit mousePressed(event);
}

void ClickableLabel::mouseReleaseEvent(QMouseEvent *event)
{
    emit mouseReleased(event);
}
