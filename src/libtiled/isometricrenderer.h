/*
 * isometricrenderer.h
 * Copyright 2009-2010, Thorbj酶rn Lindeijer <thorbjorn@lindeijer.nl>
 *
 * This file is part of libtiled.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "maprenderer.h"

namespace Tiled {

/**
 * An isometric map renderer.
 *
 * Isometric maps have diamond shaped tiles. This map renderer renders them in
 * such a way that the map will also be diamond shaped. The X axis points to
 * the bottom right while the Y axis points to the bottom left.
 * 等距离渲染器有菱形瓦砖。这张地图也将以菱形的方法呈现渲染器渲染的效果，x指向右下放，y指向做下方
 */
class TILEDSHARED_EXPORT IsometricRenderer : public MapRenderer
{
public:
    IsometricRenderer(const Map *map) : MapRenderer(map) {}

    QRect mapBoundingRect() const override;

    QRect boundingRect(const QRect &rect) const override;

    QRectF boundingRect(const MapObject *object) const override;
    QPainterPath shape(const MapObject *object) const override;

    void drawGrid(QPainter *painter, const QRectF &rect, QColor grid) const override;

    void drawTileLayer(QPainter *painter, const TileLayer *layer,
                       const QRectF &exposed = QRectF()) const override;

    void drawTileSelection(QPainter *painter,
                           const QRegion &region,
                           const QColor &color,
                           const QRectF &exposed) const override;

    void drawMapObject(QPainter *painter,
                       const MapObject *object,
                       const QColor &color) const override;

    using MapRenderer::pixelToTileCoords;
    QPointF pixelToTileCoords(qreal x, qreal y) const override;

    using MapRenderer::tileToPixelCoords;
    QPointF tileToPixelCoords(qreal x, qreal y) const override;

    using MapRenderer::screenToTileCoords;
    QPointF screenToTileCoords(qreal x, qreal y) const override;

    using MapRenderer::tileToScreenCoords;
    QPointF tileToScreenCoords(qreal x, qreal y) const override;

    using MapRenderer::screenToPixelCoords;
    QPointF screenToPixelCoords(qreal x, qreal y) const override;

    using MapRenderer::pixelToScreenCoords;
    QPointF pixelToScreenCoords(qreal x, qreal y) const override;

private:
    QPolygonF pixelRectToScreenPolygon(const QRectF &rect) const;
    QPolygonF tileRectToScreenPolygon(const QRect &rect) const;
};

} // namespace Tiled
