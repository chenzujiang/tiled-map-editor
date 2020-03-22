/*
 * commandlineparser.cpp
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

#include "commandlineparser.h"

#include <QDebug>
#include <QFileInfo>

#include "qtcompat_p.h"

using namespace Tiled;
using namespace Tiled::Internal;

CommandLineParser::CommandLineParser()
    : mLongestArgument(0)
    , mShowHelp(false)
{
}
//-->这个变量里面存储的是一个函数名,编译的时候通过构造中调用
//option<...>(...)这个函数构造出来的
void CommandLineParser::registerOption(Callback callback,
                                       void *data,
                                       QChar shortName,
                                       const QString &longName,
                                       const QString &help)
{
    mOptions.append(Option(callback, data, shortName, longName, help));

    const int length = longName.length();
    if (mLongestArgument < length)
        mLongestArgument = length;
}
//解析给定的参数。当应用程序不被期望运行时返回false（要么有解析错误，要么请求帮助）
bool CommandLineParser::parse(const QStringList &arguments)//命令行参数解析,将相应的文件保存，调用相关的导出
{
    mFilesToOpen.clear();
    mShowHelp = false;

    QStringList todo = arguments;
    mCurrentProgramName = QFileInfo(todo.takeFirst()).fileName();//takeFirst()返回应用并移除
    qDebug()<< mCurrentProgramName <<"move"<< todo;
    //获取当前程序的名称
    int index = 0;
    bool noMoreArguments = false;

    while (!todo.isEmpty())
    {//进入这里面来了就是实际的参数区
        index++;
        const QString arg = todo.takeFirst();
        qDebug()<< "parser" <<arg;
        if (arg.isEmpty())
            continue;
        //Returns the character at the given index position in the string.
        if (noMoreArguments || arg.at(0) != QLatin1Char('-'))//是假,继续判断后面参数
        {// noMoreArguments begin is false
            mFilesToOpen.append(arg);//如果参数不是--说明是操作文件,[文件]操作处理 [source] [target]
            continue;
        }

        if (arg.length() == 1) {//传统的单个链接符"-"读取文件从标准输入中读取文件，写文件到标准输出，这个不能真确的支持现在
            // Traditionally a single hyphen means read file from stdin,
            // write file to stdout. This isn't supported right now.
            qWarning().noquote() << tr("Bad argument %1: lonely hyphen").arg(index);//错误参数，孤立的链接符
            showHelp();
            return false;
        }

        // Long options
        if (arg.at(1) == QLatin1Char('-')) {
            // Double hypen "--" means no more options will follow
            if (arg.length() == 2) {
                noMoreArguments = true;
                continue;
            }

            if (!handleLongOption(arg)) {//[选项]参数处理 --xxxx
                qWarning().noquote() << tr("Unknown long argument %1: %2").arg(index).arg(arg);
                mShowHelp = true;
                break;
            }

            continue;
        }

        // Short options
        for (int i = 1; i < arg.length(); ++i) {
            const QChar c = arg.at(i);
            if (!handleShortOption(c)) {
                qDebug()<< c;
                qWarning().noquote() << tr("Unknown short argument %1.%2: %3").arg(index).arg(i).arg(c);
                mShowHelp = true;
                break;
            }
        }
    }

    if (mShowHelp) {
        showHelp();
        return false;
    }

    return true;
}

void CommandLineParser::showHelp() const
{//noquote没有引用 在多行中打印包括换行符的string
    qWarning().noquote() << tr("Usage:\n  %1 [options] [files...]").arg(mCurrentProgramName)
                         << "\n\n"
                         << tr("Options:");
    //%-*s 代表输入一个字符串，-号代表左对齐、后补空白，*号代表对齐宽度由输入时确定
    //%*s 代表输入一个字符串，右对齐、前补空白，*号代表对齐宽度由输入时确定
    qWarning("  -h %-*s : %s", mLongestArgument/*对齐宽度*/, "--help", qUtf8Printable(tr("Display this help")));

    for (const Option &option : mOptions) {
        if (!option.shortName.isNull()) {
            qWarning("  -%c %-*s : %s",
                     option.shortName.toLatin1(),
                     mLongestArgument,
                     qUtf8Printable(option.longName),
                     qUtf8Printable(option.help));
        } else {
            qWarning("     %-*s : %s",
                     mLongestArgument,
                     qUtf8Printable(option.longName),
                     qUtf8Printable(option.help));
        }
    }

    qWarning();
}

bool CommandLineParser::handleLongOption(const QString &longName)
{
    if (longName == QLatin1String("--help")) {
        mShowHelp = true;
        return true;
    }
    for (const Option &option : qAsConst(mOptions)) {
        if (longName == option.longName) {
            option.callback(option.data);
            return true;
        }
    }

    return false;
}

bool CommandLineParser::handleShortOption(QChar c)
{
    if (c == QLatin1Char('h')) {
        mShowHelp = true;
        return true;
    }
            qDebug()<< c;
    for (const Option &option : qAsConst(mOptions)) {
        if (c == option.shortName) {
            option.callback(option.data);//callback  -->是指针变量，里面存储的是void (*callback) (void * data);函数首地址
            return true;
        }
    }

    return false;
}
