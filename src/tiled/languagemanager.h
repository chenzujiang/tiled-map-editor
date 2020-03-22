/*
 * languagemanager.h
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

#pragma once

#include <QStringList>

class QTranslator;

namespace Tiled {
namespace Internal {

class LanguageManager
{
public:
    static LanguageManager *instance();
    static void deleteInstance();

    /**在应用程序中给Qt和Tiled安装翻译,当语言发生改变成被在次调用。
     * Installs the translators on the application for Qt and Tiled. Should be
     * called again when the language changes.
     */
    void installTranslators();

    /**将可用的语言作为国家代码以(列表)返回
     * Returns the available languages as a list of country codes.
     */
    QStringList availableLanguages();

private:
    LanguageManager();
    ~LanguageManager();

    void loadAvailableLanguages();

    QString mTranslationsDir;
    QStringList mLanguages;
    QTranslator *mQtTranslator;
    QTranslator *mAppTranslator;

    static LanguageManager *mInstance;
};

} // namespace Internal
} // namespace Tiled
