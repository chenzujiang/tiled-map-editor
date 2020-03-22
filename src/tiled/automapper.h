/*
 * automapper.h
 * Copyright 2010-2012, Stefan Beller, stefanbeller@googlemail.com
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

#include "tileset.h"

#include <QList>
#include <QMap>
#include <QRegion>
#include <QSet>
#include <QString>
#include <QVector>

namespace Tiled {

class Layer;
class Map;
class MapObject;
class ObjectGroup;
class TileLayer;

namespace Internal {

class MapDocument;

struct InputLayer
{
    TileLayer *tileLayer;
    bool strictEmpty;
};

class InputConditions
{
public:
    QVector<InputLayer> listYes;    // "input"
    QVector<InputLayer> listNo;     // "inputnot"
};

// Maps layer names to their conditions
typedef QMap<QString, InputConditions> InputIndex;

// Maps an index to a group of input layers
class InputLayers : public QMap<QString, InputIndex>
{
public:
    QSet<QString> names; // all names
};

class RuleOutput : public QMap<Layer*, int>
{
public:
    QString index;
};


/**
 * This class does all the work for the automapping feature.
 * basically it can do the following:
 * - check the rules map for rules and store them
 * - compare TileLayers (i. e. check if/where a certain rule must be applied)
 * - copy regions of Maps (multiple Layers, the layerlist is a
 *                         lookup-table for matching the Layers)
 */
class AutoMapper : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructs an AutoMapper.
     * All data structures, which only rely on the rules map are setup
     * here.
     *
     * @param workingDocument: the map to work on.
     * @param rules: The rule map which should be used for automapping. The
     *               AutoMapper takes ownership of this map.
     * @param rulePath: The filepath to the rule map.
     */
    AutoMapper(MapDocument *workingDocument, Map *rules,
               const QString &rulePath);
    ~AutoMapper();

    /**
     * Checks if the passed \a ruleLayerName is used in this instance
     * of Automapper.
     */
    bool ruleLayerNameUsed(const QString &ruleLayerName) const;

    /**
     * Call prepareLoad first! Returns a set of strings describing the tile
     * layers, which could be touched considering the given layers of the
     * rule map.
     */
    QSet<QString> touchedTileLayers() const;

    /**
     * This needs to be called directly before the autoMap call.
     * It sets up some data structures which change rapidly, so it is quite
     * painful to keep these data structures up to date all time. (indices of
     * layers of the working map)
     */
    bool prepareAutoMap();

    /**
     * Here is done all the automapping.
     */
    void autoMap(QRegion *where);

    /**
     * This cleans all data structures, which are setup via prepareAutoMap,
     * so the auto mapper becomes ready for its next automatic mapping.
     */
    void cleanAll();

    /**
     * Contains all errors until operation was canceled.
     * The errorlist is cleared within prepareLoad and prepareAutoMap.
     */
    QString errorString() const { return mError; }

    /**
     * Contains all warnings which occur at loading a rules map or while
     * automapping.
     * The errorlist is cleared within prepareLoad and prepareAutoMap.
     */
    QString warningString() const { return mWarning; }

private:
    /**
     * Reads the map properties of the rulesmap.
     * @return returns true when anything is ok, false when errors occurred.
     */
    bool setupRuleMapProperties();
    void setupInputLayerProperties(InputLayer &inputLayer);

    void cleanUpRulesMap();

    /**
     * Searches the rules layer for regions and stores these in \a rules.
     * @return returns true when anything is ok, false when errors occurred.
     */
    bool setupRuleList();

    /**
     * Sets up the layers in the rules map, which are used for automapping.
     * The layers are detected and put in the internal data structures
     * @return returns true when anything is ok, false when errors occurred.
     */
    bool setupRuleMapTileLayers();

    /**
     * Checks if all needed layers in the working map are there.
     * If not, add them in the correct order.
     */
    bool setupMissingLayers();

    /**
     * Checks if the layers setup as in setupRuleMapLayers are still right.
     * If it's not right, correct them.
     * @return returns true if everything went fine. false is returned when
     *         no set layer was found
     */
    bool setupCorrectIndexes();

    /**
     * sets up the tilesets which are used in automapping.
     * @return returns true when anything is ok, false when errors occurred.
     */
    bool setupTilesets();

    /**
     * Returns the conjunction of all regions of all setlayers.
     */
    QRegion computeSetLayersRegion() const;

    /**
     * This copies all Tiles from TileLayer src to TileLayer dst
     *
     * In src the Tiles are taken from the rectangle given by
     * srcX, srcY, width and height.
     * In dst they get copied to a rectangle given by
     * dstX, dstY, width, height .
     * if there is no tile in src TileLayer, there will nothing be copied,
     * so the maybe existing tile in dst will not be overwritten.
     *
     */
    void copyTileRegion(const TileLayer *srcLayer, int srcX, int srcY,
                        int width, int height, TileLayer *dstLayer,
                        int dstX, int dstY);

    /**
     * This copies all objects from the \a src_lr ObjectGroup to the \a dst_lr
     * in the given rectangle.
     *
     * The rectangle is described by the upper left corner \a srcX \a srcY
     * and its \a width and \a height. The parameter \a dstX and \a dstY
     * offset the copied objects in the destination object group.
     */
    void copyObjectRegion(const ObjectGroup *srcLayer, int srcX, int srcY,
                          int width, int height, ObjectGroup *dstLayer,
                          int dstX, int dstY);


    /**
     * This copies multiple TileLayers from one map to another.
     * Only the region \a region is considered for copying.
     * In the destination it will come to the region translated by Offset.
     * The parameter \a LayerTranslation is a map of which layers of the rulesmap
     * should get copied into which layers of the working map.
     */
    void copyMapRegion(const QRegion &region, QPoint Offset,
                       const RuleOutput &LayerTranslation);

    /**
     * This goes through all the positions of the mMapWork and checks if
     * there fits the rule given by the region in mMapRuleSet.
     * if there is a match all Layers are copied to mMapWork.
     * @param ruleIndex: the region which should be compared to all positions
     *              of mMapWork will be looked up in mRulesInput and mRulesOutput
     * @return where: an rectangle where the rule actually got applied
     */
    QRect applyRule(int ruleIndex, const QRect &where);

    /**
     * Cleans up the data structures filled by setupRuleMapLayers(),
     * so the next rule can be processed.
     */
    void cleanUpRuleMapLayers();

    /**
     * Cleans up the data structures filled by setupTilesets(),
     * so the next rule can be processed.
     */
    void cleanTilesets();

    /**
     * Cleans up the added tile layers setup by setupMissingLayers(),
     * so we have a minimal addition of tile layers by the automapping.
     */
    void cleanTileLayers();

    /**
     * where to work in
     */
    MapDocument *mMapDocument;

    /**
     * the same as mMapDocument->map()
     */
    Map *mMapWork;

    /**
     * map containing the rules, usually different than mMapWork
     */
    Map *mMapRules;

    /**
     * This contains all added tilesets as pointers.
     * if rules use Tilesets which are not in the mMapWork they are added.
     * keep track of them, because we need to delete them afterwards,
     * when they still are unused
     * they will be added while setupTilesets().
     */
    QVector<SharedTileset> mAddedTilesets;

    /**
     * description see: mAddedTilesets
     */
    QVector<Layer*> mAddedLayers;

    /**
     * Points to the tilelayer, which defines the input regions.
     */
    TileLayer *mLayerInputRegions;

    /**
     * Points to the tilelayer, which defines the output regions.
     */
    TileLayer *mLayerOutputRegions;

    /**
     * Contains all tilelayer pointers, which names begin with input*
     * It is sorted by index and name
     */
    InputLayers mInputRules;

    /**
     * List of Regions in mMapRules to know where the input rules are
     */
    QVector<QRegion> mRulesInput;

    /**
     * List of regions in mMapRules to know where the output of a
     * rule is.
     * mRulesOutput[i] is the output of that rule,
     * which has the input at mRulesInput[i], meaning that mRulesInput
     * and mRulesOutput must match with the indexes.
     */
    QVector<QRegion> mRulesOutput;

    /**
     * The inner set with layers to indexes is needed for translating
     * tile layers from mMapRules to mMapWork.
     *
     * The key is the pointer to the layer in the rulemap. The
     * pointer to the layer within the working map is not hardwired, but the
     * position in the layerlist, where it was found the last time.
     * This loosely bound pointer ensures we will get the right layer, since we
     * need to check before anyway, and it is still fast.
     *
     * The list is used to hold different translation tables
     * => one of the tables is chosen by chance, so randomness is available
     */
    QVector<RuleOutput> mLayerList;

    /**
     * store the name of the processed rules file, to have detailed
     * error messages available
     */
    QString mRulePath;

    /**
     * determines if all tiles in all touched layers should be deleted first.
     */
    bool mDeleteTiles;

    /**
     * This variable determines, how many overlapping tiles should be used.
     * The bigger the more area is remapped at an automapping operation.
     * This can lead to higher latency, but provides a better behavior on
     * interactive automapping.
     * It defaults to zero.
     */
    int mAutoMappingRadius;

    /**
     * Determines if a rule is allowed to overlap itself.
     */
    bool mNoOverlappingRules;

    QSet<QString> mTouchedTileLayers;
    QSet<QString> mTouchedObjectGroups;

    QString mError;
    QString mWarning;
};

} // namespace Internal
} // namespace Tiled
