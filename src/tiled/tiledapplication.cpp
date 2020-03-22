/*
 * tiledapplication.cpp
 * Copyright 2011, Thorbj酶rn Lindeijer <thorbjorn@lindeijer.nl>
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

#include "tiledapplication.h"

#include <QFileOpenEvent>
#include <QJsonArray>
#include <QJsonDocument>

using namespace Tiled;
using namespace Tiled::Internal;

TiledApplication::TiledApplication(int &argc, char **argv) :
    QtSingleApplication(argc, argv)
{
    connect(this, &TiledApplication::messageReceived,
            this, &TiledApplication::onMessageReceived);
}
//fileOpenRequest文件打开请求信号
bool TiledApplication::event(QEvent *event)
{
    if (event->type() == QEvent::FileOpen) {
        QFileOpenEvent *fileOpenEvent = static_cast<QFileOpenEvent*>(event);
        emit fileOpenRequest(fileOpenEvent->file());
        return true;
    }
    return QApplication::event(event);
}

void TiledApplication::onMessageReceived(const QString &message)
{
   const QJsonArray files = QJsonDocument::fromJson(message.toLatin1()/*这个字符串作为字节数组以Latin-1编码格式表现*/)/*转换为utf8json文档,并创建它*/.array();//装换为jsonarray
   for (const QJsonValue &file : files) {
       emit fileOpenRequest(file.toString());//main.cpp 434 line used
   }
}
//将JSON解析为UTF-8编码的JSON文档，并从中创建QJsonDocument。\
//如果解析成功，返回一个有效的（非空）QJsonDocument。如果它失败了，\
//返回的文档将是空的，可选的错误变量将包含关于错误的更多细节
//Returns the QJsonArray contained in the document.\
//Returns an empty array if the document contains an object.
