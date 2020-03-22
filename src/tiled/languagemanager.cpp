/*
 * languagemanager.cpp
 * Copyright 2009, Thorbj酶rn Lindeijer <thorbjorn@lindeijer.nl>
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

#include "languagemanager.h"

#include "preferences.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>
#include <QDebug>
using namespace Tiled::Internal;

LanguageManager *LanguageManager::mInstance;

LanguageManager *LanguageManager::instance()
{
    if (!mInstance)
        mInstance = new LanguageManager;
    return mInstance;
}

void LanguageManager::deleteInstance()
{
    delete mInstance;
    mInstance = nullptr;
}

LanguageManager::LanguageManager()
    : mQtTranslator(nullptr)
    , mAppTranslator(nullptr)
{
    mTranslationsDir = QCoreApplication::applicationDirPath();
#if defined(Q_OS_WIN32)
    mTranslationsDir += QLatin1String("/translations");
#elif defined(Q_OS_MAC)
    mTranslationsDir += QLatin1String("/../Translations");
#else
    mTranslationsDir += QLatin1String("/../share/tiled/translations");
#endif
}

LanguageManager::~LanguageManager()
{
    delete mQtTranslator;
    delete mAppTranslator;
}
//安装翻译,语言改变之后会调用这个函数
void LanguageManager::installTranslators()
{
    // Delete previous translators
    delete mQtTranslator;
    delete mAppTranslator;

    mQtTranslator = new QTranslator;
    mAppTranslator = new QTranslator;

    QString language = Preferences::instance()->language();
    if (language.isEmpty())
        language = QLocale::system().name();//以"language_country"形式的字符串返回该地区的语言和国家，其中语言为小写、两个字母的ISO 639语言代码，国家为大写、两个或三个字母的ISO 3166国家代码。

    const QString qtTranslationsDir =
            QLibraryInfo::location(QLibraryInfo::TranslationsPath);//Qt字符串翻译信息的位置。
    qDebug()<<qtTranslationsDir<<language;
    //利用当选择文件之后,先看看选择的qt_language qt是否能够加载成安装mQtTranslator翻译,否则delete mQtTranslator
    if (mQtTranslator->load(QLatin1String("qt_") + language,
                            qtTranslationsDir)) {
        QCoreApplication::installTranslator(mQtTranslator);//如果相关的翻译文件,安装相应的翻译
    } else {
        delete mQtTranslator;
        mQtTranslator = nullptr;
    }
    //选择的引用程序也能有相应的文件就再次安装
    if (mAppTranslator->load(QLatin1String("tiled_") + language,
                             mTranslationsDir)) {
        QCoreApplication::installTranslator(mAppTranslator);
    } else {
        delete mAppTranslator;
        mAppTranslator = nullptr;
    }
}

QStringList LanguageManager::availableLanguages()
{
    if (mLanguages.isEmpty())
        loadAvailableLanguages();
    return mLanguages;
}
//加载有效的语言
void LanguageManager::loadAvailableLanguages()
{
    mLanguages.clear();

    QStringList nameFilters;
    nameFilters.append(QLatin1String("tiled_*.qm"));

    QDirIterator iterator(mTranslationsDir, nameFilters,
                          QDir::Files | QDir::Readable);

    while (iterator.hasNext()) {
        iterator.next();
        const QString baseName = iterator.fileInfo().completeBaseName();//返回没有路径的文件的完整基名称
        // Cut off "tiled_" from the start
        mLanguages.append(baseName.mid(6));
    }
}
