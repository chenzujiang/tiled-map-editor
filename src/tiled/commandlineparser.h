/*
 * commandlineparser.h
 * Copyright 2011, Ben Longbons <b.r.longbons@gmail.com>
 * Copyright 2011, Thorbj酶rn Lindeijer <thorbjorn@lindeijer.nl>
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

#include <QCoreApplication>
#include <QStringList>
#include <QVector>

namespace Tiled {
namespace Internal {

/**
 * C-style callback function taking an arbitrary任意 data pointer.
 */
typedef void (*Callback)(void *data);

/**
 * A template function that will static-cast the given \a object to a type T
 * and call the member function of T given in the second template argument.
 *///void (T::* memberFunction)()变量 是T类中某个函数对象的地址
template<typename T,
         void (T::* memberFunction)()>
void MemberFunctionCall(void *object)
{//将void * 转换为其他类型
    T *t = static_cast<T*>(object);
    //传来的函数变量节引用取出函数调用之
    (t->*memberFunction)();
}


/**
 * A simple command line parser. Options should be registered through
 * registerOption().
 *定义了一个简单的命令行解析器，通过注册选项函数进行注册。
 * The help option (-h/--help) is provided by the parser based on the
 * registered options. help选项是由解析器基于注册选项提供的。
 */
class CommandLineParser
{
    Q_DECLARE_TR_FUNCTIONS(CommandLineParser)//给非qt类添加翻译支持

public:
    CommandLineParser();

    /**用解析器注册一个选项，当一个选项与给定的shortName or longName 相遇。就会调用回调，
     * 数据作为它唯一的参数
     * Registers an option with the parser. When an option with the given
     * \a shortName or \a longName is encountered, \a callback is called with
     * \a data as its only parameter.
     *///callback变量存储的是这种 typedef void (*Callback)(void *data)类型的函数
    void registerOption(Callback callback,
                        void *data,
                        QChar shortName,
                        const QString &longName,
                        const QString &help);

    /**
     * Convenience overload that allows registering an option with a callback
     * as a member function of a class. The class type and the member function
     * are given as template parameters, while the instance is passed in as
     * \a handler.
     *方便重载,允许回调作为类的成员函数注册一个选项.类类型和和成员函数被通过模板参数给出,
     * 而实例作为处理器传入
     * \overload
     */
    template <typename T, void (T::* memberFunction)()>
    void registerOption(T* handler,
                        QChar shortName,
                        const QString &longName,
                        const QString &help)
    {
        registerOption(MemberFunctionCall<T, memberFunction>,
                       handler,
                       shortName,
                       longName,
                       help);
    }

    /**解析给定的参数。当应用程序不被期望运行时返回false（要么有解析错误，要么请求帮助）
     * Parses the given \a arguments. Returns false when the application is not
     * expected to run (either there was a parsing error, or the help was
     * requested).
     */
    bool parse(const QStringList &arguments);

    /**返回在arguments(参数)中找到的文件s
     * Returns the files to open that were found among the arguments.
     */
    const QStringList &filesToOpen() const { return mFilesToOpen; }//命令行参数解析时append进去的

private:
    void showHelp() const;

    bool handleLongOption(const QString &longName);
    bool handleShortOption(QChar c);

    /**命令行选项的内部定义
     * Internal definition of a command line option.
     */
    struct Option
    {
        Option()
            : callback(nullptr)//callback是一个函数变量。存的是函数名
            , data(nullptr)
        {}

        Option(Callback callback,
               void *data,
               QChar shortName,
               QString longName,
               QString help)
            : callback(callback)
            , data(data)
            , shortName(shortName)
            , longName(std::move(longName))
            , help(std::move(help))
        {}

        Callback callback;
        void *data;
        QChar shortName;
        QString longName;
        QString help;
    };

    QVector<Option> mOptions;//mOption.append(Option(callback, data, shortName, longName, help))
    int mLongestArgument;
    QString mCurrentProgramName;
    QStringList mFilesToOpen;
    bool mShowHelp;
};

} // namespace Internal
} // namespace Tiled
