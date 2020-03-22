/*
 * main.cpp
 * Copyright 2008-2011, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyright 2011, Ben Longbons <b.r.longbons@gmail.com>
 * Copyright 2011, Stefan Beller <stefanbeller@googlemail.com>
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

#include "commandlineparser.h"
#include "languagemanager.h"
#include "mainwindow.h"
#include "mapdocument.h"
#include "mapformat.h"
#include "mapreader.h"
#include "pluginmanager.h"
#include "preferences.h"
#include "sparkleautoupdater.h"
#include "standardautoupdater.h"
#include "stylehelper.h"
#include "tiledapplication.h"
#include "tileset.h"
#include "tmxmapformat.h"
#include "winsparkleautoupdater.h"

#include <QDebug>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QtPlugin>

#include <memory>

#include <QDebug>
#include <QStringList>
#include <QSettings>
#ifdef Q_OS_WIN
#include <windows.h>
#if QT_VERSION >= 0x050700
#include <QtPlatformHeaders\QWindowsWindowFunctions>
#endif // QT_VERSION >= 0x050700
#endif // Q_OS_WIN
#define STRINGIFY(x) #x//siringify
#define AS_STRING(x) STRINGIFY(x)

using namespace Tiled;
using namespace Tiled::Internal;

namespace {

class CommandLineHandler : public CommandLineParser//�����з�����
{
    Q_DECLARE_TR_FUNCTIONS(CommandLineHandler)//����Qt����ӷ���֧��
public:
    CommandLineHandler();

    bool quit;
    bool showedVersion;
    bool disableOpenGL;
    bool exportMap;
    bool exportTileset;
    bool newInstance;

private:
    void showVersion();
    void justQuit();
    void setDisableOpenGL();
    void setExportMap();
    void setExportTileset();
    void showExportFormats();
    void startNewInstance();

    // Convenience wrapper around registerOption �����ڸ�����װע�����
    template <void (CommandLineHandler::* memberFunction)()>
    void option(QChar shortName,
                const QString &longName,
                const QString &help)
    {
        registerOption<CommandLineHandler, memberFunction>(this,
                                                           shortName,
                                                           longName,
                                                           help);
    }
};

/**��ʹ���ļ�������ʱ�򣬳��� �¶����Ĵ������в���������ļ��ĸ�ʽ
 * Used during file export, attempt to determine the output file format
 * from the command line parameters.
 * Query errorMsg if result is null.��ѯ errorMsg �������NULL
 */
template <typename T>
inline T *findExportFormat(const QString *filter,
                           const QString &targetFile,
                           QString &errorMsg)
{
    T *outputFormat = nullptr;
    const auto formats = PluginManager::objects<T>();// auto==QList<T*>
    if (filter) {//������������˸���ʽ[foormat]��������,->elseͨ���ļ���ȥƥ��
        // Find the format supporting the given filter����֧�ָ���ɸѡ���ĸ�ʽ
        for (T *format : formats) {
            if (!format->hasCapabilities(T::Write))
                continue;
            if (format->shortName().compare(*filter, Qt::CaseInsensitive) == 0) {
                outputFormat = format;
                break;
            }
        }

        if (!outputFormat) {
            errorMsg = QCoreApplication::translate("Command line", "Format not recognized (see --export-formats)");
            return nullptr;
        }
    }
    else {//�����и��ĵ�����ʽ,�Ҳ�����ʽ����ʱ,������û�и���ʽ
        // Find the format based on target file extension�����ļ�����չ�������ļ�
        QString suffix = QFileInfo(targetFile).completeSuffix();//�������ļ���չ���ɵ�һ��.֮����ļ����Ƶ����е��ַ�����
        for (T *format : formats) {
            if (!format->hasCapabilities(T::Write))
                continue;
            qDebug()<<"suffix"<<  suffix;
            //�ļ���ʽָ�ļ���׺.tms,.xml
            if (format->nameFilter().contains(suffix, Qt::CaseInsensitive)) {
                if (outputFormat) {
                    errorMsg = QCoreApplication::translate("Command line", "Non-unique file extension. Can't determine correct export format.");
                    return nullptr;
                }
                outputFormat = format;
            }
        }
        if (!outputFormat) {
            errorMsg = QCoreApplication::translate("Command line", "No exporter found for target file.");
            return nullptr;
        }
    }

    return outputFormat;//���ص�����ʽ����(���,eg:json)
}


} // anonymous namespace�������ֿռ�


CommandLineHandler::CommandLineHandler()
    : quit(false)
    , showedVersion(false)
    , disableOpenGL(false)
    , exportMap(false)
    , exportTileset(false)
    , newInstance(false)
{
    option<&CommandLineHandler::showVersion>(//CommandLineHandler::showVersion����
                QLatin1Char('v'),
                QLatin1String("--version"),
                tr("Display the version"));

    option<&CommandLineHandler::justQuit>(
                QChar(),
                QLatin1String("--quit"),
                tr("Only check validity of arguments"));

    option<&CommandLineHandler::setDisableOpenGL>(
                QChar(),
                QLatin1String("--disable-opengl"),
                tr("Disable hardware accelerated rendering"));

    option<&CommandLineHandler::setExportMap>(
                QChar(),
                QLatin1String("--export-map"),
                tr("Export the specified map file to target"));

    option<&CommandLineHandler::setExportTileset>(
                QChar(),
                QLatin1String("--export-tileset"),
                tr("Export the specified tileset file to target"));

    option<&CommandLineHandler::showExportFormats>(
                QChar(),
                QLatin1String("--export-formats"),
                tr("Print a list of supported export formats"));

    option<&CommandLineHandler::startNewInstance>(
                QChar(),
                QLatin1String("--new-instance"),
                tr("Start a new instance, even if an instance is already running"));
}

void CommandLineHandler::showVersion()
{
    if (!showedVersion) {
        showedVersion = true;
        qWarning().noquote() << QApplication::applicationDisplayName()
                             << QApplication::applicationVersion();
        quit = true;
    }
}
//��Ҫ(����)�˳�
void CommandLineHandler::justQuit()
{
    quit = true;
}

void CommandLineHandler::setDisableOpenGL()
{
    disableOpenGL = true;//openGL רҵ��ͼ�γ���ӿ�,�������Ժ�ƽ̨
}
//���õ�����ͼ
void CommandLineHandler::setExportMap()
{
    exportMap = true;
}
//���õ���
void CommandLineHandler::setExportTileset()
{
    exportTileset = true;
}
//��ʾ֧�ֵ����ĸ�ʽ�ļ���ʽ ����ש��map���ָ�ʽ���ж������ƣ�
void CommandLineHandler::showExportFormats()
{
    PluginManager::instance()->loadPlugins();

    QStringList formats;
    for (MapFormat *format : PluginManager::objects<MapFormat>()) {
        if (format->hasCapabilities(MapFormat::Write))
            formats.append(format->shortName());//��������T���͵���д�����Ķ���,��ʾ���ǵ�����,����֧�ֵĵ�����ʽ
    }
    formats.sort(Qt::CaseSensitive);
    qWarning().noquote() << tr("Map export formats:");
    for (const QString &name : formats)
        qWarning(" %s", qUtf8Printable(name));

    formats.clear();
/////////////////////////////////
    for (TilesetFormat *format : PluginManager::objects<TilesetFormat>()) {
        if (format->hasCapabilities(TilesetFormat::Write))
            formats.append(format->shortName());
    }
    formats.sort(Qt::CaseSensitive);
    qWarning().noquote() << tr("Tileset export formats:");
    for (const QString &name : formats)
        qWarning(" %s", qUtf8Printable(name));

    quit = true;
}

void CommandLineHandler::startNewInstance()
{
    newInstance = true;
}


int main(int argc, char *argv[])
{
#if defined(Q_OS_WIN) && (!defined(Q_CC_MINGW) || __MINGW32_MAJOR_VERSION >= 5)
    // Make console output work on Windows, if running in a console.
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {//�����ý��̸��ӵ�ָ�����̵Ŀ���̨��((unsigned long)-1 )-->ʹ�õ�ǰ���̵ĸ����ڵĿ���̨
        FILE *dummy = nullptr;                 //https://cloud.tencent.com/developer/section/1009141
        freopen_s(&dummy, "CONOUT$", "w", stdout);//����׼����ʹ���������򵽿���̨
        freopen_s(&dummy, "CONOUT$", "w", stderr);
    }
#endif

#if QT_VERSION >= 0x050600//Sets whether QGuiApplication will use fallback session management to enabled.
    QGuiApplication::setFallbackSessionManagementEnabled(false);//�Ƿ�ʹ�ñ��õ�������(����)�ػ�����(ʹ��)
#endif

    // Enable support for highres images (added in Qt 5.1, but off by default)ʹ�ܸ߷ֱ��ʵ�ͼ��֧��
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)//ʧ�ܴ��ڵ������İ�����ť
    QGuiApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);
#endif
    TiledApplication a(argc, argv);

    a.setOrganizationDomain(QLatin1String("mapeditor.org"));//����Ӧ�ó��������(ע�����)

#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    a.setApplicationName(QLatin1String("Tiled"));
#else
    a.setApplicationName(QLatin1String("tiled"));
#endif
    a.setApplicationDisplayName(QLatin1String("Tiled"));
    a.setApplicationVersion(QLatin1String(AS_STRING(TILED_VERSION)));
#ifdef Q_OS_MAC
    a.setAttribute(Qt::AA_DontShowIconsInMenus);
#endif
    StyleHelper::initialize();

    LanguageManager *languageManager = LanguageManager::instance();
    languageManager->installTranslators();//��װ����

    // Add the built-in file formats
    TmxMapFormat tmxMapFormat;//FileFormat
    PluginManager::addObject(&tmxMapFormat);

    TsxTilesetFormat tsxTilesetFormat;//FileFormat
    PluginManager::addObject(&tsxTilesetFormat);

    XmlObjectTemplateFormat xmlObjectTemplateFormat;//FileFormat
    PluginManager::addObject(&xmlObjectTemplateFormat);

    CommandLineHandler commandLine;

    if (!commandLine.parse(QCoreApplication::arguments()))
        return 0;
    if (commandLine.quit)
        return 0;
    if (commandLine.disableOpenGL)
        Preferences::instance()->setUseOpenGL(false);

    if (commandLine.exportMap) {//�����н���ʱ���õ����õ�,��������������Ϊ--export-mapΪʱ���ú��������ú������øñ���
        // Get the path to the source file and target file��ȡԴ�ļ���Ŀ���ļ���·��
        if (commandLine.exportTileset || commandLine.filesToOpen().length() < 2) {//�����в�������ʱ��ʾ�����ĸ�ʽ
            qWarning().noquote() << QCoreApplication::translate("Command line", "Export syntax is --export-map [format] <source> <target>");
            return 1;//tileset������exportMap�����ǳ�ͻ��
        }

        PluginManager::instance()->loadPlugins();

        int index = 0;
        qDebug()<< commandLine.filesToOpen().length() << commandLine.filesToOpen();
        //tiled.exe [formats] source target ����mFilesToOpen Ӧ��������
        const QString *filter = commandLine.filesToOpen().length() > 2 ? &commandLine.filesToOpen().at(index++) : nullptr;
        const QString &sourceFile = commandLine.filesToOpen().at(index++);
        const QString &targetFile = commandLine.filesToOpen().at(index++);

        QString errorMsg;
        //outputFormat������ʽ�Ķ���(plugin eg:json)
        MapFormat *outputFormat = findExportFormat<MapFormat>(filter, targetFile, errorMsg);//filter -> js json txm
        if (!outputFormat) {
            Q_ASSERT(!errorMsg.isEmpty());
            qWarning().noquote() << errorMsg;
            return 1;
        }

        // Load the source file
        const std::unique_ptr<Map> map(readMap(sourceFile, nullptr));
        if (!map) {
            qWarning().noquote() << QCoreApplication::translate("Command line", "Failed to load source map.");
            return 1;
        }

        // Write out the file
        bool success = outputFormat->write(map.get(), targetFile);//��Դ�ļ�xxx.tmx�ļ�дtarget.json�ļ���

        if (!success) {
            qWarning().noquote() << QCoreApplication::translate("Command line", "Failed to export map to target file.");
            return 1;
        }
        return 0;
    }

    if (commandLine.exportTileset) {
        // Get the path to the source file and target file
        if (commandLine.filesToOpen().length() < 2) {
            qWarning().noquote() << QCoreApplication::translate("Command line", "Export syntax is --export-tileset [format] <source> <target>");
            return 1;
        }

        PluginManager::instance()->loadPlugins();

        int index = 0;
         qDebug()<< commandLine.filesToOpen().length() << commandLine.filesToOpen();
        const QString *filter = commandLine.filesToOpen().length() > 2 ? &commandLine.filesToOpen().at(index++) : nullptr;
        qDebug()<<"file" << commandLine.filesToOpen();
        const QString &sourceFile = commandLine.filesToOpen().at(index++);
        const QString &targetFile = commandLine.filesToOpen().at(index++);

        QString errorMsg;
        TilesetFormat *outputFormat = findExportFormat<TilesetFormat>(filter, targetFile, errorMsg);//filter -> js json tsx
        if (!outputFormat) {
            Q_ASSERT(!errorMsg.isEmpty());
            qWarning().noquote() << errorMsg;
            return 1;
        }

        // Load the source file
        SharedTileset tileset(readTileset(sourceFile, nullptr));
        if (!tileset) {
            qWarning().noquote() << QCoreApplication::translate("Command line", "Failed to load source tileset.");
            return 1;
        }

        // Write out the file
        bool success = outputFormat->write(*tileset, targetFile);

        if (!success) {
            qWarning().noquote() << QCoreApplication::translate("Command line", "Failed to export tileset to target file.");
            return 1;
        }
        return 0;
    }

    if (!commandLine.filesToOpen().isEmpty() && !commandLine.newInstance) {
        // Convert files to absolute paths because the already running Tiled
        // instance likely does not have the same working directory.
        //���ļ�ת��Ϊ����·������Ϊ�Ѿ����е�ƽ��ʵ������û����ͬ�Ĺ���Ŀ¼��
        QStringList absolutePaths;
        for (const QString &fileName : commandLine.filesToOpen())
            absolutePaths.append(QFileInfo(fileName).absoluteFilePath());
        QJsonDocument doc(QJsonArray::fromStringList(absolutePaths));
        if (a.sendMessage(QLatin1String(doc.toJson())))//![1]//�ͻ��������˷�������[������Ϣʧ���Ƿ����]
            return 0;
    }

    std::unique_ptr<AutoUpdater> updater;
#ifdef TILED_SPARKLE//tiled_sparkle
#if defined(Q_OS_MAC)
    updater.reset(new SparkleAutoUpdater);
#elif defined(Q_OS_WIN)
    updater.reset(new WinSparkleAutoUpdater);
#endif
#endif

    MainWindow w;
    w.show();

    a.setActivationWindow(&w);
#if defined(Q_OS_WIN) && QT_VERSION >= 0x050700
    QWindowsWindowFunctions::setWindowActivationBehavior(QWindowsWindowFunctions::AlwaysActivateWindow);
#endif

    QObject::connect(&a, &TiledApplication::fileOpenRequest,
                     &w, [&] (const QString &file) { w.openFile(file); });

    PluginManager::instance()->loadPlugins();

    if (!commandLine.filesToOpen().isEmpty()) {
        for (const QString &fileName : commandLine.filesToOpen())
            w.openFile(fileName);//��������������ļ���ֱ�Ӵ�
    } else if (Preferences::instance()->openLastFilesOnStartup()) {
        w.openLastFiles();//�����򿪵��ļ�����������
        qDebug()<< "no file";
    }

    return a.exec();
}
