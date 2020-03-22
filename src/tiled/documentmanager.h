/*
 * documentmanager.h
 * Copyright 2010, Stefan Beller <stefanbeller@googlemail.com>
 * Copyright 2010, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
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

#include "mapdocument.h"
#include "tilesetdocument.h"

#include <QHash>
#include <QList>
#include <QObject>
#include <QPointF>
#include <QVector>

class QTabWidget;
class QUndoGroup;
class QStackedLayout;
class QTabBar;

namespace Tiled {

class FileSystemWatcher;
class ObjectTemplate;

namespace Internal {

class AbstractTool;
class BrokenLinksModel;
class BrokenLinksWidget;
class Document;
class Editor;
class FileChangedWarning;
class MapDocument;
class MapEditor;
class MapScene;
class MapView;
class TilesetDocument;
class TilesetDocumentsModel;

/**
 * This class controls the open documents.������ǿ��ƴ򿪵��ĵ�
 */
class DocumentManager : public QObject
{
    Q_OBJECT

public:
    static DocumentManager *instance();
    static void deleteInstance();

    QWidget *widget() const;

    void setEditor(Document::DocumentType documentType, Editor *editor);
    Editor *editor(Document::DocumentType documentType) const;
    void deleteEditor(Document::DocumentType documentType);
    QList<Editor*> editors() const;

    Editor *currentEditor() const;

    void saveState();
    void restoreState();

    QUndoGroup *undoGroup() const;

    Document *currentDocument() const;

    MapView *currentMapView() const;
    MapView *viewForDocument(MapDocument *mapDocument) const;

    int findDocument(const QString &fileName) const;
    int findDocument(Document *document) const;

    void switchToDocument(int index);
    bool switchToDocument(Document *document);
    void switchToDocument(MapDocument *mapDocument, QPointF viewCenter, qreal scale);

    void addDocument(const DocumentPtr &document);

    bool isDocumentModified(Document *document) const;

    DocumentPtr loadDocument(const QString &fileName,
                             FileFormat *fileFormat = nullptr,
                             QString *error = nullptr);

    bool saveDocument(Document *document, const QString &fileName);
    bool saveDocumentAs(Document *document);

    void closeCurrentDocument();
    void closeAllDocuments();

    void closeOtherDocuments(int index);
    void closeDocumentsToRight(int index);
    void closeDocumentAt(int index);

    bool reloadCurrentDocument();
    bool reloadDocumentAt(int index);

    void checkTilesetColumns(MapDocument *mapDocument);
    bool checkTilesetColumns(TilesetDocument *tilesetDocument);

    const QVector<DocumentPtr> &documents() const;

    TilesetDocumentsModel *tilesetDocumentsModel() const;

    TilesetDocument *findTilesetDocument(const SharedTileset &tileset) const;
    TilesetDocument *findTilesetDocument(const QString &fileName) const;

    void openTileset(const SharedTileset &tileset);

    void centerMapViewOn(qreal x, qreal y);
    void centerMapViewOn(const QPointF &pos)
    { centerMapViewOn(pos.x(), pos.y()); }

    void abortMultiDocumentClose();

signals:
    void fileOpenDialogRequested();
    void fileOpenRequested(const QString &path);
    void fileSaveRequested();
    void templateOpenRequested(const QString &path);
    void templateTilesetReplaced();

    /**����ǰ��ʾ�ĵ�ͼ�ĵ������仯ʱ�����ġ�
     * Emitted when the current displayed map document changed.
     */
    void currentDocumentChanged(Document *document);

    /**���û�����ر������е��ĵ�ʱ��������Ϣ��
     * Emitted when the user requested the document at \a index to be closed.
     */
    void documentCloseRequested(int index);

    /**��һ���ĵ��������ر�ʱ������
     * Emitted when a document is about to be closed.
     */
    void documentAboutToClose(Document *document);

    /**�������¼��ص�ͼʱ��������
     * Emitted when an error occurred while reloading the map.
     */
    void reloadError(const QString &error);

    void tilesetDocumentAdded(TilesetDocument *tilesetDocument);
    void tilesetDocumentRemoved(TilesetDocument *tilesetDocument);

public slots:
    void switchToLeftDocument();
    void switchToRightDocument();

    void openFileDialog();
    void openFile(const QString &path);
    void saveFile();

private slots:
    void currentIndexChanged();
    void fileNameChanged(const QString &fileName,
                         const QString &oldFileName);
    void modifiedChanged();
    void updateDocumentTab(Document *document);
    void documentSaved();
    void documentTabMoved(int from, int to);
    void tabContextMenuRequested(const QPoint &pos);

    void tilesetAdded(int index, Tileset *tileset);
    void tilesetRemoved(Tileset *tileset);
    void tilesetReplaced(int index, Tileset *tileset, Tileset *oldTileset);

    void tilesetNameChanged(Tileset *tileset);

    void fileChanged(const QString &fileName);
    void hideChangedWarning();

    void tilesetImagesChanged(Tileset *tileset);

private:
    DocumentManager(QObject *parent = nullptr);
    ~DocumentManager() override;

    bool askForAdjustment(const Tileset &tileset);

    void addToTilesetDocument(const SharedTileset &tileset, MapDocument *mapDocument);
    void removeFromTilesetDocument(const SharedTileset &tileset, MapDocument *mapDocument);

    bool eventFilter(QObject *object, QEvent *event) override;

    QVector<DocumentPtr> mDocuments;//docmuents save type
    TilesetDocumentsModel *mTilesetDocumentsModel;

    QWidget *mWidget;
    QWidget *mNoEditorWidget;
    QTabBar *mTabBar;
    FileChangedWarning *mFileChangedWarning;
    BrokenLinksModel *mBrokenLinksModel;
    BrokenLinksWidget *mBrokenLinksWidget;
    QStackedLayout *mEditorStack;
    MapEditor *mMapEditor;

    QHash<Document::DocumentType, Editor*> mEditorForType;

    QUndoGroup *mUndoGroup;
    FileSystemWatcher *mFileSystemWatcher;

    static DocumentManager *mInstance;

    bool mMultiDocumentClose;
};

/**
 * Returns the undo group that combines the undo stacks of all opened
 * documents.
 *�����������д򿪵��ĵ��ĳ�����ջ�ĳ����顣
 * @see Document::undoStack()
 */
inline QUndoGroup *DocumentManager::undoGroup() const
{
    return mUndoGroup;
}

/**
 * Returns all open documents.
 */
inline const QVector<DocumentPtr> &DocumentManager::documents() const
{
    return mDocuments;
}

inline TilesetDocumentsModel *DocumentManager::tilesetDocumentsModel() const
{
    return mTilesetDocumentsModel;
}

} // namespace Tiled::Internal
} // namespace Tiled
