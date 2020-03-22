/*
 * fileformat.h
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

#include "pluginmanager.h"

#include <QObject>
#include <QDebug>

namespace Tiled {


class TILEDSHARED_EXPORT FileFormat : public QObject
{
    Q_OBJECT

public:
    enum Capability {//性能
        NoCapability    = 0x0,
        Read            = 0x1,
        Write           = 0x2,
        ReadWrite       = Read | Write
    };
    Q_DECLARE_FLAGS(Capabilities, Capability)

    explicit FileFormat(QObject *parent = nullptr);

    /**
     * Returns whether this format has Read and/or Write capabilities.
     */
    virtual Capabilities capabilities() const;

    /**返回该格式是否具备所指定的性能
     * Returns whether this format has all given capabilities.
     */
    bool hasCapabilities(Capabilities caps) const;

    /**以这种地图的格式返回过滤后的文件名字
     * Returns name filter for files in this map format.
     */
    virtual QString nameFilter() const = 0;

    /**为这张地图格式返回短名称
     * Returns short name for this map format
     */
    virtual QString shortName() const = 0;

    /**
     * Returns whether this map format supports reading the given file.
     *这张地图的格式是否支持读写给定的文件
     * Generally would do a file extension check.通常会做这个文件的扩展检测
     */
    virtual bool supportsFile(const QString &fileName) const = 0;

    /**
     * Returns the error to be shown解释 to the user if an error occured while
     * trying to read a map.
     * 如果在当前循环的尝试读取map时发生错误的话,该函数返回一个错误的解释。
     */
    virtual QString errorString() const = 0;
};

} // namespace Tiled

Q_DECLARE_INTERFACE(Tiled::FileFormat, "org.mapeditor.FileFormat")//为接口类定义一个接口id

Q_DECLARE_OPERATORS_FOR_FLAGS(Tiled::FileFormat::Capabilities)

namespace Tiled {

/**当在使用文件对话框实例的时候可以使用这个方便的类
 * Convenience class that can be used when implementing file dialogs.
 */
template<typename Format>
class FormatHelper
{
public:
    FormatHelper(FileFormat::Capabilities capabilities,
                 QString initialFilter = QString())
        : mFilter(std::move(initialFilter))//移动构造函数
    {
        auto LambdaFun = [this,capabilities](Format *format)//fuction(result),差价类体中调用的
        {
            if (format->hasCapabilities(capabilities)) {//是否有该读写特性
                const QString nameFilter = format->nameFilter();//NULL,Returns name filter for files in this map format.
                qDebug()<<nameFilter;
                if (!mFilter.isEmpty())
                    mFilter += QLatin1String(";;");
                mFilter += nameFilter;
                qDebug()<<mFilter;

                mFormats.append(format);
                mFormatByNameFilter.insert(nameFilter, format);
            }
        };
        PluginManager::each<Format>(LambdaFun);

        //each<Format>实例化
//        PluginManager::each<Format>([this,capabilities](Format *format)//first call template each<format>,func bady take lambda pfunc call lambda bady faction
//        {//这里面就是一个lambda表达式函数,就是一个没有返回值的函数 function<void(Format*)> pfun
//            if (format->hasCapabilities(capabilities)) {//是否有该读写特性
//                const QString nameFilter = format->nameFilter();//返回支持这种T map格式的的名称
//                qDebug()<<nameFilter;
//                if (!mFilter.isEmpty())
//                    mFilter += QLatin1String(";;");
//                mFilter += nameFilter;
//                qDebug()<<mFilter;

//                mFormats.append(format);
//                mFormatByNameFilter.insert(nameFilter, format);
//            }
//        });//PluginManager::each<Format>(pfun);直接用函数
    }

    const QString &filter() const
    { return mFilter; }

    const QList<Format*> &formats() const
    { return mFormats; }

    Format *formatByNameFilter(const QString &nameFilter) const
    { return mFormatByNameFilter.value(nameFilter); }//通过键得到值

private:
    QString mFilter;
    QList<Format*> mFormats;
    QMap<QString, Format*> mFormatByNameFilter;
};

} // namespace Tiled
