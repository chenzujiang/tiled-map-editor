/*
 * preferences.cpp
 * Copyright 2009-2011, Thorbj酶rn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyright 2016, Mamed Ibrahimov <ibramlab@gmail.com>
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

#include "preferences.h"

#include "documentmanager.h"
#include "languagemanager.h"
#include "mapdocument.h"
#include "pluginmanager.h"
#include "savefile.h"
#include "tilesetmanager.h"

#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

using namespace Tiled;
using namespace Tiled::Internal;

Preferences *Preferences::mInstance;

Preferences *Preferences::instance()
{
    if (!mInstance)
        mInstance = new Preferences;
    return mInstance;
}

void Preferences::deleteInstance()
{
    delete mInstance;
    mInstance = nullptr;
}
//够函数
Preferences::Preferences()
    : mSettings(new QSettings())
{
    // Retrieve storage settings检索/取回存储设置//存储mSettings值
    mSettings->beginGroup(QLatin1String("Storage"));
    mLayerDataFormat = static_cast<Map::LayerDataFormat>//图层数据格式intValue
            (intValue("LayerDataFormat", Map::CSV));//Map::CSV is enum 4 transmit defaultValue return int change enum ,so static_cast
    mMapRenderOrder = static_cast<Map::RenderOrder>
            (intValue("MapRenderOrder", Map::RightDown));//渲染顺序

    mDtdEnabled = boolValue("DtdEnabled");//second param is false
    mSafeSavingEnabled = boolValue("SafeSavingEnabled", true);
    mReloadTilesetsOnChange = boolValue("ReloadTilesets", true);
    mStampsDirectory = stringValue("StampsDirectory");
    mTemplatesDirectory = stringValue("TemplatesDirectory");
    mObjectTypesFile = stringValue("ObjectTypesFile");
    mSettings->endGroup();

    SaveFile::setSafeSavingEnabled(mSafeSavingEnabled);

    // Retrieve interface settings 检索/取回见面(接口)设置
    mSettings->beginGroup(QLatin1String("Interface"));
    mShowGrid = boolValue("ShowGrid", true);
    mShowTileObjectOutlines = boolValue("ShowTileObjectOutlines");
    mShowTileAnimations = boolValue("ShowTileAnimations", true);
    mSnapToGrid = boolValue("SnapToGrid");//表格对齐
    mSnapToFineGrid = boolValue("SnapToFineGrid");
    mSnapToPixels = boolValue("SnapToPixels");
    mGridColor = colorValue("GridColor", Qt::black);
    mGridFine = intValue("GridFine", 4);
    mObjectLineWidth = realValue("ObjectLineWidth", 2);
    mHighlightCurrentLayer = boolValue("HighlightCurrentLayer");
    mShowTilesetGrid = boolValue("ShowTilesetGrid", true);
    mLanguage = stringValue("Language");
    mUseOpenGL = boolValue("OpenGL");
    mWheelZoomsByDefault = boolValue("WheelZoomsByDefault");
    mObjectLabelVisibility = static_cast<ObjectLabelVisiblity>
            (intValue("ObjectLabelVisibility", AllObjectLabels));
    mLabelForHoveredObject = boolValue("LabelForHoveredObject", false);
#if defined(Q_OS_MAC)
    mApplicationStyle = static_cast<ApplicationStyle>
            (intValue("ApplicationStyle", SystemDefaultStyle));
#else
    mApplicationStyle = static_cast<ApplicationStyle>
            (intValue("ApplicationStyle", TiledStyle));
#endif

    // Backwards compatibility check since 'FusionStyle' was removed from the preferences dialog.
    //自“FusionStyle”从preferences对话框中删除后，向后兼容检查。
    if (mApplicationStyle == FusionStyle)
        mApplicationStyle = TiledStyle;

    mBaseColor = colorValue("BaseColor", Qt::lightGray);
    mSelectionColor = colorValue("SelectionColor", QColor(48, 140, 198));//返回所选择的颜色QColor
    mSettings->endGroup();

    // Retrieve defined object types 检索/取回定义对象的类型
    ObjectTypesSerializer objectTypesSerializer;
    ObjectTypes objectTypes;
    bool success = objectTypesSerializer.readObjectTypes(objectTypesFile(), objectTypes);

    // For backwards compatibilty, read in object types from settings对于向后兼容，从设置中读取对象类型
    if (!success) {
        mSettings->beginGroup(QLatin1String("ObjectTypes"));
        const QStringList names = mSettings->value(QLatin1String("Names")).toStringList();
        const QStringList colors = mSettings->value(QLatin1String("Colors")).toStringList();
        mSettings->endGroup();

        if (!names.isEmpty()) {
            const int count = qMin(names.size(), colors.size());
            for (int i = 0; i < count; ++i)
                objectTypes.append(ObjectType(names.at(i), QColor(colors.at(i))));
        }
    } else {
        mSettings->remove(QLatin1String("ObjectTypes"));
    }

    Object::setObjectTypes(objectTypes);

    mSettings->beginGroup(QLatin1String("Automapping"));
    mAutoMapDrawing = boolValue("WhileDrawing");
    mSettings->endGroup();

    mSettings->beginGroup(QLatin1String("MapsDirectory"));
    mMapsDirectory = stringValue("Current");
    mSettings->endGroup();

    TilesetManager *tilesetManager = TilesetManager::instance();
    tilesetManager->setReloadTilesetsOnChange(mReloadTilesetsOnChange);//当瓦砖图片发生改变后重新加载瓦砖
    tilesetManager->setAnimateTiles(mShowTileAnimations);//设置运行tiled动画

    // Read the lists of enabled and disabled plugins
    const QStringList disabledPlugins = mSettings->value(QLatin1String("Plugins/Disabled")).toStringList();
    const QStringList enabledPlugins = mSettings->value(QLatin1String("Plugins/Enabled")).toStringList();

    PluginManager *pluginManager = PluginManager::instance();//[!3]
    for (const QString &fileName : disabledPlugins)
    {
        pluginManager->setPluginState(fileName, PluginDisabled);
        qDebug()<<fileName;
    }
    for (const QString &fileName : enabledPlugins)
    {
        pluginManager->setPluginState(fileName, PluginEnabled);
        qDebug()<<fileName;
    }

    // Keeping track of some usage information.跟踪一些使用信息。
    mSettings->beginGroup(QLatin1String("Install"));
    mFirstRun = mSettings->value(QLatin1String("FirstRun")).toDate();
    mPatreonDialogTime = mSettings->value(QLatin1String("PatreonDialogTime")).toDate();
    mRunCount = intValue("RunCount", 0) + 1;
    mIsPatron = boolValue("IsPatron");
    mCheckForUpdates = boolValue("CheckForUpdates");
    if (!mFirstRun.isValid()) {
        mFirstRun = QDate::currentDate();
        mSettings->setValue(QLatin1String("FirstRun"), mFirstRun.toString(Qt::ISODate));
    }
    if (!mSettings->contains(QLatin1String("PatreonDialogTime"))) {
        mPatreonDialogTime = mFirstRun.addMonths(1);
        const QDate today(QDate::currentDate());
        if (mPatreonDialogTime.daysTo(today) >= 0)
            mPatreonDialogTime = today.addDays(2);
        mSettings->setValue(QLatin1String("PatreonDialogTime"), mPatreonDialogTime.toString(Qt::ISODate));
    }
    mSettings->setValue(QLatin1String("RunCount"), mRunCount);
    mSettings->endGroup();

    // Retrieve startup settings读回启动设置
    mSettings->beginGroup(QLatin1String("Startup"));
    mOpenLastFilesOnStartup = boolValue("OpenLastFiles", true);
    mSettings->endGroup();
}

Preferences::~Preferences()
{
}

void Preferences::setObjectLabelVisibility(ObjectLabelVisiblity visibility)
{
    if (mObjectLabelVisibility == visibility)
        return;

    mObjectLabelVisibility = visibility;
    mSettings->setValue(QLatin1String("Interface/ObjectLabelVisibility"), visibility);//change key->value
    emit objectLabelVisibilityChanged(visibility);
}
//为悬停对象设置label
void Preferences::setLabelForHoveredObject(bool enabled)
{
    if (mLabelForHoveredObject == enabled)
        return;

    mLabelForHoveredObject = enabled;
    mSettings->setValue(QLatin1String("Interface/LabelForHoveredObject"), enabled);
    emit labelForHoveredObjectChanged(enabled);
}

void Preferences::setApplicationStyle(ApplicationStyle style)
{
    if (mApplicationStyle == style)
        return;

    mApplicationStyle = style;
    mSettings->setValue(QLatin1String("Interface/ApplicationStyle"), style);
    emit applicationStyleChanged(style);
}

void Preferences::setBaseColor(const QColor &color)
{
    if (mBaseColor == color)
        return;

    mBaseColor = color;
    mSettings->setValue(QLatin1String("Interface/BaseColor"), color.name());
    emit baseColorChanged(color);
}

void Preferences::setSelectionColor(const QColor &color)
{
    if (mSelectionColor == color)
        return;

    mSelectionColor = color;
    mSettings->setValue(QLatin1String("Interface/SelectionColor"), color.name());
    emit selectionColorChanged(color);
}

void Preferences::setShowGrid(bool showGrid)
{
    if (mShowGrid == showGrid)
        return;

    mShowGrid = showGrid;
    mSettings->setValue(QLatin1String("Interface/ShowGrid"), mShowGrid);
    emit showGridChanged(mShowGrid);
}

void Preferences::setShowTileObjectOutlines(bool enabled)
{
    if (mShowTileObjectOutlines == enabled)
        return;

    mShowTileObjectOutlines = enabled;
    mSettings->setValue(QLatin1String("Interface/ShowTileObjectOutlines"), mShowTileObjectOutlines);
    emit showTileObjectOutlinesChanged(mShowTileObjectOutlines);
}

void Preferences::setShowTileAnimations(bool enabled)
{
    if (mShowTileAnimations == enabled)
        return;

    mShowTileAnimations = enabled;
    mSettings->setValue(QLatin1String("Interface/ShowTileAnimations"),
                        mShowTileAnimations);

    TilesetManager *tilesetManager = TilesetManager::instance();
    tilesetManager->setAnimateTiles(mShowTileAnimations);//Sets whether tile animations are running.

    emit showTileAnimationsChanged(mShowTileAnimations);
}

void Preferences::setSnapToGrid(bool snapToGrid)
{
    if (mSnapToGrid == snapToGrid)
        return;

    mSnapToGrid = snapToGrid;
    mSettings->setValue(QLatin1String("Interface/SnapToGrid"), mSnapToGrid);
    emit snapToGridChanged(mSnapToGrid);
}
//栅格对齐
void Preferences::setSnapToFineGrid(bool snapToFineGrid)
{
    if (mSnapToFineGrid == snapToFineGrid)
        return;

    mSnapToFineGrid = snapToFineGrid;
    mSettings->setValue(QLatin1String("Interface/SnapToFineGrid"), mSnapToFineGrid);
    emit snapToFineGridChanged(mSnapToFineGrid);
}

void Preferences::setSnapToPixels(bool snapToPixels)
{
    if (mSnapToPixels == snapToPixels)
        return;

    mSnapToPixels = snapToPixels;
    mSettings->setValue(QLatin1String("Interface/SnapToPixels"), mSnapToPixels);
    emit snapToPixelsChanged(mSnapToPixels);
}

void Preferences::setGridColor(QColor gridColor)
{
    if (mGridColor == gridColor)
        return;

    mGridColor = gridColor;
    mSettings->setValue(QLatin1String("Interface/GridColor"), mGridColor.name());
    emit gridColorChanged(mGridColor);
}
//网格化
void Preferences::setGridFine(int gridFine)
{
    if (mGridFine == gridFine)
        return;
    mGridFine = gridFine;
    mSettings->setValue(QLatin1String("Interface/GridFine"), mGridFine);
    emit gridFineChanged(mGridFine);
}

void Preferences::setObjectLineWidth(qreal lineWidth)
{
    if (mObjectLineWidth == lineWidth)
        return;
    mObjectLineWidth = lineWidth;
    mSettings->setValue(QLatin1String("Interface/ObjectLineWidth"), mObjectLineWidth);
    emit objectLineWidthChanged(mObjectLineWidth);
}

void Preferences::setHighlightCurrentLayer(bool highlight)
{
    if (mHighlightCurrentLayer == highlight)
        return;

    mHighlightCurrentLayer = highlight;
    mSettings->setValue(QLatin1String("Interface/HighlightCurrentLayer"),
                        mHighlightCurrentLayer);
    emit highlightCurrentLayerChanged(mHighlightCurrentLayer);
}

void Preferences::setShowTilesetGrid(bool showTilesetGrid)
{
    if (mShowTilesetGrid == showTilesetGrid)
        return;

    mShowTilesetGrid = showTilesetGrid;
    mSettings->setValue(QLatin1String("Interface/ShowTilesetGrid"),
                        mShowTilesetGrid);
    emit showTilesetGridChanged(mShowTilesetGrid);
}

Map::LayerDataFormat Preferences::layerDataFormat() const
{
    return mLayerDataFormat;
}

void Preferences::setLayerDataFormat(Map::LayerDataFormat
                                     layerDataFormat)
{
    if (mLayerDataFormat == layerDataFormat)
        return;

    mLayerDataFormat = layerDataFormat;
    mSettings->setValue(QLatin1String("Storage/LayerDataFormat"),
                        mLayerDataFormat);
}
//地图渲染顺序
Map::RenderOrder Preferences::mapRenderOrder() const
{
    return mMapRenderOrder;
}

void Preferences::setMapRenderOrder(Map::RenderOrder mapRenderOrder)
{
    if (mMapRenderOrder == mapRenderOrder)
        return;

    mMapRenderOrder = mapRenderOrder;
    mSettings->setValue(QLatin1String("Storage/MapRenderOrder"),
                        mMapRenderOrder);
}

bool Preferences::dtdEnabled() const
{
    return mDtdEnabled;
}

void Preferences::setDtdEnabled(bool enabled)
{
    mDtdEnabled = enabled;
    mSettings->setValue(QLatin1String("Storage/DtdEnabled"), enabled);
}

void Preferences::setSafeSavingEnabled(bool enabled)
{
    mSafeSavingEnabled = enabled;
    mSettings->setValue(QLatin1String("Storage/SafeSavingEnabled"), enabled);
    SaveFile::setSafeSavingEnabled(enabled);
}

QString Preferences::language() const
{
    return mLanguage;
}

void Preferences::setLanguage(const QString &language)
{
    if (mLanguage == language)
        return;

    mLanguage = language;
    mSettings->setValue(QLatin1String("Interface/Language"),
                        mLanguage);

    LanguageManager::instance()->installTranslators();
    emit languageChanged();
}
//在该改变之后重新加载瓦砖
bool Preferences::reloadTilesetsOnChange() const
{
    return mReloadTilesetsOnChange;
}

void Preferences::setReloadTilesetsOnChanged(bool value)
{
    if (mReloadTilesetsOnChange == value)
        return;

    mReloadTilesetsOnChange = value;
    mSettings->setValue(QLatin1String("Storage/ReloadTilesets"),
                        mReloadTilesetsOnChange);

    TilesetManager *tilesetManager = TilesetManager::instance();
    tilesetManager->setReloadTilesetsOnChange(mReloadTilesetsOnChange);
}

void Preferences::setUseOpenGL(bool useOpenGL)
{
    if (mUseOpenGL == useOpenGL)
        return;

    mUseOpenGL = useOpenGL;
    mSettings->setValue(QLatin1String("Interface/OpenGL"), mUseOpenGL);

    emit useOpenGLChanged(mUseOpenGL);//当使用OpenGL时全屏时保证有边框,没有OpenGL时一定有边框的
}

void Preferences::setObjectTypes(const ObjectTypes &objectTypes)
{
    Object::setObjectTypes(objectTypes);
    emit objectTypesChanged();
}

static QString lastPathKey(Preferences::FileType fileType)
{
    QString key = QLatin1String("LastPaths/");

    switch (fileType) {
    case Preferences::ObjectTypesFile:
        key.append(QLatin1String("ObjectTypes"));
        break;
    case Preferences::ObjectTemplateFile:
        key.append(QLatin1String("ObjectTemplates"));
        break;
    case Preferences::ImageFile:
        key.append(QLatin1String("Images"));
        break;
    case Preferences::ExportedFile:
        key.append(QLatin1String("ExportedFile"));
        break;
    case Preferences::ExternalTileset:
        key.append(QLatin1String("ExternalTileset"));
        break;
    case Preferences::WorldFile:
        key.append(QLatin1String("WorldFile"));
        break;
    }

    return key;
}

/**返回所选择的给定文件类型最后一次打开文件的位置,只要他是使用setLastPath()设置的。
 * Returns the last location of a file chooser for the given file type. As long
 * as it was set using setLastPath().
 *
 * When no last path for this file type exists yet, the path of the currently
 * selected map is returned.
 * 当这个文件类型还没有最后的路径存在时，返回当前选定的map的路径。
 * When no map is open, the user's 'Documents' folder is returned.
 * 当没有map打开,返回用户的文档文件夹
 */
QString Preferences::lastPath(FileType fileType) const
{
    QString path = mSettings->value(lastPathKey(fileType)).toString();//LastPaths/Images

    if (path.isEmpty()) {
        DocumentManager *documentManager = DocumentManager::instance();
        Document *document = documentManager->currentDocument();
        if (document)
            path = QFileInfo(document->fileName()).path();
    }

    if (path.isEmpty()) {
        path = QStandardPaths::writableLocation(
                    QStandardPaths::DocumentsLocation);//如果不能确定位置，则返回要写入类型文件的目录或空字符串。
    }

    return path;
}

/**
 * \see lastPath()
 */
void Preferences::setLastPath(FileType fileType, const QString &path)
{
    if (path.isEmpty())
        return;

    mSettings->setValue(lastPathKey(fileType), path);
}

void Preferences::setAutomappingDrawing(bool enabled)
{
    mAutoMapDrawing = enabled;
    mSettings->setValue(QLatin1String("Automapping/WhileDrawing"), enabled);
}

QString Preferences::mapsDirectory() const
{
    return mMapsDirectory;
}

void Preferences::setMapsDirectory(const QString &path)
{
    if (mMapsDirectory == path)
        return;
    mMapsDirectory = path;
    mSettings->setValue(QLatin1String("MapsDirectory/Current"), path);

    emit mapsDirectoryChanged();
}
//设置赞助人
void Preferences::setPatron(bool isPatron)
{
    if (mIsPatron == isPatron)
        return;

    mIsPatron = isPatron;
    mSettings->setValue(QLatin1String("Install/IsPatron"), isPatron);

    emit isPatronChanged();
}

bool Preferences::shouldShowPatreonDialog() const
{
    if (mIsPatron)
        return false;
    if (mRunCount < 7)
        return false;
    if (!mPatreonDialogTime.isValid())
        return false;

    return mPatreonDialogTime.daysTo(QDate::currentDate()) >= 0;
}
//reminder提示
void Preferences::setPatreonDialogReminder(const QDate &date)
{
    if (date.isValid())
        setPatron(false);
    mPatreonDialogTime = date;
    mSettings->setValue(QLatin1String("Install/PatreonDialogTime"), mPatreonDialogTime.toString(Qt::ISODate));
}

QStringList Preferences::recentFiles() const
{
    QVariant v = mSettings->value(QLatin1String("recentFiles/fileNames"));
    return v.toStringList();
}
//返回最近打开文件List中first item的路径
QString Preferences::fileDialogStartLocation() const
{
    QStringList files = recentFiles();//列表中的第一个文件就是最近打开的,该文件的路径就是startlocation
    return (!files.isEmpty()) ? QFileInfo(files.first()).path() : QString();
}

/***
 * @projectName   Tiled
 * @brief         Adds the given file to the recent files list.
 * @author        Casey.Chen
 * @date          2019-05-18
 */
void Preferences::addRecentFile(const QString &fileName)
{
    // Remember the file by its absolute file path 记住文件的绝对文件路径
    // (not the canonical one, which avoids unexpected paths when symlinks are involved).不是规范的，当涉及到符号链接时，它避免了意想不到的路径
    const QString absoluteFilePath = QDir::cleanPath(QFileInfo(fileName).absoluteFilePath());

    if (absoluteFilePath.isEmpty())
        return;

    QStringList files = recentFiles();
    files.removeAll(absoluteFilePath);//Removes all occurrences of value in the list and returns the number of entries removed.
    files.prepend(absoluteFilePath);
    while (files.size() > MaxRecentFiles)
        files.removeLast();//移除最后一个文件
    //Removes the last item in the list. Calling this function is equivalent to calling removeAt(size() - 1).
    //The list must not be empty. If the list can be empty, call isEmpty() before calling this function.

    mSettings->beginGroup(QLatin1String("recentFiles"));
    mSettings->setValue(QLatin1String("fileNames"), files);
    mSettings->endGroup();

    emit recentFilesChanged();
}

void Preferences::clearRecentFiles()
{
    mSettings->remove(QLatin1String("recentFiles/fileNames"));
    emit recentFilesChanged();
}

void Preferences::setCheckForUpdates(bool on)
{
    if (mCheckForUpdates == on)
        return;

    mCheckForUpdates = on;
    mSettings->setValue(QLatin1String("Install/CheckForUpdates"), on);

    emit checkForUpdatesChanged();
}

void Preferences::setOpenLastFilesOnStartup(bool open)
{
    if (mOpenLastFilesOnStartup == open)
        return;

    mOpenLastFilesOnStartup = open;
    mSettings->setValue(QLatin1String("Startup/OpenLastFiles"), open);
}
#include <QDebug>
void Preferences::setPluginEnabled(const QString &fileName, bool enabled)
{
    PluginManager::instance()->setPluginState(fileName, enabled ? PluginEnabled : PluginDisabled);

    QStringList disabledPlugins;
    QStringList enabledPlugins;

    PluginManager *pluginManager = PluginManager::instance();
    auto &states = pluginManager->pluginStates();

    for (auto it = states.begin(), it_end = states.end(); it != it_end; ++it) {
        const QString &fileName = it.key();
        PluginState state = it.value();
        switch (state) {
        case PluginEnabled:
            enabledPlugins.append(fileName);
            qDebug()<<enabledPlugins;
            break;
        case PluginDisabled:
            disabledPlugins.append(fileName);
            qDebug()<<disabledPlugins;
            break;
        case PluginDefault:
        case PluginStatic:
            break;
        }
    }

    mSettings->setValue(QLatin1String("Plugins/Disabled"), disabledPlugins);
    mSettings->setValue(QLatin1String("Plugins/Enabled"), enabledPlugins);
}
//设置旋转到默认大小
void Preferences::setWheelZoomsByDefault(bool mode)
{
    if (mWheelZoomsByDefault == mode)
        return;

    mWheelZoomsByDefault = mode;
    mSettings->setValue(QLatin1String("Interface/WheelZoomsByDefault"), mode);
}

bool Preferences::boolValue(const char *key, bool defaultValue) const
{
    return mSettings->value(QLatin1String(key), defaultValue).toBool();
}

QColor Preferences::colorValue(const char *key, const QColor &def) const
{
    const QString name = mSettings->value(QLatin1String(key),
                                          def.name()).toString();
    if (!QColor::isValidColor(name))
        return QColor();

    return QColor(name);
}

QString Preferences::stringValue(const char *key, const QString &def) const
{
    return mSettings->value(QLatin1String(key), def).toString();
}

int Preferences::intValue(const char *key, int defaultValue) const
{//返回设置好的值
    return mSettings->value(QLatin1String(key), defaultValue).toInt();
}

qreal Preferences::realValue(const char *key, qreal defaultValue) const
{
    return mSettings->value(QLatin1String(key), defaultValue).toReal();
}

static QString dataLocation()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QString Preferences::stampsDirectory() const
{
    if (mStampsDirectory.isEmpty())
        return dataLocation() + QLatin1String("/stamps");

    return mStampsDirectory;
}

void Preferences::setStampsDirectory(const QString &stampsDirectory)
{
    if (mStampsDirectory == stampsDirectory)
        return;

    mStampsDirectory = stampsDirectory;
    mSettings->setValue(QLatin1String("Storage/StampsDirectory"), stampsDirectory);

    emit stampsDirectoryChanged(stampsDirectory);
}

QString Preferences::templatesDirectory() const
{
    if (mTemplatesDirectory.isEmpty())
        return dataLocation() + QLatin1String("/templates");

    return mTemplatesDirectory;
}

void Preferences::setTemplatesDirectory(const QString &templatesDirectory)
{
    if (mTemplatesDirectory == templatesDirectory)
        return;

    mTemplatesDirectory = templatesDirectory;
    mSettings->setValue(QLatin1String("Storage/TemplatesDirectory"), templatesDirectory);

    emit templatesDirectoryChanged(templatesDirectory);
}

QString Preferences::objectTypesFile() const
{
    if (mObjectTypesFile.isEmpty())
        return dataLocation() + QLatin1String("/objecttypes.xml");

    return mObjectTypesFile;
}

void Preferences::setObjectTypesFile(const QString &fileName)
{
    if (mObjectTypesFile == fileName)
        return;

    mObjectTypesFile = fileName;
    mSettings->setValue(QLatin1String("Storage/ObjectTypesFile"), fileName);

    emit stampsDirectoryChanged(fileName);
}
