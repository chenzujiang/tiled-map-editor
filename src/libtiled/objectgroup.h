/*
 * objectgroup.h
 * Copyright 2008, Roderic Morris <roderic@ccs.neu.edu>
 * Copyright 2008-2014, Thorbj酶rn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyright 2009-2010, Jeff Bland <jksb@member.fsf.org>
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

#include "tiled_global.h"

#include "layer.h"

#include <QColor>
#include <QList>
#include <QMetaType>

namespace Tiled {

class MapObject;

/**
 * A group of objects on a map.地图上的一组对象
 */
class TILEDSHARED_EXPORT ObjectGroup : public Layer
{
public:
    /**
     * Objects within an object group can either be drawn top down (sorted
     * by their y-coordinate) or by index (manual stacking order).
     *
     * The default is top down.
     * 对象组中的对象可以自上而下地绘制（按其y坐标排序），也可以通过索引（手动叠加顺序）。默认值是自上而下的。
     */
    enum DrawOrder {
        UnknownOrder = -1,
        TopDownOrder,
        IndexOrder
    };

    ObjectGroup();
    ObjectGroup(const QString &name, int x, int y);

    ~ObjectGroup() override;

    /**返回一个指针,这个指针指向一个对象组对象列表
     * Returns a pointer to the list of objects in this object group.
     */
    const QList<MapObject*> &objects() const { return mObjects; }

    /**返回这个对象组中有多少object数量
     * Returns the number of objects in this object group.
     */
    int objectCount() const { return mObjects.size(); }

    /**根据索引返回指定对象
     * Returns the object at the specified index.
     */
    MapObject *objectAt(int index) const { return mObjects.at(index); }

    /**
     * Adds an object to this object group.
     */
    void addObject(MapObject *object);

    /**
     * Inserts an object at the specified index. This is only used for undoing
     * the removal of an object at the moment, to make sure not to change the
     * saved order of the objects.插入一个对象到指定索引位置,这只用于取消当前物体的移除，
     * 以确保不会改变所保存的对象的顺序。
     */
    void insertObject(int index, MapObject *object);

    /**
     * Removes an object from this object group. Ownership of归属权 the object is transferred转移 to the caller.
     *
     * @return the index at which the specified object was removed返回指定对象被删除的索引
     */
    int removeObject(MapObject *object);

    /**
     * Removes the object at the given index. Ownership of the object is
     * transferred to the caller.
     *当你已经得到索引时，这比removeObject快。
     * This is faster than removeObject when you've already got the index.
     *
     * @param index the index at which to remove an object
     */
    void removeObjectAt(int index);

    /**
     * Moves \a count objects starting at \a from to the index given by \a to.
     *
     * The \a to index may not lie within the range of objects that is
     * being moved.
     * to index 可能不在对象范围内
     */
    void moveObjects(int from, int to, int count);

    /**返回这个对象组中所有对象的边界矩形
     * Returns the bounding rect around all objects in this object group.
     */
    QRectF objectsBoundingRect() const;

    /**返回这个对象组中是否有任何对象内容
     * Returns whether this object group contains any objects.
     */
    bool isEmpty() const override;

    /**：计算并返回该对象组使用的tilesets。
     * Computes and returns the set of tilesets used by this object group.
     */
    QSet<SharedTileset> usedTilesets() const override;

    /**返回在给定的tileset中，对象组中是否有任何tile对象。
     * Returns whether any tile objects in this object group reference tiles
     * in the given tileset.
     */
    bool referencesTileset(const Tileset *tileset) const override;

    /**
     * Replaces all references to tiles from \a oldTileset with tiles from
     * \a newTileset.用新tileset的瓷砖替换旧Tileset的所有对tile的引用
     */
    void replaceReferencesToTileset(Tileset *oldTileset,
                                    Tileset *newTileset) override;

    /**
     * Offsets all objects within the group by the \a offset given in pixel
     * coordinates, and optionally wraps them. The object's center must be
     * within \a bounds, and wrapping occurs if the displaced center is out of
     * the bounds.
     * 通过在像素坐标中给出的偏移量来补偿组内的所有对象，并可选地包装它们。
     * 对象的中心必须在一个范围内，如果移位的中心超出了界限，就会发生包装。
     *
     * \sa TileLayer::offsetTiles()
     */
    void offsetObjects(const QPointF &offset, const QRectF &bounds,
                       bool wrapX, bool wrapY);

    bool canMergeWith(Layer *other) const override;
    Layer *mergedWith(Layer *other) const override;

    const QColor &color() const;
    void setColor(const QColor &color);

    DrawOrder drawOrder() const;
    void setDrawOrder(DrawOrder drawOrder);

    ObjectGroup *clone() const override;

    void resetObjectIds();
    int highestObjectId() const;

    // Enable easy iteration over objects with range-based for允许对具有基于范围的物体进行简单的迭代
    QList<MapObject*>::iterator begin() { return mObjects.begin(); }
    QList<MapObject*>::iterator end() { return mObjects.end(); }
    QList<MapObject*>::const_iterator begin() const { return mObjects.begin(); }
    QList<MapObject*>::const_iterator end() const { return mObjects.end(); }

protected:
    ObjectGroup *initializeClone(ObjectGroup *clone) const;

private:
    QList<MapObject*> mObjects;
    QColor mColor;
    DrawOrder mDrawOrder;
};


/**
 * Returns the color of the object group, or an invalid无效 color if no color
 * is set.
 */
inline const QColor &ObjectGroup::color() const
{ return mColor; }

/**
 * Sets the display color of the object group.
 */
inline void ObjectGroup::setColor(const QColor &color)
{ mColor = color; }

/**
 * Returns the draw order for the objects in this group.
 *
 * \sa ObjectGroup::DrawOrder
 */
inline ObjectGroup::DrawOrder ObjectGroup::drawOrder() const
{ return mDrawOrder; }

/**
 * Sets the draw order for the objects in this group.
 *
 * \sa ObjectGroup::DrawOrder
 */
inline void ObjectGroup::setDrawOrder(DrawOrder drawOrder)
{ mDrawOrder = drawOrder; }


/**
 * Helper function that converts转换 a drawing order to its string value. Useful
 * for map writers.
 *
 * @return The draw order as a lowercase string.绘制顺序为一个小写的字符串
 */
TILEDSHARED_EXPORT QString drawOrderToString(ObjectGroup::DrawOrder);

/**帮助函数将字符串转换成绘图顺序枚举器。
 * Helper function that converts a string to a drawing order enumerator.
 * Useful for map readers.
 *
 * @return The draw order matching the given string, or返回与给定字符串匹配的绘制顺序
 *         ObjectGroup::UnknownOrder if the string is unrecognized.：如果字符串未被识别，则不知道。
 */
TILEDSHARED_EXPORT ObjectGroup::DrawOrder drawOrderFromString(const QString &);

} // namespace Tiled

Q_DECLARE_METATYPE(Tiled::ObjectGroup*)
