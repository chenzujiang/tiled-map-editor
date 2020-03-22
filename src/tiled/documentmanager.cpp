/*
 * documentmanager.cpp
 * Copyright 2010, Stefan Beller <stefanbeller@googlemail.com>
 * Copyright 2010-2016, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
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

#include "documentmanager.h"

#include "abstracttool.h"
#include "adjusttileindexes.h"
#include "brokenlinks.h"
#include "editor.h"
#include "filechangedwarning.h"
#include "filesystemwatcher.h"
#include "mapdocument.h"
#include "mapeditor.h"
#include "mapformat.h"
#include "map.h"
#include "maprenderer.h"
#include "mapscene.h"
#include "mapview.h"
#include "noeditorwidget.h"
#include "preferences.h"
#include "tilesetdocument.h"
#include "tilesetdocumentsmodel.h"
#include "tilesetmanager.h"
#include "tmxmapformat.h"
#include "utils.h"
#include "zoomable.h"

#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QScrollBar>
#include <QStackedLayout>
#include <QTabBar>
#include <QTabWidget>
#include <QUndoGroup>
#include <QUndoStack>
#include <QVBoxLayout>
//http://www.kuqin.com/qtdocument/qmainwindow.html  QMainwidow
#include "qtcompat_p.h"

using namespace Tiled;
using namespace Tiled::Internal;


DocumentManager *DocumentManager::mInstance;

DocumentManager *DocumentManager::instance()
{
    if (!mInstance)
        mInstance = new DocumentManager;
    return mInstance;
}

void DocumentManager::deleteInstance()
{
    delete mInstance;
    mInstance = nullptr;
}

DocumentManager::DocumentManager(QObject *parent)
    : QObject(parent)
    , mTilesetDocumentsModel(new TilesetDocumentsModel(this))
    , mWidget(new QWidget)//center widget
    , mNoEditorWidget(new NoEditorWidget(mWidget))/*new open file three button*/
    , mTabBar(new QTabBar(mWidget))
    , mFileChangedWarning(new FileChangedWarning(mWidget))/*reload and ignore(��ʼ����)*/
    , mBrokenLinksModel(new BrokenLinksModel(this))
    , mBrokenLinksWidget(new BrokenLinksWidget(mBrokenLinksModel, mWidget))
    , mMapEditor(nullptr) // todo����¼/�������: look into removing this�����Ƴ����
    , mUndoGroup(new QUndoGroup(this))
    , mFileSystemWatcher(new FileSystemWatcher(this))
    , mMultiDocumentClose(false)
{
    mBrokenLinksWidget->setVisible(true);/*�м��*/

    mTabBar->setExpanding(false);//When expanding(��չ) is true QTabBar will expand the tabs to use the empty space.
    mTabBar->setDocumentMode(true);//������԰�����ǩ���Ƿ����ʺ������ڵ�ģʽ���֡�
    mTabBar->setTabsClosable(true);//������԰���һ����ǩ���Ƿ�Ӧ����ÿ��ѡ��Ϸ��ùرհ�ť
    mTabBar->setMovable(true);//This property holds whether the user can move the tabs within the tabbar area.(�϶�)
    mTabBar->setContextMenuPolicy(Qt::CustomContextMenu);//how the widget shows a context menu

    mFileChangedWarning->setVisible(true);

    connect(mFileChangedWarning, &FileChangedWarning::reload, this, &DocumentManager::reloadCurrentDocument);
    connect(mFileChangedWarning, &FileChangedWarning::ignore, this, &DocumentManager::hideChangedWarning);

    QVBoxLayout *vertical = new QVBoxLayout(mWidget);//mWidget��ֱ�����и��ؼ�,(tabbar,FileChangeWarning,Broken,mEditorStack[!3]),
    vertical->addWidget(mTabBar);//tabBar
    vertical->addWidget(mFileChangedWarning);//FileChangedWarning
    vertical->addWidget(mBrokenLinksWidget);//next ui layout
    vertical->setMargin(0);
    vertical->setSpacing(0);
    //�ڲ����������һ��stackedlayout,��ŵĵ�һ��������ͼ�п������½���ͼ(�ÿؼ���designer�е�)��ջ���ֺ���Ҫ
    mEditorStack = new QStackedLayout;
    mEditorStack->addWidget(mNoEditorWidget);//mEditorStackջװ�ľ���������mNoEditorWidget,mapEidtor(��stack),TilesetEidtor(��stack)[1]
    vertical->addLayout(mEditorStack);

    connect(mTabBar, &QTabBar::currentChanged,
            this, &DocumentManager::currentIndexChanged);
    connect(mTabBar, &QTabBar::tabCloseRequested,
            this, &DocumentManager::documentCloseRequested);
    connect(mTabBar, &QTabBar::tabMoved,
            this, &DocumentManager::documentTabMoved);
    connect(mTabBar, &QWidget::customContextMenuRequested,
            this, &DocumentManager::tabContextMenuRequested);

    connect(mFileSystemWatcher, &FileSystemWatcher::fileChanged,
            this, &DocumentManager::fileChanged);

    connect(mBrokenLinksModel, &BrokenLinksModel::hasBrokenLinksChanged,
            mBrokenLinksWidget, &BrokenLinksWidget::setVisible);

    connect(TilesetManager::instance(), &TilesetManager::tilesetImagesChanged,
            this, &DocumentManager::tilesetImagesChanged);

    mTabBar->installEventFilter(this);//�ڸö����ϰ�װһ���¼�������filterObj��
}

DocumentManager::~DocumentManager()
{
    mTabBar->removeEventFilter(this);

    // All documents should be closed gracefully beforehand���е��ĵ���Ӧ����Ԥ�ȹر�
    Q_ASSERT(mDocuments.isEmpty());
    Q_ASSERT(mTilesetDocumentsModel->rowCount() == 0);
    delete mWidget;
}

/**
 * Returns the document manager widget. It contains the different map views
 * and a tab bar to switch between them.
 * ����һ�� document manager �ؼ����������˲�ͬ�ĵ�ͼ��ͼ,һ��tab bar������(��ͼ)֮������л�
 */
QWidget *DocumentManager::widget() const
{
    return mWidget;
}

void DocumentManager::setEditor(Document::DocumentType documentType, Editor *editor)
{
    Q_ASSERT(!mEditorForType.contains(documentType));
    mEditorForType.insert(documentType, editor);
    mEditorStack->addWidget(editor->editorWidget());//mEditorStackջװ�ľ���������no,mapEidtor(��stack),TilesetEidtor(��stack)[2,3]

    if (MapEditor *mapEditor = qobject_cast<MapEditor*>(editor))
        mMapEditor = mapEditor;
}

Editor *DocumentManager::editor(Document::DocumentType documentType) const
{
    return mEditorForType.value(documentType);//key->return value
}

void DocumentManager::deleteEditor(Document::DocumentType documentType)
{
    Q_ASSERT(mEditorForType.contains(documentType));
    Editor *editor = mEditorForType.take(documentType);
    if (editor == mMapEditor)
        mMapEditor = nullptr;
    delete editor;
}

QList<Editor *> DocumentManager::editors() const
{
    return mEditorForType.values();
}

Editor *DocumentManager::currentEditor() const
{
    if (const auto document = currentDocument())
        return editor(document->type()/*mapdoc or tilesetdoc*/);

    return nullptr;
}

void DocumentManager::saveState()
{
    QHashIterator<Document::DocumentType, Editor*> iterator(mEditorForType);
    while (iterator.hasNext())
        iterator.next().value()->saveState();
}
//�ָ�/��ԭ״̬
void DocumentManager::restoreState()
{
    QHashIterator<Document::DocumentType, Editor*> iterator(mEditorForType);
    while (iterator.hasNext())
        iterator.next().value()->restoreState();
}

/**
 * Returns the current map document, or 0 when there is none.����һ����ǰҳ���ĵ���������
 */
Document *DocumentManager::currentDocument() const
{
    const int index = mTabBar->currentIndex();
    if (index == -1)
        return nullptr;
    //Vector<QSharedPointer<Document>> a.at(index).data()���ع���ָ��ָ�������ֵ //Returns the value of the pointer referenced by this object.
    return mDocuments.at(index).data();//Document*//data()���ظö������õ�ָ���ֵ
}

/**
 * Returns the map view of the current document, or 0 when there is none.
 */
MapView *DocumentManager::currentMapView() const
{
    return mMapEditor->currentMapView();
}

/**������ʾָ���ĵ��ĵ�ͼ,û�з���null
 * Returns the map view that displays the given document, or null when there is none.
 */
MapView *DocumentManager::viewForDocument(MapDocument *mapDocument) const
{
    return mMapEditor->viewForDocument(mapDocument);
}

/**
 * Searches for a document with the given \a fileName and returns its
 * index. Returns -1 when the document isn't open.
 */
int DocumentManager::findDocument(const QString &fileName) const
{
    const QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();//�ļ��ľ���·��
    if (canonicalFilePath.isEmpty()) // file doesn't exist
        return -1;
    //���ĵ����������ҵ���Ҫ�򿪵��ĵ��������ظҵ�������(����֮���Ѿ���ӵ����������˵ģ�����2018-10-1)
    for (int i = 0; i < mDocuments.size(); ++i) {
        QFileInfo fileInfo(mDocuments.at(i)->fileName());
        if (fileInfo.canonicalFilePath() == canonicalFilePath)//���ļ��������е��ļ��Ͳ����ļ���·����һ��
            return i;
    }

    return -1;
}

int DocumentManager::findDocument(Document *document) const
{
    auto i = std::find(mDocuments.begin(), mDocuments.end(), document);
    return i != mDocuments.end() ? static_cast<int>(i - mDocuments.begin()) : -1;
}

/**
 * Switches to the map document at the given \a index.
 */
void DocumentManager::switchToDocument(int index)
{
    mTabBar->setCurrentIndex(index);
}

/**
 * Switches to the given \a document, if there is already a tab open for it.
 * \return whether the switch was succesful
 */
bool DocumentManager::switchToDocument(Document *document)
{
    const int index = findDocument(document);
    if (index != -1) {
        switchToDocument(index);
        return true;
    }

    return false;
}

/**
 * Switches to the given \a mapDocument, centering the view on \a viewCenter
 * (scene coordinates) at the given \a scale.
 *
 * If the given map document is not open yet, a tab will be created for it.
 */
void DocumentManager::switchToDocument(MapDocument *mapDocument, QPointF viewCenter, qreal scale)
{
    if (!switchToDocument(mapDocument))
        addDocument(mapDocument->sharedFromThis());

    MapView *view = currentMapView();
    view->zoomable()->setScale(scale);
    view->forceCenterOn(viewCenter);
}

void DocumentManager::switchToLeftDocument()
{
    const int tabCount = mTabBar->count();
    if (tabCount < 2)
        return;

    const int currentIndex = mTabBar->currentIndex();
    switchToDocument((currentIndex > 0 ? currentIndex : tabCount) - 1);
}

void DocumentManager::switchToRightDocument()
{
    const int tabCount = mTabBar->count();
    if (tabCount < 2)
        return;

    const int currentIndex = mTabBar->currentIndex();
    switchToDocument((currentIndex + 1) % tabCount);
}

void DocumentManager::openFileDialog()
{
    emit fileOpenDialogRequested();
}

void DocumentManager::openFile(const QString &path)
{
    emit fileOpenRequested(path);
}

void DocumentManager::saveFile()
{
    emit fileSaveRequested();
}

/**
 * Adds the new or opened \a document to the document manager.
 */
void DocumentManager::addDocument(const DocumentPtr &document)
{
    Q_ASSERT(document);
    Q_ASSERT(!mDocuments.contains(document));//false assert if has pointer

    mDocuments.append(document);
    mUndoGroup->addStack(document->undoStack());

    if (auto mapDocument = qobject_cast<MapDocument*>(document.data())) {//Returns the value of the pointer referenced by this object.
        for (const SharedTileset &tileset : mapDocument->map()->tilesets())
            addToTilesetDocument(tileset, mapDocument);//��mapDocument��map()�е�tilesets��ӵ�mapDocument��
    } else if (auto tilesetDocument = qobject_cast<TilesetDocument*>(document.data())) {
        // We may have opened a bare tileset that wasn't seen before
        if (!mTilesetDocumentsModel->contains(tilesetDocument)) {
            mTilesetDocumentsModel->append(tilesetDocument);
            emit tilesetDocumentAdded(tilesetDocument);
        }
    }

    if (!document->fileName().isEmpty())
        mFileSystemWatcher->addPath(document->fileName());
//core
    if (Editor *editor = mEditorForType.value(document->type()))//ͨ���ĵ����ͻ�ȡ�༭��(Editor��MapEdirtor��TilesetEditor)
        editor->addDocument(document.data());//�������͵��ļ���ӵ��༭����

    QString tabText = document->displayName();
    if (document->isModified())//�ĵ����޸���ʾ*
        tabText.prepend(QLatin1Char('*'));

    const int documentIndex = mTabBar->addTab(tabText);//Adds a new tab with text text. Returns the new tab's index.
    mTabBar->setTabToolTip(documentIndex, document->fileName());//����tab��indexλ�ù�����ʾΪtip(�ļ���),��������ȥ����ʾ�ļ���
//core
    connect(document.data(), &Document::fileNameChanged, this, &DocumentManager::fileNameChanged);
    connect(document.data(), &Document::modifiedChanged, this, &DocumentManager::modifiedChanged);
    connect(document.data(), &Document::saved, this, &DocumentManager::documentSaved);//�����źŵ�ʱ��û�����ӵ���

    if (auto *mapDocument = qobject_cast<MapDocument*>(document.data())) {
        connect(mapDocument, &MapDocument::tilesetAdded, this, &DocumentManager::tilesetAdded);
        connect(mapDocument, &MapDocument::tilesetRemoved, this, &DocumentManager::tilesetRemoved);
        connect(mapDocument, &MapDocument::tilesetReplaced, this, &DocumentManager::tilesetReplaced);
    }

    if (auto *tilesetDocument = qobject_cast<TilesetDocument*>(document.data())) {
        connect(tilesetDocument, &TilesetDocument::tilesetNameChanged, this, &DocumentManager::tilesetNameChanged);
    }

    switchToDocument(documentIndex);

    if (mBrokenLinksModel->hasBrokenLinks())
        mBrokenLinksWidget->show();

    // todo: fix this (move to MapEditor)
    //    centerViewOn(0, 0);
}

/**���ظ����ĵ��Ƿ���δ������޸ġ�����Ƕ��tileset�ĵ�ͼ�ļ������������κ�Ƕ���tileset�Ƿ���δ������޸ġ�
 * Returns whether the given document has unsaved modifications.
 * For map files with embedded tilesets,
 * that includes checking whether any of the embedded tilesets have unsaved modifications.
 */
bool DocumentManager::isDocumentModified(Document *document) const
{
    if (auto mapDocument = qobject_cast<MapDocument*>(document)) {
        for (const SharedTileset &tileset : mapDocument->map()->tilesets()) {
            if (const auto tilesetDocument = findTilesetDocument(tileset))
                if (tilesetDocument->isEmbedded() && tilesetDocument->isModified())
                    return true;
        }
    }

    return document->isModified();//���stack isclean state ��û���޸�
}

/**�����Ƿ��ڴ����ϸ����˸������ĵ������ǵ������ĵ���Ƕ���tileset�ĵ��������
 * Returns whether the given document was changed on disk. Taking into account
 * the case where the given document is an embedded tileset document.
 */
static bool isDocumentChangedOnDisk(Document *document)
{
    if (auto tilesetDocument = qobject_cast<TilesetDocument*>(document)) {
        if (tilesetDocument->isEmbedded())
            document = tilesetDocument->mapDocuments().first();
    }

    return document->changedOnDisk();
}

DocumentPtr DocumentManager::loadDocument(const QString &fileName,
                                          FileFormat *fileFormat,
                                          QString *error)
{
    // Return existing document if this file is already open
    int documentIndex = findDocument(fileName);
    if (documentIndex != -1)
        return mDocuments.at(documentIndex);

    // Try to find it in otherwise referenced documents�������������õ��ĵ����ҵ���
    {
        QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();//��ȡ�ļ��ı�׼·��
        for (Document *doc : Document::documentInstances()) {
            if (QFileInfo(doc->fileName()).canonicalFilePath() == canonicalFilePath)
                return doc->sharedFromThis();
        }//If this (that is, the subclass instance invoking this method) is being managed by a QSharedPointer,
        //returns a shared pointer instance pointing to this; otherwise returns a QSharedPointer holding a null pointer.����ָ���document����ָ�����;���򷵻�һ��������ָ���QSharedPointer��
    }
    //mutable ���α���Ϊ�ױ��,��ʱ��const�������档һ��ĳ����Ա�������ı�������Ծ�����Ϊcount����
    if (!fileFormat) {
        // Try to find a plugin that implements support for this format�Ӳ�����������ҵ����ʵ��(֧�ָø�ʽ��)
        const auto formats = PluginManager::objects<FileFormat>();
        for (FileFormat *format : formats) {
            if (format->supportsFile(fileName)) {
                fileFormat = format;
                break;
            }
        }
    }

    if (!fileFormat) {
        if (error)
            *error = tr("Unrecognized file format.");//�޷�ʶ����ļ���ʽ
        return DocumentPtr();
    }

    DocumentPtr document;

    if (MapFormat *mapFormat = qobject_cast<MapFormat*>(fileFormat)) {
        document = MapDocument::load(fileName, mapFormat, error);
    } else if (TilesetFormat *tilesetFormat = qobject_cast<TilesetFormat*>(fileFormat)) {
        // It could be, that we have already loaded this tileset while loading some map.�п��ܣ������ڼ��ص�ͼʱ�Ѿ����������tileset��
        if (auto tilesetDocument = findTilesetDocument(fileName)) {
            document = tilesetDocument->sharedFromThis();
        } else {
            document = TilesetDocument::load(fileName, tilesetFormat, error);
        }
    }

    return document;
}

/**����������ֵĸ����ĵ�,�������ɹ�,���ļ���ӵ�������ļ�������
 * Save the given document with the given file name. When saved
 * successfully, the file is added to the list of recent files.
 *
 * @return <code>true</code> on success, <code>false</code> on failure
 */
bool DocumentManager::saveDocument(Document *document, const QString &fileName)
{
    if (fileName.isEmpty())
        return false;

    QString error;
    if (!document->save(fileName, &error)) {
        QMessageBox::critical(mWidget->window(), QCoreApplication::translate("Tiled::Internal::MainWindow", "Error Saving File"), error);
        return false;
    }

    Preferences::instance()->addRecentFile(fileName);
    return true;
}

/**(����һ���ĵ����û�������(ѡ��)���ļ������ĵ�);;����һ�����û��Զ����ļ������ĵ�
 * Save the given document with a file name chosen by the user.
 * ����ɹ�,�ļ���ӵ�����ļ�������
 *  When saved successfully, the file is added to the list of recent files.
 *
 * @return <code>true</code> on success, <code>false</code> on failure
 */
bool DocumentManager::saveDocumentAs(Document *document)
{
    QString filter;
    QString selectedFilter;
    QString fileName = document->fileName();// 2018-10-1

    if (FileFormat *format = document->writerFormat())// 2018-10-1 writerFormat() ceate MapDocument no init
        selectedFilter = format->nameFilter();//.tmx,.xml���ظ����͵ĵ�ͼ��֧�ֵ��ļ���ʽ
    //declaration funcation pointer( = lambda ���ʽ)
    auto getSaveFileName = [&,this](const QString &defaultFileName) {
        if (fileName.isEmpty()) {
            fileName = Preferences::instance()->fileDialogStartLocation();//������Ǹ��ļ����ڵ�·��
            fileName += QLatin1Char('/');
            fileName += defaultFileName;
        }

        while (true) {
            fileName = QFileDialog::getSaveFileName(mWidget->window(),
                                                    QString(),
                                                    fileName,
                                                    filter,
                                                    &selectedFilter);

            if (!fileName.isEmpty() &&//�Ƿ��������չ��ƥ��������ļ�
                !Utils::fileNameMatchesNameFilter(QFileInfo(fileName).fileName(), selectedFilter))
            {
                QMessageBox messageBox(QMessageBox::Warning,
                                       QCoreApplication::translate("Tiled::Internal::MainWindow", "Extension Mismatch"),
                                       QCoreApplication::translate("Tiled::Internal::MainWindow", "The file extension does not match the chosen file type."),
                                       QMessageBox::Yes | QMessageBox::No,
                                       mWidget->window());

                messageBox.setInformativeText(QCoreApplication::translate("Tiled::Internal::MainWindow",
                                                                          "Tiled may not automatically recognize your file when loading. "
                                                                          "Are you sure you want to save with this extension?"));

                int answer = messageBox.exec();
                if (answer != QMessageBox::Yes)
                    continue;
            }
            return fileName;//return lamdba funcation
        }
    };

    if (auto mapDocument = qobject_cast<MapDocument*>(document)) {
        if (selectedFilter.isEmpty())//if document is MapDocument && selectedFilter isEmpty
            selectedFilter = TmxMapFormat().nameFilter();//"Tiled map files (*.tmx *.xml)"
        /*MapFormatΪ���͹�������Ķ���Helper,�������������ֵ�ù�ע��֪ʶ
         * 1 QList<MapFormat*> mFormats�����洢�˻���������Ӷ���
         * 2 QMap<QString, MapFormat*> mFormatByNameFilter;���������еĶ���Ҫһ����(�ö�����֧�ֵ������ļ���ʽ)����(object)��Ӧ
         */
        FormatHelper<MapFormat> helper(FileFormat::ReadWrite);
        filter = helper.filter();//����֧�ָ�MapFormat����������ļ���ʽ(��������б��������Ը�ʽ����)
        qDebug()<<filter ;
        auto suggestedFileName = QCoreApplication::translate("Tiled::Internal::MainWindow", "untitled");
        suggestedFileName.append(QLatin1String(".tmx"));

        fileName = getSaveFileName(suggestedFileName);
        if (fileName.isEmpty())
            return false;

        MapFormat *format = helper.formatByNameFilter(selectedFilter);//ͨ���ö����ʽ��֧�ֵĸ�ʽ�ַ�����ø�ʽ����
        mapDocument->setWriterFormat(format);//Ϊ���ĵ��������ã���ʽ����
        mapDocument->setReaderFormat(format);

    } else if (auto tilesetDocument = qobject_cast<TilesetDocument*>(document)) {
        if (selectedFilter.isEmpty())
            selectedFilter = TsxTilesetFormat().nameFilter();

        FormatHelper<TilesetFormat> helper(FileFormat::ReadWrite);
        filter = helper.filter();

        auto suggestedFileName = tilesetDocument->tileset()->name().trimmed();
        if (suggestedFileName.isEmpty())
            suggestedFileName = QCoreApplication::translate("Tiled::Internal::MainWindow", "untitled");
        suggestedFileName.append(QLatin1String(".tsx"));

        fileName = getSaveFileName(suggestedFileName);
        if (fileName.isEmpty())
            return false;

        TilesetFormat *format = helper.formatByNameFilter(selectedFilter);
        tilesetDocument->setWriterFormat(format);
    }

    return saveDocument(document, fileName);
}

/**
 * Closes the current map document. Will not ask the user whether to save
 * any changes!
 */
void DocumentManager::closeCurrentDocument()
{
    const int index = mTabBar->currentIndex();
    if (index == -1)
        return;

    closeDocumentAt(index);
}

/**
 * Close all documents. Will not ask the user whether to save any changes!
 */
void DocumentManager::closeAllDocuments()
{
    while (!mDocuments.isEmpty())
        closeCurrentDocument();
}

/**�ر������ĵ�������������ָ����ĵ���
 * Closes all documents except the one pointed to by index.
 */
void DocumentManager::closeOtherDocuments(int index)
{
    if (index == -1)
        return;

    mMultiDocumentClose = true;

    for (int i = mTabBar->count() - 1; i >= 0; --i) {
        if (i != index)
            documentCloseRequested(i);

        if (!mMultiDocumentClose)
            return;
    }
}

/**
 * Closes all documents whose tabs are to the right of the index.
 */
void DocumentManager::closeDocumentsToRight(int index)
{
    if (index == -1)
        return;

    mMultiDocumentClose = true;

    for (int i = mTabBar->count() - 1; i > index; --i) {
        documentCloseRequested(i);

        if (!mMultiDocumentClose)
            return;
    }
}

/**
 * Closes the document at the given \a index. Will not ask the user whether
 * to save any changes!
 */
void DocumentManager::closeDocumentAt(int index)
{
    auto document = mDocuments.at(index);       // keeps alive and may delete

    emit documentAboutToClose(document.data());

    mDocuments.removeAt(index);
    mTabBar->removeTab(index);

    if (Editor *editor = mEditorForType.value(document->type()))
        editor->removeDocument(document.data());

    if (!document->fileName().isEmpty()) {
        mFileSystemWatcher->removePath(document->fileName());
        document->setChangedOnDisk(false);
    }

    if (auto mapDocument = qobject_cast<MapDocument*>(document.data())) {
        for (const SharedTileset &tileset : mapDocument->map()->tilesets())
            removeFromTilesetDocument(tileset, mapDocument);
    } else if (auto tilesetDocument = qobject_cast<TilesetDocument*>(document.data())) {
        if (tilesetDocument->mapDocuments().isEmpty()) {
            mTilesetDocumentsModel->remove(tilesetDocument);
            emit tilesetDocumentRemoved(tilesetDocument);
        } else {
            tilesetDocument->disconnect(this);
        }
    }
}

/**
 * Reloads the current document. Will not ask the user whether to save any
 * changes!
 *
 * \sa reloadDocumentAt()
 */
bool DocumentManager::reloadCurrentDocument()
{
    const int index = mTabBar->currentIndex();
    if (index == -1)
        return false;

    return reloadDocumentAt(index);
}

/**
 * Reloads the document at the given \a index. It will lose any undo
 * history and current selections. Will not ask the user whether to save
 * any changes!
 *
 * Returns whether the map loaded successfully.
 */
bool DocumentManager::reloadDocumentAt(int index)
{
    const auto oldDocument = mDocuments.at(index);
    QString error;

    if (auto mapDocument = oldDocument.objectCast<MapDocument>()) {
        // TODO: Consider fixing the reload to avoid recreating the MapDocument
        auto newDocument = MapDocument::load(oldDocument->fileName(),
                                             mapDocument->readerFormat(),
                                             &error);
        if (!newDocument) {
            emit reloadError(tr("%1:\n\n%2").arg(oldDocument->fileName(), error));
            return false;
        }

        // Replace old tab
        addDocument(newDocument);
        closeDocumentAt(index);
        mTabBar->moveTab(mDocuments.size() - 1, index);

        checkTilesetColumns(newDocument.data());

    } else if (auto tilesetDocument = qobject_cast<TilesetDocument*>(oldDocument)) {
        if (tilesetDocument->isEmbedded()) {
            // For embedded tilesets, we need to reload the map
            index = findDocument(tilesetDocument->mapDocuments().first());
            if (!reloadDocumentAt(index))
                return false;
        } else if (!tilesetDocument->reload(&error)) {
            emit reloadError(tr("%1:\n\n%2").arg(oldDocument->fileName(), error));
            return false;
        }

        tilesetDocument->setChangedOnDisk(false);
    }

    if (!isDocumentChangedOnDisk(currentDocument()))
        mFileChangedWarning->setVisible(true);//false

    return true;
}

void DocumentManager::currentIndexChanged()
{
    auto document = currentDocument();
    Editor *editor = nullptr;
    bool changed = false;

    if (document) {
        editor = mEditorForType.value(document->type());//MapDocumentType,TilesetDocumentType
        mUndoGroup->setActiveStack(document->undoStack());

        changed = isDocumentChangedOnDisk(document);
    }

    if (editor) {
        editor->setCurrentDocument(document);
        mEditorStack->setCurrentWidget(editor->editorWidget());
    } else {
        mEditorStack->setCurrentWidget(mNoEditorWidget);
    }

    mFileChangedWarning->setVisible(changed);

    mBrokenLinksModel->setDocument(document);

    emit currentDocumentChanged(document);
}

void DocumentManager::fileNameChanged(const QString &fileName,
                                      const QString &oldFileName)
{
    if (!fileName.isEmpty())
        mFileSystemWatcher->addPath(fileName);
    if (!oldFileName.isEmpty())
        mFileSystemWatcher->removePath(oldFileName);

    // Update the tabs for all opened embedded tilesets
    Document *document = static_cast<Document*>(sender());
    if (MapDocument *mapDocument = qobject_cast<MapDocument*>(document)) {
        for (const SharedTileset &tileset : mapDocument->map()->tilesets()) {
            if (auto tilesetDocument = findTilesetDocument(tileset))
                updateDocumentTab(tilesetDocument);
        }
    }

    updateDocumentTab(document);
}

void DocumentManager::modifiedChanged()
{
    updateDocumentTab(static_cast<Document*>(sender()));
}

void DocumentManager::updateDocumentTab(Document *document)
{
    const int index = findDocument(document);
    if (index == -1)
        return;

    QString tabText = document->displayName();
    if (document->isModified())
        tabText.prepend(QLatin1Char('*'));

    mTabBar->setTabText(index, tabText);
    mTabBar->setTabToolTip(index, document->fileName());
}

void DocumentManager::documentSaved()
{
    Document *document = static_cast<Document*>(sender());

    if (document->changedOnDisk()) {
        document->setChangedOnDisk(false);
        if (!isDocumentModified(currentDocument()))
            mFileChangedWarning->setVisible(false);
    }
}
/***
 * @projectName   Tiled
 * @brief         �ƶ�Tab�е��ļ�index��ĳ��index
 * @author        Casey.Chen
 * @date          2019-05-18
 */
void DocumentManager::documentTabMoved(int from, int to)
{
#if QT_VERSION >= 0x050600
    mDocuments.move(from, to);
#else
    mDocuments.insert(to, mDocuments.takeAt(from));
#endif
}

void DocumentManager::tabContextMenuRequested(const QPoint &pos)
{
    int index = mTabBar->tabAt(pos);
    if (index == -1)
        return;

    QMenu menu(mTabBar->window());

    Utils::addFileManagerActions(menu, mDocuments.at(index)->fileName());

    menu.addSeparator();

    QAction *closeTab = menu.addAction(tr("Close"));
    closeTab->setIcon(QIcon(QStringLiteral(":/images/16x16/window-close.png")));
    Utils::setThemeIcon(closeTab, "window-close");
    connect(closeTab, &QAction::triggered, [this, index] {
        documentCloseRequested(index);
    });

    QAction *closeOtherTabs = menu.addAction(tr("Close Other Tabs"));
    connect(closeOtherTabs, &QAction::triggered, [this, index] {
        closeOtherDocuments(index);
    });

    QAction *closeTabsToRight = menu.addAction(tr("Close Tabs to the Right"));
    connect(closeTabsToRight, &QAction::triggered, [this, index] {
        closeDocumentsToRight(index);
    });

    menu.exec(mTabBar->mapToGlobal(pos));
}

void DocumentManager::tilesetAdded(int index, Tileset *tileset)
{
    Q_UNUSED(index)
    MapDocument *mapDocument = static_cast<MapDocument*>(QObject::sender());
    addToTilesetDocument(tileset->sharedPointer(), mapDocument);
}

void DocumentManager::tilesetRemoved(Tileset *tileset)
{
    MapDocument *mapDocument = static_cast<MapDocument*>(QObject::sender());
    removeFromTilesetDocument(tileset->sharedPointer(), mapDocument);
}

void DocumentManager::tilesetReplaced(int index, Tileset *tileset, Tileset *oldTileset)
{
    Q_UNUSED(index)
    MapDocument *mapDocument = static_cast<MapDocument*>(QObject::sender());
    addToTilesetDocument(tileset->sharedPointer(), mapDocument);
    removeFromTilesetDocument(oldTileset->sharedPointer(), mapDocument);
}

void DocumentManager::tilesetNameChanged(Tileset *tileset)
{
    auto *tilesetDocument = findTilesetDocument(tileset->sharedPointer());
    if (tilesetDocument->isEmbedded())
        updateDocumentTab(tilesetDocument);
}

void DocumentManager::fileChanged(const QString &fileName)
{
    const int index = findDocument(fileName);

    // Most likely the file was removed
    if (index == -1)
        return;

    const auto &document = mDocuments.at(index);

    // Ignore change event when it seems to be our own save
    if (QFileInfo(fileName).lastModified() == document->lastSaved())
        return;

    // Automatically reload when there are no unsaved changes
    if (!isDocumentModified(document.data())) {
        reloadDocumentAt(index);
        return;
    }

    document->setChangedOnDisk(true);

    if (isDocumentChangedOnDisk(currentDocument()))
        mFileChangedWarning->setVisible(true);
}

void DocumentManager::hideChangedWarning()
{
    Document *document = currentDocument();
    if (auto tilesetDocument = qobject_cast<TilesetDocument*>(document)) {
        if (tilesetDocument->isEmbedded())
            document = tilesetDocument->mapDocuments().first();
    }

    document->setChangedOnDisk(false);
    mFileChangedWarning->setVisible(false);
}

/**
 * Centers the current map on the pixel coordinates \a x, \a y.
 */
void DocumentManager::centerMapViewOn(qreal x, qreal y)
{
    if (MapView *view = currentMapView()) {
        auto mapDocument = view->mapScene()->mapDocument();
        view->centerOn(mapDocument->renderer()->pixelToScreenCoords(x, y));
    }
}

TilesetDocument* DocumentManager::findTilesetDocument(const SharedTileset &tileset) const
{
    return TilesetDocument::findDocumentForTileset(tileset);
}

TilesetDocument* DocumentManager::findTilesetDocument(const QString &fileName) const
{
    const QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();
    if (canonicalFilePath.isEmpty()) // file doesn't exist
        return nullptr;

    for (auto tilesetDocument : mTilesetDocumentsModel->tilesetDocuments()) {
        QString name = tilesetDocument->fileName();
        if (!name.isEmpty() && QFileInfo(name).canonicalFilePath() == canonicalFilePath)
            return tilesetDocument.data();
    }

    return nullptr;
}

/**
 * Opens the document for the given \a tileset.
 */
void DocumentManager::openTileset(const SharedTileset &tileset)
{
    TilesetDocumentPtr tilesetDocument;
    if (auto existingTilesetDocument = findTilesetDocument(tileset))
        tilesetDocument = existingTilesetDocument->sharedFromThis();
    else
        tilesetDocument = TilesetDocumentPtr::create(tileset);

    if (!switchToDocument(tilesetDocument.data()))
        addDocument(tilesetDocument);
}

void DocumentManager::addToTilesetDocument(const SharedTileset &tileset, MapDocument *mapDocument)
{
    if (auto existingTilesetDocument = findTilesetDocument(tileset)) {
        existingTilesetDocument->addMapDocument(mapDocument);//��tileset��ӵ��ĵ���
    } else {
        // Create TilesetDocument instance when it doesn't exist yet
        auto tilesetDocument = TilesetDocumentPtr::create(tileset);
        tilesetDocument->addMapDocument(mapDocument);

        mTilesetDocumentsModel->append(tilesetDocument.data());
        emit tilesetDocumentAdded(tilesetDocument.data());
    }
}

void DocumentManager::removeFromTilesetDocument(const SharedTileset &tileset, MapDocument *mapDocument)
{
    auto tilesetDocument = findTilesetDocument(tileset);
    auto tilesetDocumentPtr = tilesetDocument->sharedFromThis();    // keeps alive and may delete

    tilesetDocument->removeMapDocument(mapDocument);

    bool unused = tilesetDocument->mapDocuments().isEmpty();
    bool external = tilesetDocument->tileset()->isExternal();
    int index = findDocument(tilesetDocument);

    // Remove the TilesetDocument when its tileset is no longer reachable
    if (unused && !(index >= 0 && external)) {
        if (index != -1) {
            closeDocumentAt(index);
        } else {
            mTilesetDocumentsModel->remove(tilesetDocument);
            emit tilesetDocumentRemoved(tilesetDocument);
        }
    }
}

static bool mayNeedColumnCountAdjustment/*����*/(const Tileset &tileset)
{
    if (tileset.isCollection())
        return false;
    if (tileset.imageStatus() != LoadingReady)
        return false;
    if (tileset.columnCount() == tileset.expectedColumnCount())
        return false;
    if (tileset.columnCount() == 0 || tileset.expectedColumnCount() == 0)
        return false;
    if (tileset.expectedRowCount() < 2 || tileset.rowCount() < 2)
        return false;

    return true;
}

void DocumentManager::tilesetImagesChanged(Tileset *tileset)
{
    if (!mayNeedColumnCountAdjustment(*tileset))
        return;

    SharedTileset sharedTileset = tileset->sharedPointer();
    QList<Document*> affectedDocuments;

    for (const auto &document : mDocuments) {
        if (auto mapDocument = qobject_cast<MapDocument*>(document.data())) {
            if (mapDocument->map()->tilesets().contains(sharedTileset))
                affectedDocuments.append(document.data());
        }
    }

    if (TilesetDocument *tilesetDocument = findTilesetDocument(sharedTileset))
        affectedDocuments.append(tilesetDocument);

    if (!affectedDocuments.isEmpty() && askForAdjustment(*tileset)) {
        for (Document *document : qAsConst(affectedDocuments)) {
            if (auto mapDocument = qobject_cast<MapDocument*>(document)) {
                auto command = new AdjustTileIndexes(mapDocument, *tileset);
                document->undoStack()->push(command);
            } else if (auto tilesetDocument = qobject_cast<TilesetDocument*>(document)) {
                auto command = new AdjustTileMetaData(tilesetDocument);
                document->undoStack()->push(command);
            }
        }
    }

    tileset->syncExpectedColumnsAndRows();
}

/**
 * Checks whether the number of columns in tileset image based tilesets matches with the expected amount.
 *  Offers to adjust tile indexes if not.
 * ���tilesetͼ���� ����tilesets�������� �Ƿ���Ԥ�ڵ�������ƥ�䡣
 * ���û��ƥ�䣬����ͼ����tile������
 */
void DocumentManager::checkTilesetColumns(MapDocument *mapDocument)
{
    for (const SharedTileset &tileset : mapDocument->map()->tilesets()) {//�����Ѿ���ӵ����ŵ�ͼ��tilesets
        TilesetDocument *tilesetDocument = findTilesetDocument(tileset);
        Q_ASSERT(tilesetDocument);

        if (checkTilesetColumns(tilesetDocument)) {
            auto command = new AdjustTileIndexes(mapDocument, *tileset);
            mapDocument->undoStack()->push(command);
        }

        tileset->syncExpectedColumnsAndRows();
    }
}

bool DocumentManager::checkTilesetColumns(TilesetDocument *tilesetDocument)
{
    if (!mayNeedColumnCountAdjustment(*tilesetDocument->tileset()))
        return false;

    if (askForAdjustment(*tilesetDocument->tileset())) {
        auto command = new AdjustTileMetaData(tilesetDocument);
        tilesetDocument->undoStack()->push(command);
        return true;
    }

    return false;
}

bool DocumentManager::askForAdjustment(const Tileset &tileset)
{
    int r = QMessageBox::question(mWidget->window(),
                                  tr("Tileset Columns Changed"),
                                  tr("The number of tile columns in the tileset '%1' appears to have changed from %2 to %3. "
                                     "Do you want to adjust tile references?")
                                  .arg(tileset.name())
                                  .arg(tileset.expectedColumnCount())
                                  .arg(tileset.columnCount()),
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::Yes);

    return r == QMessageBox::Yes;
}

bool DocumentManager::eventFilter(QObject *object, QEvent *event)
{
    if (object == mTabBar && event->type() == QEvent::MouseButtonRelease) {
        // middle-click tab closing
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

        if (mouseEvent->button() == Qt::MidButton) {
            int index = mTabBar->tabAt(mouseEvent->pos());

            if (index != -1) {
                documentCloseRequested(index);
                return true;
            }
        }
    }

    return false;
}

/**
 * Unsets a flag to stop closeOtherDocuments() and closeDocumentsToRight()
 * when Cancel is pressed
 */
void DocumentManager::abortMultiDocumentClose()
{
    mMultiDocumentClose = false;
}
