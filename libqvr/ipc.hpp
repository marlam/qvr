/*
 * Copyright (C) 2016 Computer Graphics Group, University of Siegen
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef QVR_IPC_HPP
#define QVR_IPC_HPP

#include <QList>

class QLocalSocket;
class QLocalServer;
class QTcpSocket;
class QTcpServer;

class QVREvent;
class QVRApp;
class QVRDevice;
class QVRObserver;
class QByteArray;


/* This implements client/server Inter Process Communication (IPC).
 * These interfaces are only used internally and never exposed to applications.
 *
 * We use IPC either over local sockets (if all processes are on the same host,
 * which is assumed to be the case when all processes are configured without
 * a launcher), or over TCP sockets (if at least one process is on a remote host,
 * which is assumed to be the case if it is configured with a launcher).
 *
 * TODO: IPC implementation is currently not cleanly separated from QVRManager.
 * This should be improved.
 */

/* The client, for slave processes. Based on QLocalSocket/QTcpSocket. */
class QVRClient
{
private:
    QLocalSocket* _localSocket;
    QTcpSocket* _tcpSocket;

public:
    QVRClient();
    ~QVRClient();

    bool init(const QString& serverName, int timeoutMsecs = 10000); // tcp,name,port or local,name

    // communication functions to be called by the slave to apply to the master
    void sendCmdEvent(const QVREvent* e);               // starts with 'e'
    void sendCmdSync();                                 // is 's'
    bool receiveCmd(char* cmd, bool waitForIt = false); // will only get the first character
    void receiveCmdInit(QVRApp* app);
    void receiveCmdDevice(QVRDevice* dev);
    void receiveCmdWasdqeState(int*, int*, bool*);
    void receiveCmdObserver(QVRObserver* obs);
    void receiveCmdRender(float* n, float* f, QVRApp* app);
    // explicit flushing
    void flush();
};

/* The server, for the master process. Based on QLocalServer/QTcpServer. */
class QVRServer
{
private:
    QLocalServer* _localServer;
    QList<QLocalSocket*> _localSockets;
    QTcpServer* _tcpServer;
    QList<QTcpSocket*> _tcpSockets;

    void sendCmd(const char cmd, const QByteArray& data);

public:
    QVRServer();
    ~QVRServer();

    bool startLocal(); // starts listening on a local socket
    bool startTcp(const QString& address = QString()); // starts listening on a TCP socket
    QString name(); // tcp,name,port or local,name
    bool waitForClients(int clients, int timeoutMsecs = 10000); // wait for all clients to connect (fills _sockets)

    // communication functions to be called by the master to apply to slave processes
    void sendCmdInit(const QByteArray& serializedStatData);                         // starts with 'i'
    void sendCmdDevice(const QByteArray& serializedDevice);                         // starts with 'd'
    void sendCmdWasdqeState(const QByteArray& serializedWasdqeState);               // starts with 'w'
    void sendCmdObserver(const QByteArray& serializedObserver);                     // starts with 'o'
    void sendCmdRender(float n, float f, const QByteArray& serializedDynData);      // starts with 'r'
    void sendCmdQuit();                                                             // is 'q'
    bool receiveCmdSync(QList<QVREvent>* eventList);
    // explicit flushing
    void flush();
};

#endif
