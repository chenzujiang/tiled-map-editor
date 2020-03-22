/*
 * mainwindow.h
 * Copyright 2008-2015, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
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
 * �������û����棬�����˵����������ٵ�ǰ�ļ���ͬʱҲ�����в˵���������ڵ㡣
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
     * list of recent����� files.
     *
     * When a \a format is given, it is used to open the file. Otherwise, a
     * format is searched using MapFormat::supportsFile.
     *�������ĸ�ʽʱ���ø�ʽ�����ڴ��ļ��ġ������ļ���ʽʱ��MapFormat::supportsFile.��������
     * @return whether the file was successfully opened
     */
    bool openFile(const QString &fileName, FileFormat *fileFormat = nullptr);

    /**
     * Attempt to open the previously opened file.���Դ���ǰ�򿪵��ļ�
     */
    void openLastFiles();

protected:
    bool event(QEvent *event) override;

    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

    void keyPressEvent(QKeyEvent *) override;
    void keyReleaseEvent(QKeyEvent *) override;//����������ʾ�����û�д�д�麯���Ļ�

    void dragEnterEvent(QDragEnterEvent *) override;//override �ؼ��֣����Ա�����������������д�麯���Ĵ���(���û�д�д����ɵ��û����,��ͨ�麯��ͬʱ�̳нӿں�ȱʡʵ����Σ�յ�)
    void dropEvent(QDropEvent *) override;

private slots:
    void newMap();
    void openFileDialog();
    bool saveFile();
    bool saveFileAs();
    void saveAll();
    void export_(); // 'export' is a reserved word������һ��������
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
    void openPreferences();//����ѡ��

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
    void editMapProperties();//�༭��ͼ�ĵ���(����)

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
      * document.����Ҫ��ʱ�����û��Ƿ���Ҫ���������mapDocument.�����Ҫѯ�ʣ��������Ҫ�ľ�ȷ�����ǵ�ǰ���ĵ���
      *
      * @return <code>true</code> when any unsaved data is either discarded or
      *         saved, <code>false</code> when the user cancelled or saving
      *         failed.���κ�δ��������ݱ������򱣴�ʱ������true,���û�ȡ�����߱���ʧ�ܷ���false
      */
    bool confirmSave(Document *document);

    /**
      * Checks all maps for changes, if so, ask if to save these changes.
      *������еĵ�ͼ�Ƿ������ģ�����ǵĻ������Ƿ���Ҫ������ġ�
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
