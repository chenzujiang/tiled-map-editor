/*
 * layer.h
 * Copyright 2008-2010, Thorbj酶rn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyright 2009, Jeff Bland <jeff@teamphobic.com>
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

#include "object.h"
#include "tileset.h"

#include <QPixmap>
#include <QRect>
#include <QSet>
#include <QString>
#include <QVector>

namespace Tiled {

class GroupLayer;
class Map;
class ImageLayer;
class ObjectGroup;
class TileLayer;

/**
 * A map layer.图层
 */
class TILEDSHARED_EXPORT Layer : public Object
{
public:
    enum TypeFlag {
        TileLayerType   = 0x01,
        ObjectGroupType = 0x02,
        ImageLayerType  = 0x04,
        GroupLayerType  = 0x08
    };

    enum { AnyLayerType = 0xFF };

    /**
     * Constructor.
     */
    Layer(TypeFlag type, const QString &name, int x, int y);

    /**
     * The layer ID can be used to unique identify this layer of the map. It
     * stays the same regardless of whether the layer is moved or renamed.
     */
    int id() const { return mId; }
    void setId(int id) { mId = id; }

    /**
     * Returns the type of this layer.
     */
    TypeFlag layerType() const { return mLayerType; }

    /**
     * Returns the name of this layer.
     */
    const QString &name() const { return mName; }

    /**
     * Sets the name of this layer.
     */
    void setName(const QString &name) { mName = name; }

    /**
     * Returns the opacity of this layer.返回图层透明度
     */
    qreal opacity() const { return mOpacity; }

    /**
     * Sets the opacity of this layer.
     */
    void setOpacity(qreal opacity) { mOpacity = opacity; }

    qreal effectiveOpacity() const;

    /**
     * Returns the visibility of this layer.
     */
    bool isVisible() const { return mVisible; }

    /**
     * Returns the lock status of current layer.
     */
    bool isLocked() const { return mLocked; }

    /**返回层的锁状态，包括父层。
     * Returns the lock status of layer including parent layers.
     */
    bool isUnlocked() const;

    bool isHidden() const;

    /**
     * Sets the visibility of this layer.
     */
    void setVisible(bool visible) { mVisible = visible; }

    void setLocked(bool locked) { mLocked = locked; }

    /**
     * Returns the map this layer is part of.
     */
    Map *map() const { return mMap; }

    /**如果有,返回父图层
     * Returns the parent layer, if any.
     */
    GroupLayer *parentLayer() const { return mParentLayer; }

    bool isParentOrSelf(const Layer *candidate) const;
    int depth() const;
    int siblingIndex() const;
    QList<Layer*> siblings() const;//图层兄妹

    /**
     * Returns the x position of this layer (in tiles).
     */
    int x() const { return mX; }

    /**
     * Sets the x position of this layer (in tiles).
     */
    void setX(int x) { mX = x; }

    /**
     * Returns the y position of this layer (in tiles).
     */
    int y() const { return mY; }

    /**
     * Sets the y position of this layer (in tiles).
     */
    void setY(int y) { mY = y; }

    /**
     * Returns the position of this layer (in tiles).
     */
    QPoint position() const { return QPoint(mX, mY); }

    /**
     * Sets the position of this layer (in tiles).
     */
    void setPosition(QPoint pos) { setPosition(pos.x(), pos.y()); }
    void setPosition(int x, int y) { mX = x; mY = y; }

    void setOffset(const QPointF &offset);
    QPointF offset() const;

    QPointF totalOffset() const;

    virtual bool isEmpty() const = 0;

    /**计算并返回这一层使用的一组tilesets。
     * Computes and returns the set of tilesets used by this layer.
     */
    virtual QSet<SharedTileset> usedTilesets() const = 0;

    /**返回该层是否引用给定的tileset。
     * Returns whether this layer is referencing the given tileset.
     */
    virtual bool referencesTileset(const Tileset *tileset) const = 0;

    /**
     * Replaces all references to tiles from \a oldTileset with tiles from
     * \a newTileset.
     */
    virtual void replaceReferencesToTileset(Tileset *oldTileset,
                                            Tileset *newTileset) = 0;

    /**
     * Returns whether this layer can merge together with the \a other layer.
     */
    virtual bool canMergeWith(Layer *other) const = 0;

    /**
     * Returns a newly allocated layer that is the result of merging this layer
     * with the \a other layer. Where relevant, the other layer is considered
     * to be on top of this one.
     *
     * Should only be called when canMergeWith returns true.
     */
    virtual Layer *mergedWith(Layer *other) const = 0;

    /**
     * Returns a duplicate of this layer. The caller is responsible for the
     * ownership of this newly created layer.
     */
    virtual Layer *clone() const = 0;

    // These functions allow checking whether this Layer is an instance of the
    // given subclass without relying on a dynamic_cast.
    //这些函数允许检查这一层是否是给定子类的实例，而不依赖于动态转换。
    bool isTileLayer() const { return mLayerType == TileLayerType; }
    bool isObjectGroup() const { return mLayerType == ObjectGroupType; }
    bool isImageLayer() const { return mLayerType == ImageLayerType; }
    bool isGroupLayer() const { return mLayerType == GroupLayerType; }

    // These actually return this layer cast to one of投给 its subclasses.这些实际上把这个图层返回给它的一个子类。
    TileLayer *asTileLayer();
    ObjectGroup *asObjectGroup();
    ImageLayer *asImageLayer();
    GroupLayer *asGroupLayer();

protected:
    /**
     * Sets the map this layer is part of. Should only be called from the
     * Map class.设置这个地图是这一层的一部分,应该只能从地图类中调用
     */
    virtual void setMap(Map *map) { mMap = map; }
    void setParentLayer(GroupLayer *groupLayer) { mParentLayer = groupLayer; }

    Layer *initializeClone(Layer *clone) const;

    QString mName;
    int mId;
    TypeFlag mLayerType;
    int mX;
    int mY;
    QPointF mOffset;
    qreal mOpacity;
    bool mVisible;
    Map *mMap;
    GroupLayer *mParentLayer;
    bool mLocked;

    friend class Map;
    friend class GroupLayer;
};


/**这这一层像素中,设置绘制的偏移量
 * Sets the drawing offset in pixels of this layer.
 */
inline void Layer::setOffset(const QPointF &offset)
{
    mOffset = offset;
}

/**
 * Returns the drawing offset in pixels of this layer.
 */
inline QPointF Layer::offset() const
{
    return mOffset;
}


/**迭代器,用于迭代这个地图的所有图层,按照他们绘制的顺序迭代。当向前迭代时,组图层的遍历顺序在他的子图层之后
 * An iterator for iterating over the layers of a map, in the order in which
 * they are drawn. When iterating forward, group layers are traversed after
 * their children.
 *
 * Modifying the layer hierarchy while an iterator is active will lead to
 * undefined results!当迭代器是活跃的时候,修改图层的层次结构,将导致未定义的结果
 */
class TILEDSHARED_EXPORT LayerIterator
{
public:
    LayerIterator(const Map *map, int layerTypes = Layer::AnyLayerType);
    LayerIterator(Layer *start);

    Layer *currentLayer() const;
    int currentSiblingIndex() const;

    bool hasNextSibling() const;
    bool hasPreviousSibling() const;
    bool hasParent() const;

    Layer *next();
    Layer *previous();

    void toFront();
    void toBack();

    // Allow use as general iterator and in range-based for loops
    bool operator==(const LayerIterator &other) const;
    bool operator!=(const LayerIterator &other) const;
    LayerIterator &operator++();
    LayerIterator operator++(int);
    Layer *operator*() const;
    Layer *operator->() const;

private:
    const Map *mMap;
    Layer *mCurrentLayer;
    int mSiblingIndex;
    int mLayerTypes;
};


/**迭代指定的地图,从第一层开始
 * Iterate the given map, starting from the first layer.
 */
inline LayerIterator::LayerIterator(const Map *map, int layerTypes)
    : mMap(map)
    , mCurrentLayer(nullptr)
    , mSiblingIndex(-1)
    , mLayerTypes(layerTypes)
{}

/**
 * Iterate the layer's map, starting at the given \a layer.
 */
inline LayerIterator::LayerIterator(Layer *start)
    : mMap(start ? start->map() : nullptr)
    , mCurrentLayer(start)
    , mSiblingIndex(start ? start->siblingIndex() : -1)
    , mLayerTypes(Layer::AnyLayerType)
{}

inline Layer *LayerIterator::currentLayer() const
{
    return mCurrentLayer;
}

inline int LayerIterator::currentSiblingIndex() const
{
    return mSiblingIndex;
}

inline bool LayerIterator::hasNextSibling() const
{
    if (!mCurrentLayer)
        return false;

    return mSiblingIndex + 1 < mCurrentLayer->siblings().size();
}

inline bool LayerIterator::hasPreviousSibling() const
{
    return mSiblingIndex > 0;
}

inline bool LayerIterator::hasParent() const
{
    return mCurrentLayer && mCurrentLayer->parentLayer();
}

inline bool LayerIterator::operator!=(const LayerIterator &other) const
{
    return !(*this == other);
}

inline LayerIterator &LayerIterator::operator++()
{
    next();
    return *this;
}

inline LayerIterator LayerIterator::operator++(int)
{
    LayerIterator it = *this;
    next();
    return it;
}

inline Layer *LayerIterator::operator*() const
{
    return mCurrentLayer;
}

inline Layer *LayerIterator::operator->() const
{
    return mCurrentLayer;
}


TILEDSHARED_EXPORT int globalIndex(Layer *layer);
TILEDSHARED_EXPORT Layer *layerAtGlobalIndex(const Map *map, int index);

} // namespace Tiled
