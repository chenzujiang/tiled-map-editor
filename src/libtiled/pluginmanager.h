/*
 * pluginmanager.h
 * Copyright 2010, Thorbj酶rn Lindeijer <thorbjorn@lindeijer.nl>
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

#include "tiled_global.h"

#include <QList>
#include <QMap>
#include <QObject>
#include <QString>

#include <functional>

class QPluginLoader;

namespace Tiled {

enum PluginState
{
    PluginDefault,
    PluginEnabled,
    PluginDisabled,
    PluginStatic
};

struct TILEDSHARED_EXPORT PluginFile
{
    PluginFile(PluginState state,
               QObject *instance,
               QPluginLoader *loader = nullptr,
               bool defaultEnable = true)
        : state(state)
        , instance(instance)
        , loader(loader)
        , defaultEnable(defaultEnable)
    {}

    QString fileName() const;
    bool hasError() const;
    QString errorString() const;

    PluginState state;
    QObject *instance;
    QPluginLoader *loader;
    bool defaultEnable;
};


/**插件管理器加载插件并提供访问它们的方法。
 * The plugin manager loads the plugins and provides ways to access them.
 */
class TILEDSHARED_EXPORT PluginManager : public QObject
{
    Q_OBJECT

public:
    /**
     * Returns the plugin manager instance.
     */
    static PluginManager *instance();

    static void deleteInstance();

    /**扫描插件目录，以获取插件并加载他们
     * Scans the plugin directory for plugins and attempts to load them.
     */
    void loadPlugins();

    /**返回由插件管理器发现的插件列表
     * Returns the list of plugins found by the plugin manager.
     */
    const QList<PluginFile> &plugins() const { return mPlugins; }

    /**
     * Adds the given \a object. This allows the object to be found later based
     * on the interfaces it implements.添加一个给定的对象，允许这个对象在稍后的基类的接口找到他的实现
     */
    static void addObject(QObject *object);

    /**
     * Removes the given \a object.
     */
    static void removeObject(QObject *object);

    /**返回一个给定接口的对象列表的实现(实例)//返回一个列表对象实例，接口所规定的(对象列表实例)     插件管理的
     * Returns the list of objects that implement a given interface.
     */
    template<typename T>
    static QList<T*> objects()
    {
        QList<T*> results;
        if (mInstance)//c++11基于范围的迭代
            for (QObject *object : mInstance->mObjects)//从插件管理者中获取objectList::mobject
                if (T *result = qobject_cast<T*>(object))
                    results.append(result);
        return results;
    }
//https://blog.csdn.net/linwh8/article/details/51569807 c++11
    /**为每一个对象的实现指定相应的调用函数接口
     * Calls the given function for each object implementing a given interface.
     */
    template<typename T>
    static void each(std::function<void(T*)> function)//它是一个类型 形参，function是一个函数拥有void返回值和T*参数https://blog.csdn.net/infoworld/article/details/51248953
    {
        if (mInstance)
            for (QObject *object : mInstance->mObjects)
                if (T *result = qobject_cast<T*>(object))//MapDocument is exist in objectList
                    function(result);
    }// std::function<int(int ,int)>func=add;//<int(int,int)>是实例化模板参数,表示返回值为int,函数参数为2个,(int,int),即int(*pfunc)(int ,int )类型的函数

    PluginFile *pluginByFileName(const QString &fileName);//函数声明,有一个参数，返回值为结构体

    const QMap<QString, PluginState> &pluginStates() const;//返回插件状态的引用
    bool setPluginState(const QString &fileName, PluginState state);

signals:
    void objectAdded(QObject *object);
    void objectAboutToBeRemoved(QObject *object);//移除指定对象

private:
    Q_DISABLE_COPY(PluginManager)

    PluginManager();
    ~PluginManager();

    bool loadPlugin(PluginFile *plugin);
    bool unloadPlugin(PluginFile *plugin);

    static PluginManager *mInstance;

    QList<PluginFile> mPlugins;
    QMap<QString, PluginState> mPluginStates;
    QObjectList mObjects;
};


inline const QMap<QString, PluginState> &PluginManager::pluginStates() const
{
    return mPluginStates;
}

} // namespace Tiled
