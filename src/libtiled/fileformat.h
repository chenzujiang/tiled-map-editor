/*
 * fileformat.h
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

#include "pluginmanager.h"

#include <QObject>
#include <QDebug>

namespace Tiled {


class TILEDSHARED_EXPORT FileFormat : public QObject
{
    Q_OBJECT

public:
    enum Capability {//����
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

    /**���ظø�ʽ�Ƿ�߱���ָ��������
     * Returns whether this format has all given capabilities.
     */
    bool hasCapabilities(Capabilities caps) const;

    /**�����ֵ�ͼ�ĸ�ʽ���ع��˺���ļ�����
     * Returns name filter for files in this map format.
     */
    virtual QString nameFilter() const = 0;

    /**Ϊ���ŵ�ͼ��ʽ���ض�����
     * Returns short name for this map format
     */
    virtual QString shortName() const = 0;

    /**
     * Returns whether this map format supports reading the given file.
     *���ŵ�ͼ�ĸ�ʽ�Ƿ�֧�ֶ�д�������ļ�
     * Generally would do a file extension check.ͨ����������ļ�����չ���
     */
    virtual bool supportsFile(const QString &fileName) const = 0;

    /**
     * Returns the error to be shown���� to the user if an error occured while
     * trying to read a map.
     * ����ڵ�ǰѭ���ĳ��Զ�ȡmapʱ��������Ļ�,�ú�������һ������Ľ��͡�
     */
    virtual QString errorString() const = 0;
};

} // namespace Tiled

Q_DECLARE_INTERFACE(Tiled::FileFormat, "org.mapeditor.FileFormat")//Ϊ�ӿ��ඨ��һ���ӿ�id

Q_DECLARE_OPERATORS_FOR_FLAGS(Tiled::FileFormat::Capabilities)

namespace Tiled {

/**����ʹ���ļ��Ի���ʵ����ʱ�����ʹ������������
 * Convenience class that can be used when implementing file dialogs.
 */
template<typename Format>
class FormatHelper
{
public:
    FormatHelper(FileFormat::Capabilities capabilities,
                 QString initialFilter = QString())
        : mFilter(std::move(initialFilter))//�ƶ����캯��
    {
        auto LambdaFun = [this,capabilities](Format *format)//fuction(result),��������е��õ�
        {
            if (format->hasCapabilities(capabilities)) {//�Ƿ��иö�д����
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

        //each<Format>ʵ����
//        PluginManager::each<Format>([this,capabilities](Format *format)//first call template each<format>,func bady take lambda pfunc call lambda bady faction
//        {//���������һ��lambda���ʽ����,����һ��û�з���ֵ�ĺ��� function<void(Format*)> pfun
//            if (format->hasCapabilities(capabilities)) {//�Ƿ��иö�д����
//                const QString nameFilter = format->nameFilter();//����֧������T map��ʽ�ĵ�����
//                qDebug()<<nameFilter;
//                if (!mFilter.isEmpty())
//                    mFilter += QLatin1String(";;");
//                mFilter += nameFilter;
//                qDebug()<<mFilter;

//                mFormats.append(format);
//                mFormatByNameFilter.insert(nameFilter, format);
//            }
//        });//PluginManager::each<Format>(pfun);ֱ���ú���
    }

    const QString &filter() const
    { return mFilter; }

    const QList<Format*> &formats() const
    { return mFormats; }

    Format *formatByNameFilter(const QString &nameFilter) const
    { return mFormatByNameFilter.value(nameFilter); }//ͨ�����õ�ֵ

private:
    QString mFilter;
    QList<Format*> mFormats;
    QMap<QString, Format*> mFormatByNameFilter;
};

} // namespace Tiled
