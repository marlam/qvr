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

#include <QByteArray>
#include <QList>
#include <QVector>
#include <QIODevice>

class QTcpSocket;
class QTcpServer;
class QLocalSocket;
class QLocalServer;
class QSharedMemory;
class QBuffer;

class QVREvent;
class QVRApp;
class QVRDevice;
class QVRObserver;

class QVRSharedMemoryDevice;


/* This implements client/server Inter Process Communication (IPC).
 * These interfaces are only used internally and never exposed to applications.
 *
 * We use IPC either over shared memory or local sockets (if all processes are on
 * the same host, which is assumed to be the case when all processes are configured
 * without a launcher), or over TCP sockets (if at least one process is on a remote
 * host, which is assumed to be the case if it is configured with a launcher).
 */

/* A global timeout value, used whenever a communication function can time out.
 * TODO: this could be made configurable, e.g. via a master process attribute. */
extern int QVRTimeoutMsecs;

/* The client, for slave processes. Based on QVRSharedMemoryDevice/QLocalSocket/QTcpSocket.
 * Unfortunately QLocalSocket is not based on QAbstractSocket... */

typedef enum {
    QVRClientCmdInit,
    QVRClientCmdUpdateDevices,
    QVRClientCmdDevice,
    QVRClientCmdWasdqeState,
    QVRClientCmdObserver,
    QVRClientCmdRender,
    QVRClientCmdQuit,
    QVRClientCmdInvalid
} QVRClientCmd;

class QVRClient
{
private:
    QByteArray _data;
    QTcpSocket* _tcpSocket;
    QLocalSocket* _localSocket;
    QSharedMemory* _sharedMem;
    QVRSharedMemoryDevice* _sharedMemServerDevice;
    QVRSharedMemoryDevice* _sharedMemClientDevice;

    QIODevice* inputDevice();
    QIODevice* outputDevice();

public:
    QVRClient();
    ~QVRClient();

    /* Start a client by connecting to the server. The server name is of
     * the form local,name for a local server, tcp,host,port for a TCP server,
     * and shmem,key for a shared memory server. */
    bool start(const QString& serverName);

    /* Commands that this client sends to the server */
    void sendReplyUpdateDevices(int n, const QByteArray& serializedDevices);
    void sendCmdSync(int n, const QByteArray& serializedEvents);
    /* Explicit flushing of the underlying socket */
    void flush();

    /* Commands that this client receives from the server.
     * First use receiveCmd() to get the next command (if any).
     * Then use one of the remaining functions to read the arguments for that
     * command. */
    bool receiveCmd(QVRClientCmd* cmd, bool waitForIt = false);
    void receiveCmdInitArgs(QVRApp* app);
    void receiveCmdDeviceArgs(QVRDevice* dev);
    void receiveCmdWasdqeStateArgs(int*, int*, bool*);
    void receiveCmdObserverArgs(QVRObserver* obs);
    void receiveCmdRenderArgs(float* n, float* f, QVRApp* app);
};

/* The server, for the master process. Based on QLocalServer/QTcpServer. */

class QVRServer
{
private:
    QByteArray _data;
    QTcpServer* _tcpServer;
    QVector<QTcpSocket*> _tcpSockets;
    QLocalServer* _localServer;
    QVector<QLocalSocket*> _localSockets;
    QSharedMemory* _sharedMem;
    QVector<QVRSharedMemoryDevice*> _sharedMemServerDevices;
    bool _sharedMemHaveCoupledClients;
    QVector<int> _sharedMemServerForClientMap;
    QVector<QVRSharedMemoryDevice*> _sharedMemClientDevices;
    QVector<bool> _clientIsSynced;

    int inputDevices() const;
    QIODevice* inputDevice(int i);

    void sendCmd(const char cmd,
            const QByteArray& data0 = QByteArray(static_cast<const char*>(0), 0),
            const QByteArray& data1 = QByteArray(static_cast<const char*>(0), 0));

public:
    QVRServer();
    ~QVRServer();

    /* Start a server. You must choose to start either a tcp server or a local server
     * or a shared memory server.
     * In case of a tcp server, you can optionally specify an IP address to listen on. */
    bool startTcp(const QString& address = QString());
    bool startLocal();
    bool startSharedMemory();
    /* Return the name of the server. This is either local,name for local servers
     * or tcp,host,port for tcp servers. Pass this name to QVRClient::start(). */
    QString name();
    /* Wait until the given number of clients have connected to this server. */
    bool waitForClients();

    /* Commands that this server sends to all clients. */
    void sendCmdInit(const QByteArray& serializedStatData);
    void sendCmdUpdateDevices();
    void sendCmdDevice(const QByteArray& serializedDevice);
    void sendCmdWasdqeState(const QByteArray& serializedWasdqeState);
    void sendCmdObserver(const QByteArray& serializedObserver);
    void sendCmdRender(float n, float f, const QByteArray& serializedDynData);
    void sendCmdQuit();
    /* Explicit flushing of the underlying sockets */
    void flush();

    /* Replies that this server receives from clients. See sendCmdUpdateDevices(). */
    void receiveReplyUpdateDevices(QList<QVRDevice*> devices);
    /* Commands that this server receives from all clients.
     * This is always a list of zero or more event commands followed by a sync command.
     * The events (if any) will be appended to the given list. */
    void receiveCmdSync(QList<QVREvent>* eventList);
};

#endif
