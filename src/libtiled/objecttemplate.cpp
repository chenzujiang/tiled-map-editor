/*
 * objecttemplate.cpp
 * Copyright 2017, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyright 2017, Mohamed Thabet <thabetx@gmail.com>
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

#include "objecttemplate.h"

#include "objecttemplateformat.h"

namespace Tiled {

ObjectTemplate::ObjectTemplate()
    : ObjectTemplate(QString())
{
}

ObjectTemplate::ObjectTemplate(const QString &fileName)
    : Object(ObjectTemplateType)
    , mFileName(fileName)
    , mObject(nullptr)
{
}

ObjectTemplate::~ObjectTemplate()
{
    setObject(nullptr);
}

void ObjectTemplate::setObject(const MapObject *object)
{
    MapObject *oldObject = mObject;
    Tileset *tileset = nullptr;

    if (object) {
        tileset = object->cell().tileset();
        mObject = object->clone();
        mObject->markAsTemplateBase();
    } else {
        mObject = nullptr;
    }

    if (oldObject)
        delete oldObject;

    if (tileset)
        mTileset = tileset->sharedPointer();
    else
        mTileset.reset();
}

void ObjectTemplate::setFormat(ObjectTemplateFormat *format)
{
    mFormat = format;
}

ObjectTemplateFormat *ObjectTemplate::format() const
{
    return mFormat;
}

} // namespace Tiled
