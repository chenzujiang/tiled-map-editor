/*
 * mapformat.h
 * Copyright 2008-2015, Thorbj酶rn Lindeijer <bjorn@lindeijer.nl>
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
    Q_INTERFACES(Tiled::FileFormat)//用到了FileFormat接口read and write(函数),在类的头文件中对该接口进行了声明

public:
    explicit MapFormat(QObject *parent = nullptr)
        : FileFormat(parent)
    {}
//https://blog.csdn.net/u010525694/article/details/72846701?locationNum=3&fps=1 //lamaba
    /**
     * Returns the absolute paths for the files that will be written by
     * this format for a given map.
     * 将被写入指定指定格式地图文件的绝对路径
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
     *根据建议的文件名编写给定的地图。
     * This function may write to a different file name or may even write to
     * multiple files. The actual files that will be written to can be
     * determined by calling outputFiles().
     *这个函数可以写不同的文件,或者写多个文件,被写入的实际文件可以调用outputFiles()类确定
     * @return <code>true</code> on success, <code>false</code> when an error
     *         occurred. The error can be retrieved by errorString().
     */
    virtual bool write(const Map *map, const QString &fileName) = 0;
};

} // namespace Tiled

Q_DECLARE_INTERFACE(Tiled::MapFormat, "org.mapeditor.MapFormat")//子类中用该类接口,在此声明接口
//Q_INTERFACES宏也是与qobject_cast相关，没有Q_DECLARE_INTERFACE和Q_INTERFACES这两个宏，
//就无法对从插件中获取的实例指针进行qobject_cast映射。
//不过，Q_INTERFACES宏并没有在Qt的源码中定义，他是MOC的菜，MOC会利用这个宏生成一些代码。要注意一点，
//如果一个头文件或源文件中用到了Q_INTERFACES宏，
//那么在调用这个宏之前，必须存在一个 Q_DECLARE_INTERFACE宏声明相应的接口
//（或者包含一个用Q_DECLARE_INTERFACE宏声明了该接口的头文件），MOC会检查这一点，因为它在为Q_INTERFACES宏生成代码时要用到Q_DECLARE_INTERFACE宏的IID参数
//https://blog.csdn.net/NewThinker_wei/article/details/41292115
namespace Tiled {

/**
 * Convenience class for adding a format that can only be read.
 */
class TILEDSHARED_EXPORT ReadableMapFormat : public MapFormat
{
    Q_OBJECT
    Q_INTERFACES(Tiled::MapFormat)//这里用到了基类声明的write接口

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
    Q_INTERFACES(Tiled::MapFormat)//这里用到了基类的read接口

public:
    explicit WritableMapFormat(QObject *parent = nullptr)
        : MapFormat(parent)
    {}

    Capabilities capabilities() const override { return Write; }
    Map *read(const QString &) override { return nullptr; }
    bool supportsFile(const QString &) const override { return false; }
};


/**尝试着读取给定的地图,使用已经添加到插件管理器中的。失败返回TMX如果有一个能力
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
