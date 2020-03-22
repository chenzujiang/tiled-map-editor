/*
 * changepolygon.h
 * Copyright 2011, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
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

#include <QPolygonF>
#include <QUndoCommand>

namespace Tiled {

class MapObject;

namespace Internal {

class MapDocument;

/**
 * Changes the polygon of a MapObject.
 *
 * This class expects the polygon to be already changed, and takes the previous
 * polygon in the constructor.
 */
class ChangePolygon : public QUndoCommand
{
public:
    ChangePolygon(MapDocument *mapDocument,
                  MapObject *mapObject,
                  const QPolygonF &oldPolygon);

    ChangePolygon(MapDocument *mapDocument,
                  MapObject *mapObject,
                  const QPolygonF &newPolygon,
                  const QPolygonF &oldPolygon);

    void undo() override;
    void redo() override;

private:
    MapDocument *mMapDocument;
    MapObject *mMapObject;

    QPolygonF mOldPolygon;
    QPolygonF mNewPolygon;
    bool mOldChangeState;
};

class TogglePolygonPolyline : public QUndoCommand
{
public:
    TogglePolygonPolyline(MapObject *mapObject);

    void undo() override { toggle(); }
    void redo() override { toggle(); }

private:
    void toggle();

    MapObject *mMapObject;
};

class SplitPolyline : public QUndoCommand
{
public:
    SplitPolyline(MapDocument *mapDocument,
                  MapObject *mapObject,
                  int edgeIndex);

    void undo() override;
    void redo() override;

private:
    MapDocument *mMapDocument;
    MapObject *mFirstPolyline;
    MapObject *mSecondPolyline;

    int mEdgeIndex;
    int mObjectIndex;
    bool mOldChangeState;
};

} // namespace Internal
} // namespace Tiled
