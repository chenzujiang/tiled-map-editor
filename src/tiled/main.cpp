/*
 * main.cpp
 * Copyright 2008-2011, Thorbj酶rn Lindeijer <thorbjorn@lindeijer.nl>
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

class CommandLineHandler : public CommandLineParser//命令行分析器
{
    Q_DECLARE_TR_FUNCTIONS(CommandLineHandler)//给非Qt类添加翻译支持
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

    // Convenience wrapper around registerOption 方便在附近包装注册操作
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

/**当使用文件导出的时候，尝试 下定决心从命令行参数中输出文件的格式
 * Used during file export, attempt to determine the output file format
 * from the command line parameters.
 * Query errorMsg if result is null.查询 errorMsg 如果返回NULL
 */
template <typename T>
inline T *findExportFormat(const QString *filter,
                           const QString &targetFile,
                           QString &errorMsg)
{
    T *outputFormat = nullptr;
    const auto formats = PluginManager::objects<T>();// auto==QList<T*>
    if (filter) {//如果命令行命运给格式[foormat]不进这里,->else通过文件名去匹配
        // Find the format supporting the given filter查找支持给定筛选器的格式
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
    else {//命令行给的导出格式,找不到格式对象时,命令行没有给格式
        // Find the format based on target file extension根据文件的扩展名查找文件
        QString suffix = QFileInfo(targetFile).completeSuffix();//完整的文件扩展名由第一个.之后的文件名称的所有的字符构成
        for (T *format : formats) {
            if (!format->hasCapabilities(T::Write))
                continue;
            qDebug()<<"suffix"<<  suffix;
            //文件格式指文件后缀.tms,.xml
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

    return outputFormat;//返回导出格式对象(插件,eg:json)
}


} // anonymous namespace匿名名字空间


CommandLineHandler::CommandLineHandler()
    : quit(false)
    , showedVersion(false)
    , disableOpenGL(false)
    , exportMap(false)
    , exportTileset(false)
    , newInstance(false)
{
    option<&CommandLineHandler::showVersion>(//CommandLineHandler::showVersion对象
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
//需要(合理)退出
void CommandLineHandler::justQuit()
{
    quit = true;
}

void CommandLineHandler::setDisableOpenGL()
{
    disableOpenGL = true;//openGL 专业的图形程序接口,夸编程语言和平台
}
//设置导出地图
void CommandLineHandler::setExportMap()
{
    exportMap = true;
}
//设置导出
void CommandLineHandler::setExportTileset()
{
    exportTileset = true;
}
//显示支持导出的格式文件格式 （瓦砖和map两种格式所有对象名称）
void CommandLineHandler::showExportFormats()
{
    PluginManager::instance()->loadPlugins();

    QStringList formats;
    for (MapFormat *format : PluginManager::objects<MapFormat>()) {
        if (format->hasCapabilities(MapFormat::Write))
            formats.append(format->shortName());//反回所有T类型的有写能力的对象,显示他们的名称,都是支持的导出格式
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
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {//将调用进程附加到指定进程的控制台。((unsigned long)-1 )-->使用当前进程的父窗口的控制台
        FILE *dummy = nullptr;                 //https://cloud.tencent.com/developer/section/1009141
        freopen_s(&dummy, "CONOUT$", "w", stdout);//将标准输出和错误输出重向到控制台
        freopen_s(&dummy, "CONOUT$", "w", stderr);
    }
#endif

#if QT_VERSION >= 0x050600//Sets whether QGuiApplication will use fallback session management to enabled.
    QGuiApplication::setFallbackSessionManagementEnabled(false);//是否将使用备用的依赖的(备用)回话管理(使能)
#endif

    // Enable support for highres images (added in Qt 5.1, but off by default)使能高分辨率的图形支持
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)//失能窗口的上下文帮助按钮
    QGuiApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);
#endif
    TiledApplication a(argc, argv);

    a.setOrganizationDomain(QLatin1String("mapeditor.org"));//设置应用程序的域名(注册表中)

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
    languageManager->installTranslators();//安装翻译

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

    if (commandLine.exportMap) {//命令行解析时调用的设置的,当长的命令行中为--export-map为时调用函数。调用函数设置该变量
        // Get the path to the source file and target file获取源文件和目标文件的路径
        if (commandLine.exportTileset || commandLine.filesToOpen().length() < 2) {//命令行参数不对时显示导出的格式
            qWarning().noquote() << QCoreApplication::translate("Command line", "Export syntax is --export-map [format] <source> <target>");
            return 1;//tileset导出和exportMap导出是冲突的
        }

        PluginManager::instance()->loadPlugins();

        int index = 0;
        qDebug()<< commandLine.filesToOpen().length() << commandLine.filesToOpen();
        //tiled.exe [formats] source target 参数mFilesToOpen 应该是三个
        const QString *filter = commandLine.filesToOpen().length() > 2 ? &commandLine.filesToOpen().at(index++) : nullptr;
        const QString &sourceFile = commandLine.filesToOpen().at(index++);
        const QString &targetFile = commandLine.filesToOpen().at(index++);

        QString errorMsg;
        //outputFormat导出格式的对象(plugin eg:json)
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
        bool success = outputFormat->write(map.get(), targetFile);//把源文件xxx.tmx文件写target.json文件中

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
        //将文件转换为绝对路径，因为已经运行的平铺实例可能没有相同的工作目录。
        QStringList absolutePaths;
        for (const QString &fileName : commandLine.filesToOpen())
            absolutePaths.append(QFileInfo(fileName).absoluteFilePath());
        QJsonDocument doc(QJsonArray::fromStringList(absolutePaths));
        if (a.sendMessage(QLatin1String(doc.toJson())))//![1]//客户端向服务端发送数据[发送消息失败是服务端]
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
            w.openFile(fileName);//如果命令行中有文件，直接打开
    } else if (Preferences::instance()->openLastFilesOnStartup()) {
        w.openLastFiles();//将最后打开的文件保存起来的
        qDebug()<< "no file";
    }

    return a.exec();
}
