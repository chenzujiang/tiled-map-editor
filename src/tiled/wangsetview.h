/*
 * wangsetview.h
 * Copyright 2017, Benjamin Trotter <bdtrotte@ucsc.edu>
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

#include "wangsetmodel.h"

#include <QTreeView>

namespace Tiled {
namespace Internal {

class TilesetDocument;
class Zoomable;

class WangSetView : public QTreeView
{
    Q_OBJECT

public:
    WangSetView(QWidget *parent = nullptr);

    void setTilesetDocument(TilesetDocument *tilesetDocument);

    Zoomable *zoomable() const { return mZoomable; }

    WangSet *wangSetAt(const QModelIndex &index) const;

protected:
    bool event(QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    void editWangSetProperties();

    void adjustScale();

private:
    Zoomable *mZoomable;
    TilesetDocument *mTilesetDocument;
};

} // namespace Internal
} // namespace Tiled

Q_DECLARE_METATYPE(Tiled::Internal::WangSetView *)
