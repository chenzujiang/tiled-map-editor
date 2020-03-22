/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Solutions component.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include "qtlocalpeer.h"
#include <QCoreApplication>
#include <QDataStream>
#include <QTime>
#if defined(Q_OS_WIN)
#include <QLibrary>
#include <qt_windows.h>
typedef BOOL(WINAPI*PProcessIdToSessionId)(DWORD,DWORD*);
//typedef int(*P)(int,int*)//function pointer
//      一个指针类型          定义了一个指针对象       = 0;
static PProcessIdToSessionId pProcessIdToSessionId = 0;
#endif//int Max(int,int*)
          //      int* p;        int a; p = &a;    //  指针变量他是一个变量 int* p; p取取到的只是一个地址。*P取到这个地址对应的内容
//int(*p)(int,int*)中的P是指针变量，这个变量存储的是该种(类型)的地址。[adrr]P = Max,这个变量存这个函数的地址 *P取这个指针的内容,Max里面的内容
//typedef int(*P)(int,int*)中的P是种类型，类型定义变量，该变量就只能存储类型的{内容}
//                             (类型)QFunctionPointer resolve(const char *symbol);
//pProcessIdToSessionId = (PProcessIdToSessionId)lib.resolve("ProcessIdToSessionId");返回的是一个变量
#if defined(Q_OS_UNIX)
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#endif

namespace QtLP_Private {
#include "qtlockedfile.cpp"
#if defined(Q_OS_WIN)
#include "qtlockedfile_win.cpp"
#else
#include "qtlockedfile_unix.cpp"
#endif
}

const char* QtLocalPeer::ack = "ack";

QtLocalPeer::QtLocalPeer(QObject* parent, const QString &appId)
    : QObject(parent), id(appId)
{
    QString prefix = id;
    if (id.isEmpty()) {
        id = QCoreApplication::applicationFilePath();       //返回此应用程序的路径(.exe).
#if defined(Q_OS_WIN)
        id = id.toLower();
#endif
        prefix = id.section(QLatin1Char('/'), -1);          //返回第一个/开始的字符(从右到左)tiled.exe
    }
    prefix.remove(QRegExp(QLatin1String("[^a-zA-Z]")));     //移除不是字符a-zA-Z的所有字符
    prefix.truncate(6);                                     //缩短字符到给定的索引位置处(截取前6个字符0-5)(tilede)

    QByteArray idc = id.toUtf8();
    quint16 idNum = qChecksum(idc.constData(), idc.size());//返回计算给定数据的开始的len字节基于CRC-16-CCITT算法的校验码(一个4位16进制数)
    socketName = QLatin1String("qtsingleapp-") + prefix + QLatin1Char('-') + QString::number(idNum, 16);//qtsingleapp-tilede-xxxx
/*    typedef int (*AvgFunction)(int, int);
//    AvgFunction avg = (AvgFunction) library->resolve("avg");
//    if (avg)
//        return avg(5, 8);
//    else
//        return -1;*/
#if defined(Q_OS_WIN)
    if (!pProcessIdToSessionId) {
        QLibrary lib("kernel32");
        pProcessIdToSessionId = (PProcessIdToSessionId)lib.resolve("ProcessIdToSessionId");
    }
    if (pProcessIdToSessionId) {
        DWORD sessionId = 0;
        pProcessIdToSessionId(GetCurrentProcessId(), &sessionId);   //检索与指定进程关联的远程桌面服务会话retrieves the remote desktop services session associated with a specified process
        socketName += QLatin1Char('-') + QString::number(sessionId, 16);
    }
#else
    socketName += QLatin1Char('-') + QString::number(::getuid(), 16);
#endif
    qDebug()<<socketName;//"qtsingleapp-tilede-ef8a-1"
    server = new QLocalServer(this);
    QString lockName = QDir(QDir::tempPath()).absolutePath()        //返回系统临时目录的绝对路径并转换绝对路径
                       + QLatin1Char('/') + socketName              //qtsingleapp-7d69-1
                       + QLatin1String("-lockfile");

    lockFile.setFileName(lockName);                                 //设置文件的名称,名称可以没有路径、相对路径或绝对路径。
    lockFile.open(QIODevice::ReadWrite);                            //C:/Users/atk/AppData/Local/Temp/qtsingleapp-tilede-ef8a-1-lockfile文件
}


//客户端是锁不上的
bool QtLocalPeer::isClient()
{
    if (lockFile.isLocked()) //锁住的是服务端
        return false;

    if (!lockFile.lock(QtLP_Private::QtLockedFile::WriteLock, false))//能锁是服务端，客户端不能锁上
        return true;
    //告诉服务器侦听按名称传入的连接。如果服务器当前正在监听，那么它将返回false。成功返回true，否则返回false。
    bool res = server->listen(socketName);//如果是服务端,第一次没有锁，调用lock锁上。锁上之后，调用listen告知服务端侦听socketName传来的链接
#if defined(Q_OS_UNIX) && (QT_VERSION >= QT_VERSION_CHECK(4,5,0))
    // ### Workaround
    if (!res && server->serverError() == QAbstractSocket::AddressInUseError) {
        QFile::remove(QDir::cleanPath(QDir::tempPath())+QLatin1Char('/')+socketName);
        res = server->listen(socketName);
    }
#endif
    if (!res)
        qWarning("QtSingleCoreApplication: listen on local socket failed, %s", qUtf8Printable(server->errorString()));
    //This signal is emitted every time a new connection is available.每一次都有一个可用链接是发送这个信号
    QObject::connect(server, SIGNAL(newConnection()), SLOT(receiveConnection()));
    return false;
}

// ![2]第一次运行的充当服务端,将lockName锁住,调用Listen进行监听socketName。后面的是客户端
bool QtLocalPeer::sendMessage(const QString &message, int timeout)
{
    if (!isClient()){//客户端发送消息
        qDebug()<< "server";
        return false;//服务端调用这个函数在此返回,不会进行数据的发送环节
    }
    else{
        qDebug()<< "client";
    }
    QLocalSocket socket;
    bool connOk = false;
    for(int i = 0; i < 2; i++) {
        //[1]链接到服务端
        // Try twice, in case the other instance is just starting up尝试两次，以防另一个实例刚刚启动
        socket.connectToServer(socketName);//链接服务端,因为服务端是监听在这个socketName的链接[如果建立了连接，QlocalSocket将进入ConnectedState并发出Connected（）。]
        connOk = socket.waitForConnected(timeout/2);//如果建立连接返回true
        //[3-1]等待链接成功应答
        if (connOk || i)
            break;
        int ms = 250;
#if defined(Q_OS_WIN)
        Sleep(DWORD(ms));
#else
        struct timespec ts = { ms / 1000, (ms % 1000) * 1000 * 1000 };
        nanosleep(&ts, NULL);
#endif
    }
    if (!connOk)
        return false;

    QByteArray uMsg(message.toUtf8());
    QDataStream ds(&socket);
     qDebug()<<"send"<<uMsg;//[4]
    ds.writeBytes(uMsg.constData(), uMsg.size());//将缓冲区uMsg.costData中的,uMsg.size()长度的数据写入到流中，并返回流的引用
    //对于缓冲设备，此函数将等待缓冲写入数据的有效负载被写入设备并发出byteswrite()信号，
    //或者等待msecs毫秒过去。如果msecs是-1，这个函数就不会超时。对于未缓冲的设备，它立即返回//tcp之间发送，属于缓冲设备
    bool res = socket.waitForBytesWritten(timeout);//等待写完成
    if (res) {
    //块，直到有新的数据可用(reading)并发出readyRead()信号，或者直到经过msecs毫秒。如果msecs是-1，这个函数就不会超时
        res &= socket.waitForReadyRead(timeout);   // wait for ack
        if (res)//    ^这个函数等等服务端发送来数据
        {
           // qDebug()<< socket.read(qstrlen(ack));debug读取了之后就读取不来了，这里debug为造成
            res &= (socket.read(qstrlen(ack)) == ack);//server端的应答数据ack字符串
        }
    }
    return res;
}

// ![3]
void QtLocalPeer::receiveConnection()
{
    QLocalSocket* socket = server->nextPendingConnection();
    qDebug()<< "new connection";
    if (!socket)
        return;
    //[2]链接成功
    //返回可供读取的字节数。此函数通常与顺序设备一起使用，以确定读取之前在缓冲区中分配的字节数。
    while (socket->bytesAvailable() < (int)sizeof(quint32))
        socket->waitForReadyRead();//[3]接收client的消息循环
    qDebug()<< "bytesAvailable";
    //[5]读取到相关的数据，然后处理即将数据存储到message对象中
    QDataStream ds(socket);
    QByteArray uMsg;
    quint32 remaining;//剩余的
    ds >> remaining;
    uMsg.resize(remaining);//设置uMsg字节数大小,有这么大一块空间
    int got = 0;
    char* uMsgBuf = uMsg.data();
    do {//最多读取len个字节到uMsgBuf中一次
        got = ds.readRawData(uMsgBuf, remaining);//将套接字的数据读入到缓冲区中
        remaining -= got;//有可能一次读取不完
        uMsgBuf += got;
    } while (remaining && got >= 0 && socket->waitForReadyRead(2000));//如果数据可用来读取，函数返回true;否则返回false
    if (got < 0) {
        qWarning("QtLocalPeer: Message reception failed %s", socket->errorString().toLatin1().constData());
        delete socket;
        return;
    }
    QString message(QString::fromUtf8(uMsg));//uMsgBuf指针指向的是开辟的缓冲区(接收网络数据的区域)开始位置
    //[6]应答Client端
    socket->write(ack, qstrlen(ack));//将以\0结尾的8-bit字符中的数据写入设备。返回实际写入的字节数，如果发生错误，返回-1
    socket->waitForBytesWritten(1000);
    socket->waitForDisconnected(1000); // make sure client reads ack确保客户端读取ack
    delete socket;
    emit messageReceived(message); //### (might take a long time to return)//可能需要一个长的时间才能返回
}
//总结：1 服务端先启动起来锁定文件,服务端启动起来锁定不了文件了变成客户端
//     2 客户端需要发起连接请求，进入等待server应答循环
//     3 当客户端调用请求函数connectToServer,\
//       server发送新的链接请求信号newConnection,进入链接槽函数。判断\
//       next链接socket是否为空。不空表示链接成功。进入接收clicent消息循环
//     4 客户单发送消息，如果发送成功，等待进入接收server服务端的应答信息循环
//     5 服务端接收到消息之后。将信息存储到message对象中。发送应答client ack字符串\
//         确保客户端能够有时间接收和处理。延时删除socket
//     6 client接收到消息之后验证消息。退出。
//     7 server将接收到的信息进行发送发送到软件里面进行处理
//注意：确保能够进入client-server-system需要携带文件
