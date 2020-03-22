/*
 * filesystemwatcher.cpp
 * Copyright 2011-2014, Thorbj酶rn Lindeijer <bjorn@lindeijer.nl>
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

#include "filesystemwatcher.h"

#include <QDebug>
#include <QFile>
#include <QFileSystemWatcher>
#include <QStringList>

namespace Tiled {

FileSystemWatcher::FileSystemWatcher(QObject *parent) :
    QObject(parent),
    mWatcher(new QFileSystemWatcher(this))
{
    connect(mWatcher, &QFileSystemWatcher::fileChanged,
            this, &FileSystemWatcher::onFileChanged);
    connect(mWatcher, &QFileSystemWatcher::directoryChanged,
            this, &FileSystemWatcher::onDirectoryChanged);
}

void FileSystemWatcher::addPath(const QString &path)
{
    // Just silently ignore the request when the file doesn't exist
    if (!QFile::exists(path))
        return;

    QMap<QString, int>::iterator entry = mWatchCount.find(path);
    if (entry == mWatchCount.end()) {
        mWatcher->addPath(path);//Adds path to the file system watcher if path exists. The path is not added if it does not exist, or if it is already being monitored by the file system watcher.
        mWatchCount.insert(path, 1);
    } else {//路径被监视,添加到查看器中
        // Path is already being watched, increment watch count
        ++entry.value();
    }
}

void FileSystemWatcher::removePath(const QString &path)
{
    QMap<QString, int>::iterator entry = mWatchCount.find(path);
    if (entry == mWatchCount.end()) {
        if (QFile::exists(path))
            qWarning() << "FileSystemWatcher: Path was never added:" << path;
        return;
    }

    // Decrement watch count逐渐减少查看的数量
    --entry.value();

    if (entry.value() == 0) {
        mWatchCount.erase(entry);//删除map中的iterator pos指向的（键、值）对，并将迭代器返回到地图中的下一个项目。
        mWatcher->removePath(path);
    }
}

void FileSystemWatcher::onFileChanged(const QString &path)
{
    // If the file was replaced, the watcher is automatically removed and needs
    // to be re-added to keep watching it for changes. This happens commonly
    // with applications that do atomic saving.
    // 如果文件被替代,查看器将制动移除并需要从新添加并更新它的改变,通常发生这样的话需要原子保持
    if (!mWatcher->files().contains(path))//返回被监视的文件路径列表
        if (QFile::exists(path))
            mWatcher->addPath(path);

    emit fileChanged(path);
}

void FileSystemWatcher::onDirectoryChanged(const QString &path)
{
    emit directoryChanged(path);
}

} // namespace Tiled
