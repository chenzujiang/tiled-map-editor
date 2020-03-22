/*
 * layer.cpp
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

#include "layer.h"

#include "grouplayer.h"
#include "imagelayer.h"
#include "map.h"
#include "objectgroup.h"
#include "tilelayer.h"

namespace Tiled {

Layer::Layer(TypeFlag type, const QString &name, int x, int y) :
    Object(LayerType),
    mName(name),
    mId(0),
    mLayerType(type),
    mX(x),
    mY(y),
    mOpacity(1.0),//图层的透明度
    mVisible(true),
    mMap(nullptr),
    mParentLayer(nullptr),
    mLocked(false)
{
}

/**返回有效的不透明度，即不透明度乘以任何父层的不透明度。
 * Returns the effective opacity, which is the opacity multiplied by the opacity of any parent layers.
 */
qreal Layer::effectiveOpacity() const
{
    auto opacity = mOpacity;
    const Layer *layer = this;
    while ((layer = layer->parentLayer()))
        opacity *= layer->opacity();
    return opacity;
}

/**返回这个图层的显隐结果,当该图层的任何一个父图层时隐藏的,那么可见的图层任然是隐藏的.
 * Returns whether this layer is hidden. A visible layer may still be hidden,
 * when one of its parent layers is not visible.
 */
bool Layer::isHidden() const
{
    const Layer *layer = this;
    while (layer && layer->isVisible())
        layer = layer->parentLayer();
    return layer;      // encountered an invisible layer
}
//返回该图层的锁住状态,当该图层及它的任何一个父图层是锁住的话，那么返回false
bool Layer::isUnlocked() const
{
    const Layer *layer = this;
    while (layer && !layer->isLocked())
        layer = layer->parentLayer();
    return !layer;
}

/**返回后选者是图层自己还是图层的父图层
 * Returns whether the given \a candidate is this layer or one of its parents.
 */
bool Layer::isParentOrSelf(const Layer *candidate) const
{
    const Layer *layer = this;
    while (layer != candidate && layer->parentLayer())
        layer = layer->parentLayer();
    return layer == candidate;
}

/**
 * Returns the depth of this layer in the hierarchy.
 */
int Layer::depth() const
{
    int d = 0;
    GroupLayer *p = mParentLayer;
    while (p) {
        ++d;
        p = p->parentLayer();
    }
    return d;
}

/**返回这个图层在他兄弟姊妹之间的索引
 * Returns the index of this layer among its siblings.
 */
int Layer::siblingIndex() const
{
    if (mParentLayer)
        return mParentLayer->layers().indexOf(const_cast<Layer*>(this));
    if (mMap)
        return mMap->layers().indexOf(const_cast<Layer*>(this));
    return 0;
}

/**返回这个图层(map)的兄弟连链表，包括这个图层
 * Returns the list of siblings of this layer, including this layer.
 */
QList<Layer *> Layer::siblings() const
{
    if (mParentLayer)
        return mParentLayer->layers();
    if (mMap)
        return mMap->layers();//返回这张地图的顶层层的列表。

    return QList<Layer *>();
}

/**计算总偏移,这个偏移量包含了所以parent图层偏移量
 * Computes the total offset. which is the offset including the offset of all parent layers.
 */
QPointF Layer::totalOffset() const
{
    auto offset = mOffset;
    const Layer *layer = this;
    while ((layer = layer->parentLayer()))
        offset += layer->offset();
    return offset;
}

/**
 * A helper function for initializing the members of the given instance to
 * those of this layer. Used by subclasses when cloning.
 *一个帮助函数，用于初始化给定实例的成员到这一层的成员。在克隆时被子类所使用。
 * Layer name, position and size are not cloned, since they are assumed to have
 * already been passed to the constructor. Also, map ownership is not cloned,
 * since the clone is not added to the map.
 *图层的名字,位置和大小不能被克隆,因为他们认为被传递给了构造函数了,地图的所有权没有被克隆,因为他们没有被添加地图中
 * \return the initialized clone (the same instance that was passed in)
 * \sa clone()
 */
Layer *Layer::initializeClone(Layer *clone) const
{
    // mId is not copied, will be assigned when layer is added to a map 。id是没有拷贝的当图层添加到地图中时分配
    clone->mOffset = mOffset;
    clone->mOpacity = mOpacity;
    clone->mVisible = mVisible;
    clone->setProperties(properties());
    return clone;
}

TileLayer *Layer::asTileLayer()
{//是不是给定子类的实例
    return isTileLayer() ? static_cast<TileLayer*>(this) : nullptr;
}

ObjectGroup *Layer::asObjectGroup()
{
    return isObjectGroup() ? static_cast<ObjectGroup*>(this) : nullptr;
}

ImageLayer *Layer::asImageLayer()
{
    return isImageLayer() ? static_cast<ImageLayer*>(this) : nullptr;
}

GroupLayer *Layer::asGroupLayer()
{
    return isGroupLayer() ? static_cast<GroupLayer*>(this) : nullptr;
}


Layer *LayerIterator::next()
{
    Layer *layer = mCurrentLayer;
    int index = mSiblingIndex;// init is -1

    do {
        Q_ASSERT(!layer || (index >= 0 && index < mMap->layerCount()));

        // Traverse to next sibling
        ++index;

        if (!layer) {
            // Traverse to the first layer of the map
            if (mMap && index < mMap->layerCount())
                layer = mMap->layerAt(index);
            else
                break;
        }

        const auto siblings = layer->siblings();

        // Traverse to parent layer if last child
        if (index == siblings.size()) {
            layer = layer->parentLayer();
            index = layer ? layer->siblingIndex() : mMap->layerCount();
        } else {
            layer = siblings.at(index);

            // If next layer is a group, traverse to its first child
            while (layer->isGroupLayer()) {
                auto groupLayer = static_cast<GroupLayer*>(layer);
                if (groupLayer->layerCount() > 0) {
                    index = 0;
                    layer = groupLayer->layerAt(0);
                } else {
                    break;
                }
            }
        }
    } while (layer && !(layer->layerType() & mLayerTypes));

    mCurrentLayer = layer;
    mSiblingIndex = index;

    return layer;
}

Layer *LayerIterator::previous()
{
    Layer *layer = mCurrentLayer;
    int index = mSiblingIndex;

    do {
        Q_ASSERT(!layer || (index >= 0 && index < mMap->layerCount()));

        // Traverse to previous sibling
        --index;

        if (!layer) {
            // Traverse to the last layer of the map if at the end
            if (mMap && index >= 0 && index < mMap->layerCount())
                layer = mMap->layerAt(index);
            else
                break;
        } else {
            // Traverse down to last child if applicable
            if (layer->isGroupLayer()) {
                auto groupLayer = static_cast<GroupLayer*>(layer);
                if (groupLayer->layerCount() > 0) {
                    index = groupLayer->layerCount() - 1;
                    layer = groupLayer->layerAt(index);
                    continue;
                }
            }

            // Traverse to previous sibling (possibly of a parent)
            do {
                if (index >= 0) {
                    const auto siblings = layer->siblings();
                    layer = siblings.at(index);
                    break;
                }

                layer = layer->parentLayer();
                if (layer)
                    index = layer->siblingIndex() - 1;
            } while (layer);
        }
    } while (layer && !(layer->layerType() & mLayerTypes));

    mCurrentLayer = layer;
    mSiblingIndex = index;

    return layer;
}

void LayerIterator::toFront()
{
    mCurrentLayer = nullptr;
    mSiblingIndex = -1;
}

void LayerIterator::toBack()
{
    mCurrentLayer = nullptr;
    mSiblingIndex = mMap ? mMap->layerCount() : 0;
}

bool LayerIterator::operator==(const LayerIterator &other) const
{
    return mMap == other.mMap &&
            mCurrentLayer == other.mCurrentLayer &&
            mSiblingIndex == other.mSiblingIndex &&
            mLayerTypes == other.mLayerTypes;
}


/**
 * Returns the global layer index for the given \a layer.
 * Obtained by iterating the layer's map while incrementing the index until layer is found.
 * 这个索引通过迭代地图的图层获得,循环的增加索引直到找到它
 */
int globalIndex(Layer *layer)
{
    if (!layer)
        return -1;

    LayerIterator counter(layer->map());
    int index = 0;
    while (counter.next() && counter.currentLayer() != layer)
        ++index;

    return index;
}

/**
 * Returns the layer at the given global \a index.
 *返回给定的图层,在给定的全局索引中
 * \sa globalIndex()
 */
Layer *layerAtGlobalIndex(const Map *map, int index)
{
    LayerIterator counter(map);
    while (counter.next() && index > 0)
        --index;

    return counter.currentLayer();
}

} // namespace Tiled
