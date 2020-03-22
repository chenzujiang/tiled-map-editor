/*
 * objectgroupitem.h
 * Copyright 2009, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
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

#pragma once

#include "layeritem.h"

#include "objectgroup.h"

namespace Tiled {
namespace Internal {

/**
 * A graphics item representing an object group in a QGraphicsView. It only
 * serves to group together the objects belonging to the same object group.
 *
 * @see MapObjectItem
 */
class ObjectGroupItem : public LayerItem
{
public:
    ObjectGroupItem(ObjectGroup *objectGroup, QGraphicsItem *parent = nullptr);

    ObjectGroup *objectGroup() const;

    // QGraphicsItem
    QRectF boundingRect() const override;
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;
};

inline ObjectGroup *ObjectGroupItem::objectGroup() const
{
    return static_cast<ObjectGroup*>(layer());
}

} // namespace Internal
} // namespace Tiled
