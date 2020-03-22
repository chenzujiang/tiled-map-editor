/*
 * mapobjectitem.h
 * Copyright 2008, Roderic Morris <roderic@ccs.neu.edu>
 * Copyright 2008-2011, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
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

#include <QCoreApplication>
#include <QGraphicsItem>

namespace Tiled {

class MapObject;

namespace Internal {

class Handle;
class MapDocument;
class ObjectGroupItem;
class PointHandle;
class ResizeHandle;

/**
 * A graphics item displaying a map object.
 */
class MapObjectItem : public QGraphicsItem
{
    Q_DECLARE_TR_FUNCTIONS(MapObjectItem)

public:
    /**
     * Constructor.
     *
     * @param object the object to be displayed
     * @param parent the item of the object group this object belongs to
     */
    MapObjectItem(MapObject *object, MapDocument *mapDocument,
                  QGraphicsItem *parent = nullptr);

    enum { Type = UserType + 1 };
    int type() const override { return Type; }

    MapObject *mapObject() const;

    /**
     * Should be called when the map object this item refers to was changed.
     */
    void syncWithMapObject();

    bool isHoverIndicator() const;
    void setIsHoverIndicator(bool isHoverIndicator);

    // QGraphicsItem
    QRectF boundingRect() const override;
    QPainterPath shape() const override;

    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;

    /**
     * Resizes the associated map object. The \a bounds are given in pixel
     * coordinates.
     */
    void resizeObject(const QRectF &bounds);

    /**
     * Sets a new polygon on the associated object.
     */
    void setPolygon(const QPolygonF &polygon);

    /**
     * A helper function to determine the color of a map object. The color is
     * determined first of all by the object type, and otherwise by the group
     * that the object is in. If still no color is defined, it defaults to
     * gray.
     */
    static QColor objectColor(const MapObject *object);

private:
    MapDocument *mapDocument() const { return mMapDocument; }
    QColor color() const { return mColor; }

    MapObject *mObject;
    MapDocument *mMapDocument;

    /** Bounding rect cached, for adapting to geometry change correctly. */
    QRectF mBoundingRect;
    QString mName;      // Copy of the name, so we know when it changes
    QPolygonF mPolygon; // Copy of the polygon, for the same reason
    QColor mColor;      // Cached color of the object
    bool mIsHoveredIndicator = false;
};


inline MapObject *MapObjectItem::mapObject() const
{
    return mObject;
}

inline bool MapObjectItem::isHoverIndicator() const
{
    return mIsHoveredIndicator;
}

} // namespace Internal
} // namespace Tiled

Q_DECLARE_METATYPE(Tiled::Internal::MapObjectItem*)
