/*
 * tiledapplication.cpp
 * Copyright 2011, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
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
//fileOpenRequest�ļ��������ź�
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
   const QJsonArray files = QJsonDocument::fromJson(message.toLatin1()/*����ַ�����Ϊ�ֽ�������Latin-1�����ʽ����*/)/*ת��Ϊutf8json�ĵ�,��������*/.array();//װ��Ϊjsonarray
   for (const QJsonValue &file : files) {
       emit fileOpenRequest(file.toString());//main.cpp 434 line used
   }
}
//��JSON����ΪUTF-8�����JSON�ĵ��������д���QJsonDocument��\
//��������ɹ�������һ����Ч�ģ��ǿգ�QJsonDocument�������ʧ���ˣ�\
//���ص��ĵ����ǿյģ���ѡ�Ĵ���������������ڴ���ĸ���ϸ��
//Returns the QJsonArray contained in the document.\
//Returns an empty array if the document contains an object.
