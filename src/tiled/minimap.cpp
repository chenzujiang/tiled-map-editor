/*
 * minimap.cpp
 * Copyright 2012, Christoph Schnackenberg <bluechs@gmx.de>
 * Copyright 2012, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
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
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "minimap.h"

#include "documentmanager.h"
#include "map.h"
#include "mapdocument.h"
#include "maprenderer.h"
#include "mapscene.h"
#include "mapview.h"
#include "utils.h"
#include "zoomable.h"

#include <QCursor>
#include <QResizeEvent>
#include <QScrollBar>
#include <QUndoStack>

using namespace Tiled;
using namespace Tiled::Internal;

MiniMap::MiniMap(QWidget *parent)
    : QFrame(parent)
    , mMapDocument(nullptr)
    , mDragging(false)
    , mMouseMoveCursorState(false)
    , mRedrawMapImage(false)
    , mRenderFlags(MiniMapRenderer::DrawTileLayers
                   | MiniMapRenderer::DrawMapObjects
                   | MiniMapRenderer::DrawImageLayers
                   | MiniMapRenderer::IgnoreInvisibleLayer
                   | MiniMapRenderer::SmoothPixmapTransform)
{
    setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    setMinimumSize(50, 50);

    // for cursor changes
    setMouseTracking(true);

    mMapImageUpdateTimer.setSingleShot(true);
    connect(&mMapImageUpdateTimer, &QTimer::timeout,
            this, &MiniMap::redrawTimeout);
}

void MiniMap::setMapDocument(MapDocument *map)
{
    const DocumentManager *dm = DocumentManager::instance();

    if (mMapDocument) {
        mMapDocument->disconnect(this);

        if (MapView *mapView = dm->viewForDocument(mMapDocument)) {
            mapView->zoomable()->disconnect(this);
            mapView->horizontalScrollBar()->disconnect(this);
            mapView->verticalScrollBar()->disconnect(this);
        }
    }

    mMapDocument = map;

    if (mMapDocument) {
        connect(mMapDocument->undoStack(), &QUndoStack::indexChanged,
                this, &MiniMap::scheduleMapImageUpdate);

        if (MapView *mapView = dm->viewForDocument(mMapDocument)) {
            connect(mapView->horizontalScrollBar(), &QAbstractSlider::valueChanged, this, [this] { update(); });
            connect(mapView->verticalScrollBar(), &QAbstractSlider::valueChanged, this, [this] { update(); });
            connect(mapView->zoomable(), &Zoomable::scaleChanged, this, [this] { update(); });
        }
    }

    scheduleMapImageUpdate();
}

QSize MiniMap::sizeHint() const
{
    return Utils::dpiScaled(QSize(200, 200));
}
//���ŵ�ͼͼƬ�ĸ���
void MiniMap::scheduleMapImageUpdate()
{
    mMapImageUpdateTimer.start(100);
}

void MiniMap::paintEvent(QPaintEvent *pe)
{
    QFrame::paintEvent(pe);

    if (mRedrawMapImage) {
        renderMapToImage();
        mRedrawMapImage = false;
    }

    if (mMapImage.isNull() || mImageRect.isEmpty())
        return;

    QPainter p(this);
    p.setRenderHints(QPainter::SmoothPixmapTransform);

    QColor backgroundColor(palette().dark().color());
    if (mMapDocument && mMapDocument->map()->backgroundColor().isValid())
        backgroundColor = mMapDocument->map()->backgroundColor();
    p.setBrush(backgroundColor);
    p.setPen(Qt::NoPen);
    p.drawRect(contentsRect());

    p.drawImage(mImageRect, mMapImage);

    const QRect viewRect = viewportRect();
    p.setBrush(Qt::NoBrush);
    p.setPen(QColor(0, 0, 0, 128));
    p.translate(1, 1);
    p.drawRect(viewRect);

    QPen outLinePen(QColor(255, 0, 0), 2);
    outLinePen.setJoinStyle(Qt::MiterJoin);
    p.translate(-1, -1);
    p.setPen(outLinePen);
    p.drawRect(viewRect);
}

void MiniMap::resizeEvent(QResizeEvent *)
{
    updateImageRect();
    scheduleMapImageUpdate();
}

void MiniMap::updateImageRect()
{
    QRect imageRect = mMapImage.rect();
    if (imageRect.isEmpty()) {
        mImageRect = QRect();
        return;
    }

    // Scale and center the image
    const QRect r = contentsRect();
    qreal scale = qMin((qreal) r.width() / imageRect.width(),
                       (qreal) r.height() / imageRect.height());
    imageRect.setSize(imageRect.size() * scale);
    imageRect.moveCenter(r.center());

    mImageRect = imageRect;
}
//�����ͼӰ��
void MiniMap::renderMapToImage()
{
    if (!mMapDocument) {
        mMapImage = QImage();
        return;
    }

    MapRenderer *renderer = mMapDocument->renderer();
#if QT_VERSION >= 0x050600
    const QSize viewSize = contentsRect().size() * devicePixelRatioF();
#else
    const QSize viewSize = contentsRect().size() * devicePixelRatio();
#endif
    QSize mapSize = renderer->mapBoundingRect().size();

    if (mapSize.isEmpty()) {
        mMapImage = QImage();
        return;
    }

    // Determine the largest possible scale
    qreal scale = qMin((qreal) viewSize.width() / mapSize.width(),
                       (qreal) viewSize.height() / mapSize.height());

    // Allocate a new image when the size changed
    const QSize imageSize = mapSize * scale;
    if (mMapImage.size() != imageSize) {
        mMapImage = QImage(imageSize, QImage::Format_ARGB32_Premultiplied);
        updateImageRect();
    }

    if (imageSize.isEmpty())
        return;

    MiniMapRenderer miniMapRenderer(mMapDocument->map());
    miniMapRenderer.renderToImage(mMapImage, mRenderFlags);
}
//������ͼ�ھֲ�������
void MiniMap::centerViewOnLocalPixel(QPoint centerPos, int delta)
{
    MapView *mapView = DocumentManager::instance()->currentMapView();
    if (!mapView)
        return;

    if (delta != 0)
        mapView->zoomable()->handleWheelDelta(delta);

    mapView->forceCenterOn(mapToScene(centerPos));
}

void MiniMap::redrawTimeout()
{
    mRedrawMapImage = true;
    update();
}
//ת���¼�
void MiniMap::wheelEvent(QWheelEvent *event)
{
    if (event->orientation() == Qt::Vertical) {
        centerViewOnLocalPixel(event->pos(), event->delta());
        return;
    }

    QFrame::wheelEvent(event);
}

void MiniMap::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QPoint cursorPos = event->pos();
        QRect viewPort = viewportRect();

        if (viewPort.contains(cursorPos)) {
            mDragOffset = viewPort.center() - cursorPos + QPoint(1, 1);
            cursorPos += mDragOffset;
        } else {
            mDragOffset = QPoint();
            centerViewOnLocalPixel(cursorPos);
        }

        mDragging = true;
        setCursor(Qt::ClosedHandCursor);

        return;
    }

    QFrame::mouseReleaseEvent(event);
}

void MiniMap::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && mDragging) {
        mDragging = false;

        QRect viewPort = viewportRect();
        if (viewPort.contains(event->pos())) {
            setCursor(Qt::OpenHandCursor);
            mMouseMoveCursorState = true;
        } else if (rect().contains(event->pos())) {
            unsetCursor();
            mMouseMoveCursorState = false;
        }

        return;
    }

    QFrame::mouseReleaseEvent(event);
}

void MiniMap::mouseMoveEvent(QMouseEvent *event)
{
    if (mDragging) {
        centerViewOnLocalPixel(event->pos() + mDragOffset);
        return;
    }

    if (viewportRect().contains(event->pos())) {
        if (!mMouseMoveCursorState) {
            setCursor(Qt::OpenHandCursor);
            mMouseMoveCursorState = true;
        }
    } else {
        if (mMouseMoveCursorState) {
            unsetCursor();
            mMouseMoveCursorState = false;
        }
    }

    QFrame::mouseMoveEvent(event);
}

QRect MiniMap::viewportRect() const
{
    MapView *mapView = DocumentManager::instance()->currentMapView();
    if (!mapView)
        return QRect(0, 0, 1, 1);

    const QRectF sceneRect = mapView->mapScene()->sceneRect();
    const QRectF viewRect = mapView->mapToScene(mapView->viewport()->geometry()).boundingRect();
    return QRect((viewRect.x() - sceneRect.x()) / sceneRect.width() * mImageRect.width() + mImageRect.x(),
                 (viewRect.y() - sceneRect.y()) / sceneRect.height() * mImageRect.height() + mImageRect.y(),
                 viewRect.width() / sceneRect.width() * mImageRect.width(),
                 viewRect.height() / sceneRect.height() * mImageRect.height());
}

QPointF MiniMap::mapToScene(QPoint p) const
{
    if (mImageRect.isEmpty())
        return QPointF();

    MapView *mapView = DocumentManager::instance()->currentMapView();
    if (!mapView)
        return QPointF();

    const QRectF sceneRect = mapView->mapScene()->sceneRect();
    p -= mImageRect.topLeft();
    return QPointF(p.x() * (sceneRect.width() / mImageRect.width()) + sceneRect.x(),
                   p.y() * (sceneRect.height() / mImageRect.height()) + sceneRect.y());
}
