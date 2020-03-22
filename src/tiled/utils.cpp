/*
 * utils.cpp
 * Copyright 2009-2010, Thorbj酶rn Lindeijer <thorbjorn@lindeijer.nl>
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

#include "utils.h"

#include "preferences.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImageReader>
#include <QImageWriter>
#include <QKeyEvent>
#include <QMainWindow>
#include <QMenu>
#include <QProcess>
#include <QRegExp>
#include <QScreen>
#include <QSettings>

static QString toImageFileFilter(const QList<QByteArray> &formats)
{
    QString filter(QCoreApplication::translate("Utils", "Image files"));
    filter += QLatin1String(" (");
    bool first = true;
    for (const QByteArray &format : formats) {
        if (!first)
            filter += QLatin1Char(' ');
        first = false;
        filter += QLatin1String("*.");
        filter += QString::fromLatin1(format.toLower());
    }
    filter += QLatin1Char(')');
    return filter;
}

namespace Tiled {
namespace Utils {//实用工具 卷展栏 实用菜单

/**：返回一个与所有可读的图像格式匹配的文件过滤对话框。
 * Returns a file dialog filter that matches all readable image formats.
 */
QString readableImageFormatsFilter()
{
    return toImageFileFilter(QImageReader::supportedImageFormats());
}

/**
 * Returns a file dialog filter that matches all writable image formats.
 */
QString writableImageFormatsFilter()
{
    return toImageFileFilter(QImageWriter::supportedImageFormats());
}

// Makes a list of filters from a normal filter string "Image Files (*.png *.jpg)"
//
// Copied from qplatformdialoghelper.cpp in Qt, used under the terms of the GPL
// version 2.0.
QStringList cleanFilterList(const QString &filter)
{
    const char filterRegExp[] =
    "^(.*)\\(([a-zA-Z0-9_.,*? +;#\\-\\[\\]@\\{\\}/!<>\\$%&=^~:\\|]*)\\)$";

    QRegExp regexp(QString::fromLatin1(filterRegExp));
    Q_ASSERT(regexp.isValid());
    QString f = filter;
    int i = regexp.indexIn(f);
    if (i >= 0)
        f = regexp.cap(2);
    return f.split(QLatin1Char(' '), QString::SkipEmptyParts);
}

/**返回一个文件名是否有一个与nameFilter匹配的扩展名。
 * Returns whether the \a fileName has an extension that is matched by the \a nameFilter.
 */
bool fileNameMatchesNameFilter(const QString &fileName,
                               const QString &nameFilter)
{
    QRegExp rx;
    rx.setCaseSensitivity(Qt::CaseInsensitive);
    rx.setPatternSyntax(QRegExp::Wildcard);

    const QStringList filterList = cleanFilterList(nameFilter);
    for (const QString &filter : filterList) {
        rx.setPattern(filter);
        if (rx.exactMatch(fileName))
            return true;
    }
    return false;
}


/**
 * Restores a widget's geometry.恢复小部件的几何形状
 * Requires the widget to have its object name set.要求小部件拥有它的对象名称集。
 */
//几何图形恢复
void restoreGeometry(QWidget *widget)
{
    Q_ASSERT(!widget->objectName().isEmpty());

    const QSettings *settings = Internal::Preferences::instance()->settings();

    const QString key = widget->objectName() + QLatin1String("/Geometry");
    widget->restoreGeometry(settings->value(key).toByteArray());

    if (QMainWindow *mainWindow = qobject_cast<QMainWindow*>(widget)) {
        const QString stateKey = widget->objectName() + QLatin1String("/State");
        mainWindow->restoreState(settings->value(stateKey).toByteArray());
    }
}

/**
 * Saves a widget's geometry.
 * Requires the widget to have its object name set.
 */
void saveGeometry(QWidget *widget)
{
    Q_ASSERT(!widget->objectName().isEmpty());

    QSettings *settings = Internal::Preferences::instance()->settings();

    const QString key = widget->objectName() + QLatin1String("/Geometry");
    settings->setValue(key, widget->saveGeometry());

    if (QMainWindow *mainWindow = qobject_cast<QMainWindow*>(widget)) {
        const QString stateKey = widget->objectName() + QLatin1String("/State");
        settings->setValue(stateKey, mainWindow->saveState());
    }
}
//水平方向上每一寸的逻辑点数/像素个数
qreal defaultDpiScale()
{
    static qreal scale = []{
        if (const QScreen *screen = QGuiApplication::primaryScreen())
            return screen->logicalDotsPerInchX() / 96.0;//This property holds the number of logical dots or pixels per inch in the horizontal direction
                                                        //This value is used to convert font point sizes to pixel sizes.
        return 1.0;
    }();//https://blog.csdn.net/jasonLee_lijiaqi/article/details/80803005
    return scale;
}
//每英寸逻辑点数or pixel规模
qreal dpiScaled(qreal value)
{
#ifdef Q_OS_MAC
    // On mac the DPI is always 72 so we should not scale it
    return value;
#else
    static const qreal scale = defaultDpiScale();
    return value * scale;
#endif
}

QSize dpiScaled(QSize value)
{
    return QSize(qRound(dpiScaled(value.width())),
                 qRound(dpiScaled(value.height())));
}

QPoint dpiScaled(QPoint value)
{
    return QPoint(qRound(dpiScaled(value.x())),
                  qRound(dpiScaled(value.y())));
}
//dots
QRectF dpiScaled(QRectF value)
{
    return QRectF(dpiScaled(value.x()),//rect x coordinate value
                  dpiScaled(value.y()),
                  dpiScaled(value.width()),
                  dpiScaled(value.height()));
}

QSize smallIconSize()
{
    static QSize size = dpiScaled(QSize(16, 16));
    return size;
}

bool isZoomInShortcut(QKeyEvent *event)
{
    if (event->matches(QKeySequence::ZoomIn))
        return true;
    if (event->key() == Qt::Key_Plus)
        return true;
    if (event->key() == Qt::Key_Equal)
        return true;

    return false;
}

bool isZoomOutShortcut(QKeyEvent *event)
{
    if (event->matches(QKeySequence::ZoomOut))
        return true;
    if (event->key() == Qt::Key_Minus)
        return true;
    if (event->key() == Qt::Key_Underscore)
        return true;

    return false;
}

bool isResetZoomShortcut(QKeyEvent *event)
{
    if (event->key() == Qt::Key_0 && event->modifiers() & Qt::ControlModifier)
        return true;

    return false;
}

/*
 * Code based on FileUtils::showInGraphicalShell from Qt Creator
 * Copyright (C) 2016 The Qt Company Ltd.
 * Used under the terms of the GNU General Public License version 3
 */
static void showInFileManager(const QString &fileName)
{
    // Mac, Windows support folder or file.
#if defined(Q_OS_WIN)
    QStringList param;
    if (!QFileInfo(fileName).isDir())
        param += QLatin1String("/select,");
    param += QDir::toNativeSeparators(fileName);
    QProcess::startDetached(QLatin1String("explorer.exe"), param);
#elif defined(Q_OS_MAC)
    QStringList scriptArgs;
    scriptArgs << QLatin1String("-e")
               << QString::fromLatin1("tell application \"Finder\" to reveal POSIX file \"%1\"")
                                     .arg(fileName);
    QProcess::execute(QLatin1String("/usr/bin/osascript"), scriptArgs);
    scriptArgs.clear();
    scriptArgs << QLatin1String("-e")
               << QLatin1String("tell application \"Finder\" to activate");
    QProcess::execute(QLatin1String("/usr/bin/osascript"), scriptArgs);
#else
    // We cannot select a file here, because xdg-open would open the file
    // instead of the file browser...
    QProcess::startDetached(QString(QLatin1String("xdg-open \"%1\""))
                            .arg(QFileInfo(fileName).absolutePath()));
#endif
}

void addFileManagerActions(QMenu &menu, const QString &fileName)
{
    QAction *copyPath = menu.addAction(QCoreApplication::translate("Utils", "Copy File Path"));
    QObject::connect(copyPath, &QAction::triggered, [fileName] {
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(QDir::toNativeSeparators(fileName));
    });

    QAction *openFolder = menu.addAction(QCoreApplication::translate("Utils", "Open Containing Folder..."));
    QObject::connect(openFolder, &QAction::triggered, [fileName] {
        showInFileManager(fileName);
    });
}

} // namespace Utils
} // namespace Tiled
