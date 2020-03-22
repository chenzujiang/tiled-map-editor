/*
 * id.cpp
 * Copyright 2016, Thorbjørn Lindeijer <bjorn@lindeijer.nl>
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

/**
 * The following code is largely based on id.cpp from Qt Creator.
 */

#include "id.h"

#include <QHash>

namespace Tiled {
namespace Internal {

class StringHash
{
public:
    StringHash()
        : hash(0)
    {}

    explicit StringHash(const QByteArray &s)
        : string(s)
        , hash(qHash(s))//Returns the hash value for the key, using seed to seed the calculation. This function was introduced in Qt 5.0.
    {}

    QByteArray string;
    uint hash;
};

static bool operator==(const StringHash &sh1, const StringHash &sh2)
{
    return sh1.string == sh2.string;
}

static uint qHash(const StringHash &sh)
{
    return sh.hash;
}

static QHash<quintptr, StringHash> stringFromId;
static QHash<StringHash, quintptr> idFromString;


Id::Id(const char *name)
{
    static uint firstUnusedId = 1;

    static QByteArray temp;
    //��������QByteArray��ʹ����������ĵ�һ��size�ֽ�//ǰ���size���ַ�
    temp.setRawData(name, qstrlen(name));//avoid copying data���⸴������qstrlen"\0"֮ǰ�ĸ���

    StringHash sh(temp);
    //If the hash contains no item with the given key, the function returns defaultValue.
    int id = idFromString.value(sh, 0);//��ͨ������ַ���������hash�ܷ���id�����
                                       //û��,������ַ���������hash��һ��id

    if (id == 0) {
        id = firstUnusedId++;
        sh.string = QByteArray(temp.constData(), temp.length());    // copy
        idFromString.insert(sh, id);
        stringFromId.insert(id, sh);
    }

    mId = id;
}

QByteArray Id::name() const
{
    return stringFromId.value(mId).string;
}

} // namespace Internal
} // namespace Tiled
