/*
 * mainwindow.h
 * Copyright 2008-2015, Thorbj酶rn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyright 2008, Roderic Morris <roderic@ccs.neu.edu>
 * Copyright 2009-2010, Jeff Bland <jksb@member.fsf.org>
 * Copyright 2010-2011, Stefan Beller <stefanbeller@googlemail.com>
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

#include "clipboardmanager.h"
#include "consoledock.h"
#include "document.h"
#include "preferences.h"
#include "preferencesdialog.h"

#include <QMainWindow>
#include <QPointer>
#include <QSessionManager>
#include <QSettings>

class QComboBox;
class QLabel;

namespace Ui {
class MainWindow;
}

namespace Tiled {

class FileFormat;
class TileLayer;
class Terrain;

namespace Internal {

class ActionManager;
class AutomappingManager;
class DocumentManager;
class MapDocument;
class MapDocumentActionHandler;
class MapScene;
class MapView;
class ObjectTypesEditor;
class TilesetDocument;
class Zoomable;

/**
 * The main editor window.
 *https://blog.csdn.net/xoyojank/article/details/18279611
 * Represents the main user interface, including the menu bar. It keeps track
 * of the current file and is also the entry point of all menu actions.
 * 代表主用户界面，包括菜单栏。它跟踪当前文件，同时也是所有菜单操作的入口点。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr, Qt::WindowFlags flags = 0);
    ~MainWindow() override;

    void commitData(QSessionManager &manager);

    /**
     * Opens the given file. When opened successfully, the file is added to the
     * list of recent最近的 files.
     *
     * When a \a format is given, it is used to open the file. Otherwise, a
     * format is searched using MapFormat::supportsFile.
     *当给定的格式时，该格式适用于打开文件的。否则文件格式时用MapFormat::supportsFile.来搜索的
     * @return whether the file was successfully opened
     */
    bool openFile(const QString &fileName, FileFormat *fileFormat = nullptr);

    /**
     * Attempt to open the previously opened file.尝试打开以前打开的文件
     */
    void openLastFiles();

protected:
    bool event(QEvent *event) override;

    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

    void keyPressEvent(QKeyEvent *) override;
    void keyReleaseEvent(QKeyEvent *) override;//编译器会提示，如果没有从写虚函数的话

    void dragEnterEvent(QDragEnterEvent *) override;//override 关键字，可以避免派生类中忘记重写虚函数的错误(如果没有从写会造成调用基类的,普通虚函数同时继承接口和缺省实现是危险的)
    void dropEvent(QDropEvent *) override;

private slots:
    void newMap();
    void openFileDialog();
    bool saveFile();
    bool saveFileAs();
    void saveAll();
    void export_(); // 'export' is a reserved word导出是一个保留字
    void exportAs();
    void exportAsImage();
    void reload();
    void closeFile();
    void closeAllFiles();

    void cut();
    void copy();
    void paste();
    void pasteInPlace();
    void delete_();
    void openPreferences();//打开手选项

    void labelVisibilityActionTriggered(QAction *action);
    void zoomIn();
    void zoomOut();
    void zoomNormal();
    void setFullScreen(bool fullScreen);
    void toggleClearView(bool clearView);
    void resetToDefaultLayout();

    bool newTileset(const QString &path = QString());
    void reloadTilesetImages();
    void addExternalTileset();
    void resizeMap();
    void offsetMap();
    void editMapProperties();//编辑地图的道具(内容)

    void editTilesetProperties();

    void updateWindowTitle();
    void updateActions();
    void updateZoomable();
    void updateZoomActions();
    void openDocumentation();
    void becomePatron();
    void aboutTiled();
    void openRecentFile();

    void documentChanged(Document *document);
    void closeDocument(int index);

    void reloadError(const QString &error);
    void autoMappingError(bool automatic);
    void autoMappingWarning(bool automatic);

    void onObjectTypesEditorClosed();

    void ensureHasBorderInFullScreen();

private:
    /**
      * Asks the user whether the given \a mapDocument should be saved, when
      * necessary. If it needs to ask, also makes sure that it is the current
      * document.当必要的时候，问用户是否需要保存给定的mapDocument.如果需要询问，如果是需要的就确保他是当前的文档。
      *
      * @return <code>true</code> when any unsaved data is either discarded or
      *         saved, <code>false</code> when the user cancelled or saving
      *         failed.当任何未保存的数据被丢弃或保存时，返回true,当用户取消或者保存失败返回false
      */
    bool confirmSave(Document *document);

    /**
      * Checks all maps for changes, if so, ask if to save these changes.
      *检查所有的地图是否发生更改，如果是的话，问是否需要保存更改。
      * @return <code>true</code> when any unsaved data is either discarded or
      *         saved, <code>false</code> when the user cancelled or saving
      *         failed.
      */
    bool confirmAllSave();

    void writeSettings();
    void readSettings();

    void updateRecentFilesMenu();
    void updateViewsAndToolbarsMenu();

    void retranslateUi();

    void exportMapAs(MapDocument *mapDocument);
    void exportTilesetAs(TilesetDocument *tilesetDocument);

    ActionManager *mActionManager;
    Ui::MainWindow *mUi;
    Document *mDocument = nullptr;
    Zoomable *mZoomable = nullptr;
    MapDocumentActionHandler *mActionHandler;
    ConsoleDock *mConsoleDock;
    ObjectTypesEditor *mObjectTypesEditor;
    QSettings mSettings;

    QAction *mRecentFiles[Preferences::MaxRecentFiles];

    QMenu *mLayerMenu;
    QMenu *mNewLayerMenu;
    QMenu *mGroupLayerMenu;
    QMenu *mViewsAndToolbarsMenu;
    QAction *mViewsAndToolbarsAction;
    QAction *mShowObjectTypesEditor;

    QAction *mResetToDefaultLayout;

    void setupQuickStamps();

    AutomappingManager *mAutomappingManager;
    DocumentManager *mDocumentManager;

    QPointer<PreferencesDialog> mPreferencesDialog;

    QMap<QMainWindow*, QByteArray> mMainWindowStates;
};

} // namespace Internal
} // namespace Tiled
