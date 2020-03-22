/*
 * mapformat.h
 * Copyright 2008-2015, Thorbjørn Lindeijer <bjorn@lindeijer.nl>
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

#pragma once

#include "fileformat.h"
#include "pluginmanager.h"

#include <QObject>
#include <QStringList>
#include <QMap>

namespace Tiled {

class Map;

/**
 * An interface to be implemented for adding support for a map format to Tiled.
 * It can implement support for either loading or saving to a certain map
 * format, or both.
 */
class TILEDSHARED_EXPORT MapFormat : public FileFormat
{
    Q_OBJECT
    Q_INTERFACES(Tiled::FileFormat)//�õ���FileFormat�ӿ�read and write(����),�����ͷ�ļ��жԸýӿڽ���������

public:
    explicit MapFormat(QObject *parent = nullptr)
        : FileFormat(parent)
    {}
//https://blog.csdn.net/u010525694/article/details/72846701?locationNum=3&fps=1 //lamaba
    /**
     * Returns the absolute paths for the files that will be written by
     * this format for a given map.
     * ����д��ָ��ָ����ʽ��ͼ�ļ��ľ���·��
     * This is supported for Export formats only!
     */
    virtual QStringList outputFiles(const Map *, const QString &fileName) const
    { return QStringList(fileName); }


    /**
     * Reads the map and returns a new Map instance, or 0 if reading failed.
     */
    virtual Map *read(const QString &fileName) = 0;

    /**
     * Writes the given \a map based on the suggested \a fileName.
     *���ݽ�����ļ�����д�����ĵ�ͼ��
     * This function may write to a different file name or may even write to
     * multiple files. The actual files that will be written to can be
     * determined by calling outputFiles().
     *�����������д��ͬ���ļ�,����д����ļ�,��д���ʵ���ļ����Ե���outputFiles()��ȷ��
     * @return <code>true</code> on success, <code>false</code> when an error
     *         occurred. The error can be retrieved by errorString().
     */
    virtual bool write(const Map *map, const QString &fileName) = 0;
};

} // namespace Tiled

Q_DECLARE_INTERFACE(Tiled::MapFormat, "org.mapeditor.MapFormat")//�������ø���ӿ�,�ڴ������ӿ�
//Q_INTERFACES��Ҳ����qobject_cast��أ�û��Q_DECLARE_INTERFACE��Q_INTERFACES�������꣬
//���޷��ԴӲ���л�ȡ��ʵ��ָ�����qobject_castӳ�䡣
//������Q_INTERFACES�겢û����Qt��Դ���ж��壬����MOC�Ĳˣ�MOC���������������һЩ���롣Ҫע��һ�㣬
//���һ��ͷ�ļ���Դ�ļ����õ���Q_INTERFACES�꣬
//��ô�ڵ��������֮ǰ���������һ�� Q_DECLARE_INTERFACE��������Ӧ�Ľӿ�
//�����߰���һ����Q_DECLARE_INTERFACE�������˸ýӿڵ�ͷ�ļ�����MOC������һ�㣬��Ϊ����ΪQ_INTERFACES�����ɴ���ʱҪ�õ�Q_DECLARE_INTERFACE���IID����
//https://blog.csdn.net/NewThinker_wei/article/details/41292115
namespace Tiled {

/**
 * Convenience class for adding a format that can only be read.
 */
class TILEDSHARED_EXPORT ReadableMapFormat : public MapFormat
{
    Q_OBJECT
    Q_INTERFACES(Tiled::MapFormat)//�����õ��˻���������write�ӿ�

public:
    explicit ReadableMapFormat(QObject *parent = nullptr)
        : MapFormat(parent)
    {}
    Capabilities capabilities() const override { return Read; }
    bool write(const Map *, const QString &) override { return false; }
};


/**
 * Convenience class for adding a format that can only be written.
 */
class TILEDSHARED_EXPORT WritableMapFormat : public MapFormat
{
    Q_OBJECT
    Q_INTERFACES(Tiled::MapFormat)//�����õ��˻����read�ӿ�

public:
    explicit WritableMapFormat(QObject *parent = nullptr)
        : MapFormat(parent)
    {}

    Capabilities capabilities() const override { return Write; }
    Map *read(const QString &) override { return nullptr; }
    bool supportsFile(const QString &) const override { return false; }
};


/**�����Ŷ�ȡ�����ĵ�ͼ,ʹ���Ѿ���ӵ�����������еġ�ʧ�ܷ���TMX�����һ������
 * Attempt to read the given map using any of the map formats added
 * to the plugin manager, falling back to the TMX format if none are capable.
 */
TILEDSHARED_EXPORT Map *readMap(const QString &fileName,
                                QString *error = nullptr);

/**
 * Attempts to find a map format supporting the given file.
 */
TILEDSHARED_EXPORT MapFormat *findSupportingMapFormat(const QString &fileName);

} // namespace Tiled
