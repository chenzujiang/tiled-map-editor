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


#include "qtsingleapplication.h"
#include "qtlocalpeer.h"
#include <QWidget>
/*!
    \class QtSingleApplication qtsingleapplication.h
    \brief QtSingleApplication类提供了一个API来检测并且与正在运行的应用程序实例通信。

    这个类允许您创建 一次 只能运行一个实例的应用程序。如果用户尝试着启动其他的实例,已经
    运行的实例将被激活(替代),：另一个用例是客户机-服务器系统.第一个启动的实例将承担server的角色，
    后面的实例将充当该服务器的客户机。
    This class allows you to create applications where only one
    instance should be running at a time. I.e., if the user tries to
    launch another instance, the already running instance will be
    activated instead. Another usecase is a client-server system,
    where the first started instance will assume the role of server,
    and the later instances will act as clients of that server.
    默认情况下，可执行文件的完整路径用于确定两个程序是否是相同的实例应用程序。
    您还可以提供一个显式的标识符字符串，以便进行比较。
    By default, the full path of the executable file is used to
    determine whether two processes are instances of the same
    application.
    You can also provide an explicit identifier string that will be compared instead.
    在启动阶段，应用程序应该尽早创建QtSingleApplication对象.调用isRuning()
    函数来判断是否有这个应用程序已经被运行。如果函数返回false,表明没有其他的应用
    程序在运行.该实例假定角色为正在运行的实例.这种情况下这个应用程序将继续初始化
    在使用exec()正常进入事件循环之前的应用程序用户界面。
    The application should create the QtSingleApplication object early
    in the startup phase, and call isRunning() to find out if another
    instance of this application is already running. If isRunning()
    returns false, it means that no other instance is running, and
    this instance has assumed the role as the running instance. In
    this case, the application should continue with the initialization
    of the application user interface before entering the event loop with exec(), as normal.
    当接收到来自于其他相同应用程序发来的消息时,messageReceived()信号将被发出。
    当接收到一条消息时，可能有助于用户启动应用程序，让它变得可见。为了实现这一点，
    QtSingleApplication提供了setActivationWindow()函数和activateWindow()槽。
    The messageReceived() signal will be emitted when the running
    application receives messages from another instance of the same
    application. When a message is received it might be helpful to the
    user to raise the application so that it becomes visible. To
    facilitate this, QtSingleApplication provides the
    setActivationWindow() function and the activateWindow() slot.
    如果 isRunning()返回true,其他实例已经被运行。它可能会被另一个实例发出警告
    通过启动使用sendMessage().此外，诸如启动参数之类的数据(例如，用户希望打开
    这个新实例的文件的名称)也可以通过这个函数传递给正在运行的实例。然后，应用程序应该终止(或进入客户机模式)。
    If isRunning() returns true, another instance is already
    running. It may be alerted to the fact that another instance has
    started by using the sendMessage() function. Also data such as
    startup parameters (e.g. the name of the file the user wanted this
    new instance to open) can be passed to the running instance with
    this function. Then, the application should terminate (or enter
    client mode).
    如果isRunning()返回true，但sendMessage()失败，则为指示正在运行的实例已冻结。
    If isRunning() returns true, but sendMessage() fails, that is an
    indication that the running instance is frozen.
    下面是一个示例，演示如何使用QtSingleApplication转换现有的应用程序
    它非常简单，确实如此不要使用QtSingleApplication的所有功能(参见例子)
    Here's an example that shows how to convert an existing
    application to use QtSingleApplication. It is very simple and does
    not make use of all QtSingleApplication's functionality (see the
    examples for that).

    \code
    // Original
    int main(int argc, char **argv)
    {
        QApplication app(argc, argv);

        MyMainWidget mmw;
        mmw.show();
        return app.exec();
    }

    // Single instance
    int main(int argc, char **argv)
    {
        QtSingleApplication app(argc, argv);

        if (app.isRunning())
            return !app.sendMessage(someDataString);

        MyMainWidget mmw;
        app.setActivationWindow(&mmw);
        mmw.show();
        return app.exec();
    }
    \endcode
    一旦这个应用程序销毁(通常当处理退出或者奔溃)
    当用户下一次尝试运行应用程序时，当然不会遇到此实例。下一个调用isRunning()
    或sendMessage()的实例将充当新的运行实例。
    Once this QtSingleApplication instance is destroyed (normally when
    the process exits or crashes), when the user next attempts to run the
    application this instance will not, of course, be encountered. The
    next instance to call isRunning() or sendMessage() will assume the
    role as the new running instance.
    对于控制台(非gui)应用程序，QtSingleCoreApplication可能代替这个类使用，以避免对QtGui库的依赖
    For console (non-GUI) applications, QtSingleCoreApplication may be
    used instead of this class, to avoid the dependency on the QtGui
    library.

    \sa QtSingleCoreApplication
*/


void QtSingleApplication::sysInit(const QString &appId)
{
    actWin = 0;
    peer = new QtLocalPeer(this, appId);
    connect(peer, SIGNAL(messageReceived(const QString&)), SIGNAL(messageReceived(const QString&)));//本地就可以不需要加this调用相应的信号函数
}


/*!
    Creates a QtSingleApplication object. The application identifier
    will be QCoreApplication::applicationFilePath(). \a argc, \a
    argv, and \a GUIenabled are passed on to the QAppliation constructor.

    If you are creating a console application (i.e. setting \a
    GUIenabled to false), you may consider using
    QtSingleCoreApplication instead.
 * 创建一个QtSingleApplication对象,应用程序的标识符将是QCoreApplication::applicationFilePath().
 * argc,argv,GUIenable传递给QAppliation的构造函数。
 * 如果你创建了控制台应用程序,(例如:设置了GUIenable为false),你可以考虑使用QtSingleCoreApplication来代替
 *
*/

QtSingleApplication::QtSingleApplication(int &argc, char **argv, bool GUIenabled)
    : QApplication(argc, argv, GUIenabled)
{
    sysInit();
}


/*!
    Creates a QtSingleApplication object with the application
    identifier \a appId. \a argc and \a argv are passed on to the
    QAppliation constructor.
*/

QtSingleApplication::QtSingleApplication(const QString &appId, int &argc, char **argv)
    : QApplication(argc, argv)
{
    sysInit(appId);
}

/*!
    Returns true if another instance of this application is running;
    otherwise false.
    此函数不查找由其他用户运行的应用程序实例(在Windows上:在另一个会话中运行)。
    This function does not find instances of this application that are
    being run by a different user (on Windows: that are running in
    another session).

    \sa sendMessage()
*/

bool QtSingleApplication::isRunning()
{
    return peer->isClient();
}


/*!
    Tries to send the text \a message to the currently running
    instance. The QtSingleApplication object in the running instance
    will emit the messageReceived() signal when it receives the
    message.
    如果消息已发送到当前实例并由其处理，则此函数返回true。
    如果当前没有正在运行的实例，或者正在运行的实例未能在超时毫秒内处理消息，
    则此函数返回false。
    This function returns true if the message has been sent to, and
    processed by, the current instance. If there is no instance
    currently running, or if the running instance fails to process the
    message within \a timeout milliseconds, this function return false.

    \sa isRunning(), messageReceived()
*/
bool QtSingleApplication::sendMessage(const QString &message, int timeout)
{
    return peer->sendMessage(message, timeout);
}


/*!
    Returns the application identifier. Two processes with the same
    identifier will be regarded as instances of the same application.
*/
QString QtSingleApplication::id() const
{
    return peer->applicationId();
}


/*!
  Sets the activation window of this application to \a aw.
  The activation window is the widget that will be activated by activateWindow().
  This is typically the application's main window.
    将此应用程序的激活窗口设置为\a aw。
    活跃的窗口是一个widget,他将通过activeateWindow()函数激活
    这通常是应用程序的主窗口。
  If \a activateOnMessage is true (the default), the window will be
  activated automatically every time a message is received,
  just prior to the messageReceived() signal being emitted.
   就在发出messageReceived()信号之前。 如果\a activateOnMessage为true(默认值)，则窗口每次收到消息时自动激活
  \sa activateWindow(), messageReceived()
*/

void QtSingleApplication::setActivationWindow(QWidget* aw, bool activateOnMessage)
{
    actWin = aw;
    if (activateOnMessage)
        connect(peer, SIGNAL(messageReceived(const QString&)), this, SLOT(activateWindow()));
    else
        disconnect(peer, SIGNAL(messageReceived(const QString&)), this, SLOT(activateWindow()));
}


/*!如果通过调用setActivationWindow()设置了应用程序激活窗口，则返回应用程序激活窗口，否则返回0。
    Returns the applications activation window if one has been set by calling setActivationWindow(), otherwise returns 0.

    \sa setActivationWindow()
*/
QWidget* QtSingleApplication::activationWindow() const
{
    return actWin;
}


/*!还原、提升和激活此应用程序的自动码窗口。
 * 如果没有设置激活窗口，此函数将不执行任何操作。
 * 这是一个方便的函数，用于在用户尝试启动另一个实例时显示此应用程序实例已被激活。
 * 此函数通常应在响应messageReceived()信号时调用。默认情况下，如果设置了激活窗口，则会自动执行。
  De-minimizes, raises, and activates this application's activation window.
  This function does nothing if no activation window has been set.

  This is a convenience function to show the user that this
  application instance has been activated when he has tried to start
  another instance.

  This function should typically be called in response to the
  messageReceived() signal. By default, that will happen
  automatically, if an activation window has been set.

  \sa setActivationWindow(), messageReceived(), initialize()
*/
void QtSingleApplication::activateWindow()
{
    if (actWin) {
        actWin->setWindowState(actWin->windowState() & ~Qt::WindowMinimized);
        actWin->raise();//将此小部件提升到父小部件堆栈的顶部。
                        //调用之后，小部件将在任何重叠的兄弟部件前面显示。
        actWin->activateWindow();
    }
}


/*!
    \fn void QtSingleApplication::messageReceived(const QString& message)
    当前应用程序接收到来自于其他应用程序的消息时,messageReceived()信号被发出来
    This signal is emitted when the current instance receives a \a
    message from another instance of this application.

    \sa sendMessage(), setActivationWindow(), activateWindow()
*/


/*!
    \fn void QtSingleApplication::initialize(bool dummy = true)

    \obsolete
*/
