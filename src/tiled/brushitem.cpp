/*
 * brushitem.cpp
 * Copyright 2008-2017, Thorbj酶rn Lindeijer <thorbjorn@lindeijer.nl>
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

#include "brushitem.h"

#include "map.h"
#include "mapdocument.h"
#include "maprenderer.h"
#include "painttilelayer.h"
#include "tile.h"
#include "tilelayer.h"

#include <QApplication>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QUndoStack>

using namespace Tiled;
using namespace Tiled::Internal;

BrushItem::BrushItem():
    mMapDocument(nullptr)
{
    setFlag(QGraphicsItem::ItemUsesExtendedStyleOption);
}

/**设置这个brush正在操作的文档
 * Sets the map document this brush is operating on.
 */
void BrushItem::setMapDocument(MapDocument *mapDocument)
{
    if (mMapDocument == mapDocument)
        return;

    mMapDocument = mapDocument;

    // The tiles in the stamp may no longer be valid邮票上的瓷砖可能不再有效
    clear();
}

/**
 * Clears the tile layer, map and region set on this item.
 */
void BrushItem::clear()
{
    mTileLayer.clear();
    mMap.clear();
    mRegion = QRegion();

    updateBoundingRect();
    update();
}

/**设置了平铺图层代表这个画刷(设置一个表示这个笔刷的平铺层)，当不设置任何平铺图层时，画刷只画选择的颜色
 * Sets a tile layer representing this brush. When no tile layer is set,
 * the brush only draws the selection color.
 */
void BrushItem::setTileLayer(const SharedTileLayer &tileLayer)
{
    mTileLayer = tileLayer;
    mRegion = tileLayer ? tileLayer->region() : QRegion();

    updateBoundingRect();
    update();
}

/**设置一个平铺图层以及应该突出显示的区域。这允许高亮的区域不能被瓦砖的指定瓦砖层覆盖
 * Sets a tile layer as well as the region that should be highlighted along
 * with it. This allows highlighting of areas that are not covered by tiles in
 * the given tile layer.
 */
void BrushItem::setTileLayer(const SharedTileLayer &tileLayer,
                             const QRegion &region)
{
    mTileLayer = tileLayer;
    mRegion = region;

    updateBoundingRect();
    update();
}

void BrushItem::setMap(const SharedMap &map)
{
    mMap = map;
    mRegion = map->tileRegion();

    updateBoundingRect();
    update();
}

/**
 * Changes the position of the tile layer, if one is set.
 */
void BrushItem::setTileLayerPosition(const QPoint &pos)
{
    if (!mTileLayer)
        return;

    const QPoint oldPosition(mTileLayer->x(), mTileLayer->y());
    if (oldPosition == pos)
        return;

    mRegion.translate(pos - oldPosition);
    mTileLayer->setX(pos.x());
    mTileLayer->setY(pos.y());
    updateBoundingRect();
}

/**
 * Sets the region of tiles that this brush item occupies.
 */
void BrushItem::setTileRegion(const QRegion &region)
{
    if (mRegion == region)
        return;

    mRegion = region;
    updateBoundingRect();
}

/**
 * Sets the layer offset used by the currently active layer.
 */
void BrushItem::setLayerOffset(const QPointF &offset)
{
    setPos(offset);
}

QRectF BrushItem::boundingRect() const
{
    return mBoundingRect;
}

void BrushItem::paint(QPainter *painter,
                      const QStyleOptionGraphicsItem *option,
                      QWidget *)
{
    QColor insideMapHighlight = QApplication::palette().highlight().color();
    insideMapHighlight.setAlpha(64);
    QColor outsideMapHighlight = QColor(255, 0, 0, 64);

    QRegion insideMapRegion = mRegion;
    QRegion outsideMapRegion;

    if (!mMapDocument->currentLayer()->isUnlocked()) {
        qSwap(insideMapRegion, outsideMapRegion);
    } else if (!mMapDocument->map()->infinite()) {
        int mapWidth = mMapDocument->map()->width();
        int mapHeight = mMapDocument->map()->height();
        QRegion mapRegion = QRegion(0, 0, mapWidth, mapHeight);

        insideMapRegion = mRegion.intersected(mapRegion);
        outsideMapRegion = mRegion.subtracted(mapRegion);
    }

    const MapRenderer *renderer = mMapDocument->renderer();
    if (mTileLayer) {
        const qreal opacity = painter->opacity();
        painter->setOpacity(0.75);
        renderer->drawTileLayer(painter, mTileLayer.data(), option->exposedRect);
        painter->setOpacity(opacity);
    } else if (mMap) {
        const qreal opacity = painter->opacity();
        painter->setOpacity(0.75);
        LayerIterator it(mMap.data(), Layer::TileLayerType);
        while (auto tileLayer = static_cast<TileLayer*>(it.next()))
            renderer->drawTileLayer(painter, tileLayer, option->exposedRect);
        painter->setOpacity(opacity);
    }

    renderer->drawTileSelection(painter, insideMapRegion,
                                insideMapHighlight,
                                option->exposedRect);
    renderer->drawTileSelection(painter, outsideMapRegion,
                                outsideMapHighlight,
                                option->exposedRect);
}

void BrushItem::updateBoundingRect()
{
    prepareGeometryChange();

    if (!mMapDocument) {
        mBoundingRect = QRectF();
        return;
    }

    const QRect bounds = mRegion.boundingRect();
    mBoundingRect = mMapDocument->renderer()->boundingRect(bounds);

    QMargins drawMargins;

    // Adjust for amount of pixels tiles extend at the top and to the right
    if (mTileLayer) {
        drawMargins = mTileLayer->drawMargins();

        QSize tileSize = mMapDocument->map()->tileSize();
        drawMargins.setTop(drawMargins.top() - tileSize.height());
        drawMargins.setRight(drawMargins.right() - tileSize.width());
    } else if (mMap) {
        drawMargins = mMap->drawMargins();
    } else {
        return;
    }

    // Since we're also drawing a tile selection, we should not apply
    // negative margins
    mBoundingRect.adjust(qMin(0, -drawMargins.left()),
                         qMin(0, -drawMargins.top()),
                         qMax(0, drawMargins.right()),
                         qMax(0, drawMargins.bottom()));
}
