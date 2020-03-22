/*
 * documentmanager.cpp
 * Copyright 2010, Stefan Beller <stefanbeller@googlemail.com>
 * Copyright 2010-2016, Thorbj酶rn Lindeijer <thorbjorn@lindeijer.nl>
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
    , mFileChangedWarning(new FileChangedWarning(mWidget))/*reload and ignore(开始会有)*/
    , mBrokenLinksModel(new BrokenLinksModel(this))
    , mBrokenLinksWidget(new BrokenLinksWidget(mBrokenLinksModel, mWidget))
    , mMapEditor(nullptr) // todo备忘录/代办的事: look into removing this考虑移除这个
    , mUndoGroup(new QUndoGroup(this))
    , mFileSystemWatcher(new FileSystemWatcher(this))
    , mMultiDocumentClose(false)
{
    mBrokenLinksWidget->setVisible(true);/*中间的*/

    mTabBar->setExpanding(false);//When expanding(扩展) is true QTabBar will expand the tabs to use the empty space.
    mTabBar->setDocumentMode(true);//这个属性包含标签栏是否以适合主窗口的模式呈现。
    mTabBar->setTabsClosable(true);//这个属性包含一个标签栏是否应该在每个选项卡上放置关闭按钮
    mTabBar->setMovable(true);//This property holds whether the user can move the tabs within the tabbar area.(拖动)
    mTabBar->setContextMenuPolicy(Qt::CustomContextMenu);//how the widget shows a context menu

    mFileChangedWarning->setVisible(true);

    connect(mFileChangedWarning, &FileChangedWarning::reload, this, &DocumentManager::reloadCurrentDocument);
    connect(mFileChangedWarning, &FileChangedWarning::ignore, this, &DocumentManager::hideChangedWarning);

    QVBoxLayout *vertical = new QVBoxLayout(mWidget);//mWidget垂直布局中父控件,(tabbar,FileChangeWarning,Broken,mEditorStack[!3]),
    vertical->addWidget(mTabBar);//tabBar
    vertical->addWidget(mFileChangedWarning);//FileChangedWarning
    vertical->addWidget(mBrokenLinksWidget);//next ui layout
    vertical->setMargin(0);
    vertical->setSpacing(0);
    //在布局中添加了一个stackedlayout,存放的第一个就是试图中看到的新建地图(该控件是designer托的)。栈布局很重要
    mEditorStack = new QStackedLayout;
    mEditorStack->addWidget(mNoEditorWidget);//mEditorStack栈装的就三个对象mNoEditorWidget,mapEidtor(有stack),TilesetEidtor(有stack)[1]
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

    mTabBar->installEventFilter(this);//在该对象上安装一个事件过滤器filterObj。
}

DocumentManager::~DocumentManager()
{
    mTabBar->removeEventFilter(this);

    // All documents should be closed gracefully beforehand所有的文档都应该在预先关闭
    Q_ASSERT(mDocuments.isEmpty());
    Q_ASSERT(mTilesetDocumentsModel->rowCount() == 0);
    delete mWidget;
}

/**
 * Returns the document manager widget. It contains the different map views
 * and a tab bar to switch between them.
 * 返回一个 document manager 控件，它包含了不同的地图视图,一个tab bar在他们(地图)之间进行切换
 */
QWidget *DocumentManager::widget() const
{
    return mWidget;
}

void DocumentManager::setEditor(Document::DocumentType documentType, Editor *editor)
{
    Q_ASSERT(!mEditorForType.contains(documentType));
    mEditorForType.insert(documentType, editor);
    mEditorStack->addWidget(editor->editorWidget());//mEditorStack栈装的就三个对象no,mapEidtor(有stack),TilesetEidtor(有stack)[2,3]

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
//恢复/还原状态
void DocumentManager::restoreState()
{
    QHashIterator<Document::DocumentType, Editor*> iterator(mEditorForType);
    while (iterator.hasNext())
        iterator.next().value()->restoreState();
}

/**
 * Returns the current map document, or 0 when there is none.返回一个当前页面文档给调用者
 */
Document *DocumentManager::currentDocument() const
{
    const int index = mTabBar->currentIndex();
    if (index == -1)
        return nullptr;
    //Vector<QSharedPointer<Document>> a.at(index).data()返回共享指针指定对象的值 //Returns the value of the pointer referenced by this object.
    return mDocuments.at(index).data();//Document*//data()返回该对象引用的指针的值
}

/**
 * Returns the map view of the current document, or 0 when there is none.
 */
MapView *DocumentManager::currentMapView() const
{
    return mMapEditor->currentMapView();
}

/**返回显示指定文档的地图,没有返回null
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
    const QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();//文件的绝对路径
    if (canonicalFilePath.isEmpty()) // file doesn't exist
        return -1;
    //从文档管理者中找到需要打开的文档，并返回敢当的索引(启动之后已经添加到导航栏中了的？？？2018-10-1)
    for (int i = 0; i < mDocuments.size(); ++i) {
        QFileInfo fileInfo(mDocuments.at(i)->fileName());
        if (fileInfo.canonicalFilePath() == canonicalFilePath)//当文件管理者中的文件和参数文件的路径都一样
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
            addToTilesetDocument(tileset, mapDocument);//将mapDocument中map()中的tilesets添加到mapDocument中
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
    if (Editor *editor = mEditorForType.value(document->type()))//通过文档类型获取编辑者(Editor有MapEdirtor和TilesetEditor)
        editor->addDocument(document.data());//将种类型的文件添加到编辑器中

    QString tabText = document->displayName();
    if (document->isModified())//文档有修改显示*
        tabText.prepend(QLatin1Char('*'));

    const int documentIndex = mTabBar->addTab(tabText);//Adds a new tab with text text. Returns the new tab's index.
    mTabBar->setTabToolTip(documentIndex, document->fileName());//设置tab的index位置工具提示为tip(文件名),鼠标放上上去的提示文件名
//core
    connect(document.data(), &Document::fileNameChanged, this, &DocumentManager::fileNameChanged);
    connect(document.data(), &Document::modifiedChanged, this, &DocumentManager::modifiedChanged);
    connect(document.data(), &Document::saved, this, &DocumentManager::documentSaved);//发送信号的时候还没有链接调用

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

/**返回给定文档是否有未保存的修改。对于嵌入tileset的地图文件，这包括检查任何嵌入的tileset是否有未保存的修改。
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

    return document->isModified();//如果stack isclean state 就没有修改
}

/**返回是否在磁盘上更改了给定的文档。考虑到给定文档是嵌入的tileset文档的情况。
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

    // Try to find it in otherwise referenced documents试着在其他引用的文档中找到它
    {
        QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();//获取文件的标准路径
        for (Document *doc : Document::documentInstances()) {
            if (QFileInfo(doc->fileName()).canonicalFilePath() == canonicalFilePath)
                return doc->sharedFromThis();
        }//If this (that is, the subclass instance invoking this method) is being managed by a QSharedPointer,
        //returns a shared pointer instance pointing to this; otherwise returns a QSharedPointer holding a null pointer.返回指向此document共享指针对象;否则返回一个包含空指针的QSharedPointer。
    }
    //mutable 修饰变量为易变的,及时在const函数里面。一般某个成员函数不改变对象属性就声明为count函数
    if (!fileFormat) {
        // Try to find a plugin that implements support for this format从插件管理者中找到这个实现(支持该格式的)
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
            *error = tr("Unrecognized file format.");//无法识别的文件格式
        return DocumentPtr();
    }

    DocumentPtr document;

    if (MapFormat *mapFormat = qobject_cast<MapFormat*>(fileFormat)) {
        document = MapDocument::load(fileName, mapFormat, error);
    } else if (TilesetFormat *tilesetFormat = qobject_cast<TilesetFormat*>(fileFormat)) {
        // It could be, that we have already loaded this tileset while loading some map.有可能，我们在加载地图时已经加载了这个tileset。
        if (auto tilesetDocument = findTilesetDocument(fileName)) {
            document = tilesetDocument->sharedFromThis();
        } else {
            document = TilesetDocument::load(fileName, tilesetFormat, error);
        }
    }

    return document;
}

/**保存给定名字的给定文档,如果保存成功,将文件添加到最近的文件链表中
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

/**(保存一个文档由用户所设置(选择)的文件名的文档);;保存一个由用户自定义文件名的文档
 * Save the given document with a file name chosen by the user.
 * 保存成功,文件添加到最近文件链表中
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
        selectedFilter = format->nameFilter();//.tmx,.xml返回该类型的地图所支持的文件格式
    //declaration funcation pointer( = lambda 表达式)
    auto getSaveFileName = [&,this](const QString &defaultFileName) {
        if (fileName.isEmpty()) {
            fileName = Preferences::instance()->fileDialogStartLocation();//最近打开那个文件所在的路径
            fileName += QLatin1Char('/');
            fileName += defaultFileName;
        }

        while (true) {
            fileName = QFileDialog::getSaveFileName(mWidget->window(),
                                                    QString(),
                                                    fileName,
                                                    filter,
                                                    &selectedFilter);

            if (!fileName.isEmpty() &&//是否有与该扩展名匹配的名称文件
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
        /*MapFormat为类型构造出来的对象Helper,这个对象有如下值得关注的知识
         * 1 QList<MapFormat*> mFormats这个类存储了基类的所有子对象
         * 2 QMap<QString, MapFormat*> mFormatByNameFilter;上面链表中的对象都要一个键(该对象所支持的所有文件格式)与它(object)对应
         */
        FormatHelper<MapFormat> helper(FileFormat::ReadWrite);
        filter = helper.filter();//返回支持该MapFormat对象的所有文件格式(插件链表中遍历了所以格式对象)
        qDebug()<<filter ;
        auto suggestedFileName = QCoreApplication::translate("Tiled::Internal::MainWindow", "untitled");
        suggestedFileName.append(QLatin1String(".tmx"));

        fileName = getSaveFileName(suggestedFileName);
        if (fileName.isEmpty())
            return false;

        MapFormat *format = helper.formatByNameFilter(selectedFilter);//通过该对象格式所支持的格式字符串获得格式对象
        mapDocument->setWriterFormat(format);//为该文档对象设置（格式对象
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

/**关闭所有文档，除了索引所指向的文档。
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
 * @brief         移动Tab中的文件index到某个index
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
        existingTilesetDocument->addMapDocument(mapDocument);//将tileset添加到文档中
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

static bool mayNeedColumnCountAdjustment/*调整*/(const Tileset &tileset)
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
 * 检查tileset图像中 基于tilesets的列数量 是否与预期的数量相匹配。
 * 如果没有匹配，则试图调整tile索引。
 */
void DocumentManager::checkTilesetColumns(MapDocument *mapDocument)
{
    for (const SharedTileset &tileset : mapDocument->map()->tilesets()) {//返回已经添加到这张地图的tilesets
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
