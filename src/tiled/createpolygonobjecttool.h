/*
 * createpolygonobjecttool.h
 * Copyright 2014, Martin Ziel <martin.ziel.com>
 * Copyright 2015-2018, Thorbjørn Lindeijer <bjorn@lindeijer.nl>
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

#include "createobjecttool.h"

namespace Tiled {

namespace Internal {

class PointHandle;

class CreatePolygonObjectTool : public CreateObjectTool
{
    Q_OBJECT

public:
    CreatePolygonObjectTool(QObject *parent);
    ~CreatePolygonObjectTool() override;

    void activate(MapScene *scene) override;
    void deactivate(MapScene *scene) override;

    void keyPressed(QKeyEvent *event) override;
    void mouseMoved(const QPointF &pos,
                    Qt::KeyboardModifiers modifiers) override;
    void mousePressed(QGraphicsSceneMouseEvent *event) override;

    void languageChanged() override;

    void extend(MapObject *mapObject, bool extendingFirst);

protected:
    void mouseMovedWhileCreatingObject(const QPointF &pos,
                                       Qt::KeyboardModifiers modifiers) override;
    void mousePressedWhileCreatingObject(QGraphicsSceneMouseEvent *event) override;

    bool startNewMapObject(const QPointF &pos, ObjectGroup *objectGroup) override;
    MapObject *createNewMapObject() override;
    void cancelNewMapObject() override;
    void finishNewMapObject() override;
    MapObject *clearNewMapObjectItem() override;

private slots:
    void updateHover(const QPointF &scenePos, QGraphicsSceneMouseEvent *event = nullptr);
    void updateHandles();

    void objectsChanged(const QList<MapObject *> &objects);
    void objectsRemoved(const QList<MapObject *> &objects);

    void layerRemoved(Layer *layer);

private:
    enum Mode {
        NoMode,
        Creating,
        ExtendingAtBegin,
        ExtendingAtEnd,
    };

    void languageChangedImpl();

    void finishExtendingMapObject();
    void abortExtendingMapObject();

    void synchronizeOverlayObject();

    void setHoveredHandle(PointHandle *handle);

    MapObject *mOverlayPolygonObject;
    ObjectGroup *mOverlayObjectGroup;
    MapObjectItem *mOverlayPolygonItem;
    QPointF mLastPixelPos;
    Mode mMode;
    bool mFinishAsPolygon;

    /// The handles associated with polygon points of selected map objects
    QList<PointHandle*> mHandles;
    PointHandle *mHoveredHandle;
    PointHandle *mClickedHandle;
};

} // namespace Internal
} // namespace Tiled
