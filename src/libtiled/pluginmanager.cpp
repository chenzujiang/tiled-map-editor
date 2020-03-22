/*
 * pluginmanager.cpp
 * Copyright 2010, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
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

#include "pluginmanager.h"

#include "mapformat.h"
#include "plugin.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QPluginLoader>

namespace Tiled {

QString PluginFile::fileName() const
{
    if (loader)
        return loader->fileName();

    return QStringLiteral("<static>");
}
//�Ƿ���ڴ���
bool PluginFile::hasError() const
{
    if (instance)
        return false;

    return state == PluginEnabled || (defaultEnable && state == PluginDefault);
}
//Returns a text string with the description of the last error that occurred.
QString PluginFile::errorString() const
{
    if (loader)
        return loader->errorString();

    return QString();
}


PluginManager *PluginManager::mInstance;

PluginManager::PluginManager()
{
}

PluginManager::~PluginManager()
{
}

/**���������������״̬����Ϊ��ʽ���û���û�ʹ��Ĭ��״̬��
 * Sets the enabled state of the given plugin to be explicitly��ȷ�� enabled or
 * disabled or to use the default state.
 *
 * Returns whether the change was successful.
 */
bool PluginManager::setPluginState(const QString &fileName, PluginState state)
{
    if (state == PluginDefault)//Ĭ��״̬�Ƴ����
        mPluginStates.remove(fileName);//��map��ɾ�����е���key������item
    else
        mPluginStates.insert(fileName, state);

    PluginFile *plugin = pluginByFileName(fileName);
    if (!plugin)
        return false;

    plugin->state = state;

    bool loaded = plugin->instance != nullptr;
    bool enable = state == PluginEnabled || (plugin->defaultEnable && state != PluginDisabled);
    bool success = false;

    if (enable && !loaded) {
        success = loadPlugin(plugin);
    } else if (!enable && loaded) {
        success = unloadPlugin(plugin);
    } else {
        success = true;
    }

    return success;
}

bool PluginManager::loadPlugin(PluginFile *plugin)
{
    plugin->instance = plugin->loader->instance();

    if (plugin->instance) {
        if (Plugin *p = qobject_cast<Plugin*>(plugin->instance))
            p->initialize();
        else
            addObject(plugin->instance);

        return true;
    } else {
        qWarning().noquote() << "Error:" << plugin->loader->errorString();
        return false;
    }
}

bool PluginManager::unloadPlugin(PluginFile *plugin)
{
    bool derivedPlugin = qobject_cast<Plugin*>(plugin->instance) != nullptr;

    if (plugin->loader->unload()) {
        if (!derivedPlugin)
            removeObject(plugin->instance);

        plugin->instance = nullptr;
        return true;
    } else {
        return false;
    }
}
//��ȡ���������ʵ��
PluginManager *PluginManager::instance()
{
    if (!mInstance)
        mInstance = new PluginManager;

    return mInstance;
}

void PluginManager::deleteInstance()
{
    delete mInstance;
    mInstance = nullptr;
}

void PluginManager::addObject(QObject *object)
{
    Q_ASSERT(object);
    Q_ASSERT(mInstance);
    Q_ASSERT(!mInstance->mObjects.contains(object));//���mInstance->mObjects�����ɹ�����true��������û�а���

    mInstance->mObjects.append(object);//�����ò������������ߵ�ObjectList����ȥ
    emit mInstance->objectAdded(object);//����źŵ����þ�����Ӳ��ʱ�����̨������Ϣ
}

void PluginManager::removeObject(QObject *object)
{
    if (!mInstance)
        return;

    Q_ASSERT(object);
    Q_ASSERT(mInstance->mObjects.contains(object));//Returns true if the list contains an occurrence of value; otherwise returns false.

    emit mInstance->objectAboutToBeRemoved(object);
    mInstance->mObjects.removeOne(object);//Removes the first occurrence of value in the list and returns true on success; otherwise returns false.
}

/**ɨ����Ŀ¼���Ի�ȡ�������������
 * Scans the plugin directory for plugins and attempts to load them.
 */
void PluginManager::loadPlugins()
{
    // Load static plugins//�����ɲ��װ��������ľ�̬���ʵ������������б�
    const QObjectList &staticPluginInstances = QPluginLoader::staticInstances();
    for (QObject *instance : staticPluginInstances) {
        mPlugins.append(PluginFile(PluginStatic, instance));//��̬���
        qDebug()<<instance->objectName();
        if (Plugin *plugin = qobject_cast<Plugin*>(instance))//����ӿ��ṩ�Բ��ʵ�ֵ���չ�ķ���
            plugin->initialize();
        else
            addObject(instance);
    }

    // Determine the plugin path based on the application location����Ӧ�ó���λ��ȷ�����·��
#ifndef TILED_PLUGIN_DIR
    QString pluginPath = QCoreApplication::applicationDirPath();
#endif

#if defined(Q_OS_WIN32)
    pluginPath += QLatin1String("/plugins/tiled");
#elif defined(Q_OS_MAC)
    pluginPath += QLatin1String("/../PlugIns");
#elif defined(TILED_PLUGIN_DIR)
    QString pluginPath = QLatin1String(TILED_PLUGIN_DIR);
#else
    pluginPath += QLatin1String("/../lib/tiled/plugins");
#endif

    // Load dynamic plugins
    QDirIterator iterator(pluginPath, QDir::Files | QDir::Readable);
    while (iterator.hasNext()) //���Ŀ¼��������һ����Ŀ���򷵻�true;����,������false��
    {
        const QString &pluginFile = iterator.next();
        if (!QLibrary::isLibrary(pluginFile))//Returns true if fileName has a valid suffix for a loadable library; otherwise returns false.
            continue;

        QString fileName = QFileInfo(pluginFile).fileName();
        qDebug()<<fileName;
        PluginState state = mPluginStates.value(fileName);//ͨ����������ֻ�ȡ�����״̬

        auto *loader = new QPluginLoader(pluginFile, this);
        //QJsonObject QPluginLoader::metaData()������������Ԫ���ݡ�Ԫ�������ڱ�����ʱʹ��Q_PLUGIN_METADATA��ָ����json��ʽ�����ݡ�
        //.value()����һ��QJsonValue����ʾԿ�׼���ֵ��
        auto metaData = loader->metaData().value(QStringLiteral("MetaData")).toObject();
        bool defaultEnable = metaData.value(QStringLiteral("defaultEnable")).toBool();

        bool enable = state == PluginEnabled || (defaultEnable && state != PluginDisabled);

        QObject *instance = nullptr;

        if (enable) {
            instance = loader->instance();

            if (!instance)
                qWarning().noquote() << "Error:" << loader->errorString();
        }
        //�������info ������һ������ò��(�ļ�)����
        mPlugins.append(PluginFile(state, instance, loader, defaultEnable));

        if (!instance)
            continue;

        if (Plugin *plugin = qobject_cast<Plugin*>(instance))
            plugin->initialize();
        else
            addObject(instance);
    }
}
//ͨ����������ַ��ز��
PluginFile *PluginManager::pluginByFileName(const QString &fileName)
{
    for (PluginFile &plugin : mPlugins)
        if (plugin.loader && QFileInfo(plugin.loader->fileName()).fileName() == fileName)
            return &plugin;

    return nullptr;
}

} // namespace Tiled
