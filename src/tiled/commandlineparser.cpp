/*
 * commandlineparser.cpp
 * Copyright 2011, Ben Longbons <b.r.longbons@gmail.com>
 * Copyright 2011, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
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
//-->�����������洢����һ��������,�����ʱ��ͨ�������е���
//option<...>(...)����������������
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
//���������Ĳ�������Ӧ�ó��򲻱���������ʱ����false��Ҫô�н�������Ҫô���������
bool CommandLineParser::parse(const QStringList &arguments)//�����в�������,����Ӧ���ļ����棬������صĵ���
{
    mFilesToOpen.clear();
    mShowHelp = false;

    QStringList todo = arguments;
    mCurrentProgramName = QFileInfo(todo.takeFirst()).fileName();//takeFirst()����Ӧ�ò��Ƴ�
    qDebug()<< mCurrentProgramName <<"move"<< todo;
    //��ȡ��ǰ���������
    int index = 0;
    bool noMoreArguments = false;

    while (!todo.isEmpty())
    {//�������������˾���ʵ�ʵĲ�����
        index++;
        const QString arg = todo.takeFirst();
        qDebug()<< "parser" <<arg;
        if (arg.isEmpty())
            continue;
        //Returns the character at the given index position in the string.
        if (noMoreArguments || arg.at(0) != QLatin1Char('-'))//�Ǽ�,�����жϺ������
        {// noMoreArguments begin is false
            mFilesToOpen.append(arg);//�����������--˵���ǲ����ļ�,[�ļ�]�������� [source] [target]
            continue;
        }

        if (arg.length() == 1) {//��ͳ�ĵ������ӷ�"-"��ȡ�ļ��ӱ�׼�����ж�ȡ�ļ���д�ļ�����׼��������������ȷ��֧������
            // Traditionally a single hyphen means read file from stdin,
            // write file to stdout. This isn't supported right now.
            qWarning().noquote() << tr("Bad argument %1: lonely hyphen").arg(index);//������������������ӷ�
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

            if (!handleLongOption(arg)) {//[ѡ��]�������� --xxxx
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
{//noquoteû������ �ڶ����д�ӡ�������з���string
    qWarning().noquote() << tr("Usage:\n  %1 [options] [files...]").arg(mCurrentProgramName)
                         << "\n\n"
                         << tr("Options:");
    //%-*s ��������һ���ַ�����-�Ŵ�������롢�󲹿հף�*�Ŵ��������������ʱȷ��
    //%*s ��������һ���ַ������Ҷ��롢ǰ���հף�*�Ŵ��������������ʱȷ��
    qWarning("  -h %-*s : %s", mLongestArgument/*������*/, "--help", qUtf8Printable(tr("Display this help")));

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
            option.callback(option.data);//callback  -->��ָ�����������洢����void (*callback) (void * data);�����׵�ַ
            return true;
        }
    }

    return false;
}
