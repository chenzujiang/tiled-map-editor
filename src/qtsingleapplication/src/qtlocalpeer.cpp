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
//      һ��ָ������          ������һ��ָ�����       = 0;
static PProcessIdToSessionId pProcessIdToSessionId = 0;
#endif//int Max(int,int*)
          //      int* p;        int a; p = &a;    //  ָ���������һ������ int* p; pȡȡ����ֻ��һ����ַ��*Pȡ�������ַ��Ӧ������
//int(*p)(int,int*)�е�P��ָ���������������洢���Ǹ���(����)�ĵ�ַ��[adrr]P = Max,�����������������ĵ�ַ *Pȡ���ָ�������,Max���������
//typedef int(*P)(int,int*)�е�P�������ͣ����Ͷ���������ñ�����ֻ�ܴ洢���͵�{����}
//                             (����)QFunctionPointer resolve(const char *symbol);
//pProcessIdToSessionId = (PProcessIdToSessionId)lib.resolve("ProcessIdToSessionId");���ص���һ������
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
        id = QCoreApplication::applicationFilePath();       //���ش�Ӧ�ó����·��(.exe).
#if defined(Q_OS_WIN)
        id = id.toLower();
#endif
        prefix = id.section(QLatin1Char('/'), -1);          //���ص�һ��/��ʼ���ַ�(���ҵ���)tiled.exe
    }
    prefix.remove(QRegExp(QLatin1String("[^a-zA-Z]")));     //�Ƴ������ַ�a-zA-Z�������ַ�
    prefix.truncate(6);                                     //�����ַ�������������λ�ô�(��ȡǰ6���ַ�0-5)(tilede)

    QByteArray idc = id.toUtf8();
    quint16 idNum = qChecksum(idc.constData(), idc.size());//���ؼ���������ݵĿ�ʼ��len�ֽڻ���CRC-16-CCITT�㷨��У����(һ��4λ16������)
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
        pProcessIdToSessionId(GetCurrentProcessId(), &sessionId);   //������ָ�����̹�����Զ���������Ựretrieves the remote desktop services session associated with a specified process
        socketName += QLatin1Char('-') + QString::number(sessionId, 16);
    }
#else
    socketName += QLatin1Char('-') + QString::number(::getuid(), 16);
#endif
    qDebug()<<socketName;//"qtsingleapp-tilede-ef8a-1"
    server = new QLocalServer(this);
    QString lockName = QDir(QDir::tempPath()).absolutePath()        //����ϵͳ��ʱĿ¼�ľ���·����ת������·��
                       + QLatin1Char('/') + socketName              //qtsingleapp-7d69-1
                       + QLatin1String("-lockfile");

    lockFile.setFileName(lockName);                                 //�����ļ�������,���ƿ���û��·�������·�������·����
    lockFile.open(QIODevice::ReadWrite);                            //C:/Users/atk/AppData/Local/Temp/qtsingleapp-tilede-ef8a-1-lockfile�ļ�
}


//�ͻ����������ϵ�
bool QtLocalPeer::isClient()
{
    if (lockFile.isLocked()) //��ס���Ƿ����
        return false;

    if (!lockFile.lock(QtLP_Private::QtLockedFile::WriteLock, false))//�����Ƿ���ˣ��ͻ��˲�������
        return true;
    //���߷��������������ƴ�������ӡ������������ǰ���ڼ�������ô��������false���ɹ�����true�����򷵻�false��
    bool res = server->listen(socketName);//����Ƿ����,��һ��û����������lock���ϡ�����֮�󣬵���listen��֪���������socketName����������
#if defined(Q_OS_UNIX) && (QT_VERSION >= QT_VERSION_CHECK(4,5,0))
    // ### Workaround
    if (!res && server->serverError() == QAbstractSocket::AddressInUseError) {
        QFile::remove(QDir::cleanPath(QDir::tempPath())+QLatin1Char('/')+socketName);
        res = server->listen(socketName);
    }
#endif
    if (!res)
        qWarning("QtSingleCoreApplication: listen on local socket failed, %s", qUtf8Printable(server->errorString()));
    //This signal is emitted every time a new connection is available.ÿһ�ζ���һ�����������Ƿ�������ź�
    QObject::connect(server, SIGNAL(newConnection()), SLOT(receiveConnection()));
    return false;
}

// ![2]��һ�����еĳ䵱�����,��lockName��ס,����Listen���м���socketName��������ǿͻ���
bool QtLocalPeer::sendMessage(const QString &message, int timeout)
{
    if (!isClient()){//�ͻ��˷�����Ϣ
        qDebug()<< "server";
        return false;//����˵�����������ڴ˷���,����������ݵķ��ͻ���
    }
    else{
        qDebug()<< "client";
    }
    QLocalSocket socket;
    bool connOk = false;
    for(int i = 0; i < 2; i++) {
        //[1]���ӵ������
        // Try twice, in case the other instance is just starting up�������Σ��Է���һ��ʵ���ո�����
        socket.connectToServer(socketName);//���ӷ����,��Ϊ������Ǽ��������socketName������[������������ӣ�QlocalSocket������ConnectedState������Connected������]
        connOk = socket.waitForConnected(timeout/2);//����������ӷ���true
        //[3-1]�ȴ����ӳɹ�Ӧ��
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
    ds.writeBytes(uMsg.constData(), uMsg.size());//��������uMsg.costData�е�,uMsg.size()���ȵ�����д�뵽���У���������������
    //���ڻ����豸���˺������ȴ�����д�����ݵ���Ч���ر�д���豸������byteswrite()�źţ�
    //���ߵȴ�msecs�����ȥ�����msecs��-1����������Ͳ��ᳬʱ������δ������豸������������//tcp֮�䷢�ͣ����ڻ����豸
    bool res = socket.waitForBytesWritten(timeout);//�ȴ�д���
    if (res) {
    //�飬ֱ�����µ����ݿ���(reading)������readyRead()�źţ�����ֱ������msecs���롣���msecs��-1����������Ͳ��ᳬʱ
        res &= socket.waitForReadyRead(timeout);   // wait for ack
        if (res)//    ^��������ȵȷ���˷���������
        {
           // qDebug()<< socket.read(qstrlen(ack));debug��ȡ��֮��Ͷ�ȡ�����ˣ�����debugΪ���
            res &= (socket.read(qstrlen(ack)) == ack);//server�˵�Ӧ������ack�ַ���
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
    //[2]���ӳɹ�
    //���ؿɹ���ȡ���ֽ������˺���ͨ����˳���豸һ��ʹ�ã���ȷ����ȡ֮ǰ�ڻ������з�����ֽ�����
    while (socket->bytesAvailable() < (int)sizeof(quint32))
        socket->waitForReadyRead();//[3]����client����Ϣѭ��
    qDebug()<< "bytesAvailable";
    //[5]��ȡ����ص����ݣ�Ȼ���������ݴ洢��message������
    QDataStream ds(socket);
    QByteArray uMsg;
    quint32 remaining;//ʣ���
    ds >> remaining;
    uMsg.resize(remaining);//����uMsg�ֽ�����С,����ô��һ��ռ�
    int got = 0;
    char* uMsgBuf = uMsg.data();
    do {//����ȡlen���ֽڵ�uMsgBuf��һ��
        got = ds.readRawData(uMsgBuf, remaining);//���׽��ֵ����ݶ��뵽��������
        remaining -= got;//�п���һ�ζ�ȡ����
        uMsgBuf += got;
    } while (remaining && got >= 0 && socket->waitForReadyRead(2000));//������ݿ�������ȡ����������true;���򷵻�false
    if (got < 0) {
        qWarning("QtLocalPeer: Message reception failed %s", socket->errorString().toLatin1().constData());
        delete socket;
        return;
    }
    QString message(QString::fromUtf8(uMsg));//uMsgBufָ��ָ����ǿ��ٵĻ�����(�����������ݵ�����)��ʼλ��
    //[6]Ӧ��Client��
    socket->write(ack, qstrlen(ack));//����\0��β��8-bit�ַ��е�����д���豸������ʵ��д����ֽ���������������󣬷���-1
    socket->waitForBytesWritten(1000);
    socket->waitForDisconnected(1000); // make sure client reads ackȷ���ͻ��˶�ȡack
    delete socket;
    emit messageReceived(message); //### (might take a long time to return)//������Ҫһ������ʱ����ܷ���
}
//�ܽ᣺1 ��������������������ļ�,����������������������ļ��˱�ɿͻ���
//     2 �ͻ�����Ҫ�����������󣬽���ȴ�serverӦ��ѭ��
//     3 ���ͻ��˵���������connectToServer,\
//       server�����µ����������ź�newConnection,�������Ӳۺ������ж�\
//       next����socket�Ƿ�Ϊ�ա����ձ�ʾ���ӳɹ����������clicent��Ϣѭ��
//     4 �ͻ���������Ϣ��������ͳɹ����ȴ��������server����˵�Ӧ����Ϣѭ��
//     5 ����˽��յ���Ϣ֮�󡣽���Ϣ�洢��message�����С�����Ӧ��client ack�ַ���\
//         ȷ���ͻ����ܹ���ʱ����պʹ�����ʱɾ��socket
//     6 client���յ���Ϣ֮����֤��Ϣ���˳���
//     7 server�����յ�����Ϣ���з��ͷ��͵����������д���
//ע�⣺ȷ���ܹ�����client-server-system��ҪЯ���ļ�
