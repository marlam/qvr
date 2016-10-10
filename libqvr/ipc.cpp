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

#include <QByteArray>
#include <QLocalSocket>
#include <QLocalServer>
#include <QTcpSocket>
#include <QTcpServer>
#include <QHostInfo>

#include "event.hpp"
#include "app.hpp"
#include "device.hpp"
#include "observer.hpp"
#include "logging.hpp"
#include "ipc.hpp"

int QVRTimeoutMsecs = -1; // the default is to never timeout

QVRClient::QVRClient() : _localSocket(NULL), _tcpSocket(NULL)
{
}

QVRClient::~QVRClient()
{
    delete _localSocket;
    delete _tcpSocket;
}

QIODevice* QVRClient::device()
{
    return (_localSocket
            ? dynamic_cast<QIODevice*>(_localSocket)
            : dynamic_cast<QIODevice*>(_tcpSocket));
}

void QVRClient::waitForBytesAvailable(int size)
{
    while (device()->bytesAvailable() < size)
        device()->waitForReadyRead(QVRTimeoutMsecs);
}

bool QVRClient::start(const QString& serverName)
{
    Q_ASSERT(!_localSocket);
    Q_ASSERT(!_tcpSocket);

    QStringList args = serverName.split(',');
    if (args.length() == 2 && args[0] == "local") {
        QLocalSocket* socket = new QLocalSocket;
        socket->connectToServer(args[1]);
        if (!socket->waitForConnected(QVRTimeoutMsecs)) {
            QVR_FATAL("cannot connect to local server %s", qPrintable(args[1]));
            delete socket;
            return false;
        }
        QVR_INFO("connected to local server %s", qPrintable(socket->fullServerName()));
        _localSocket = socket;
    } else if (args.length() == 3 && args[0] == "tcp") {
        int port = args[2].toInt();
        QTcpSocket* socket = new QTcpSocket;
        socket->connectToHost(args[1], port);
        if (!socket->waitForConnected(QVRTimeoutMsecs)) {
            QVR_FATAL("cannot connect to tcp server %s port %s", qPrintable(args[1]), qPrintable(args[2]));
            delete socket;
            return false;
        }
        socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
        QVR_INFO("connected to tcp server %s port %d", qPrintable(socket->peerName()), socket->peerPort());
        _tcpSocket = socket;
    } else {
        QVR_FATAL("invalid server specification %s", qPrintable(serverName));
        return false;
    }
    return true;
}

void QVRClient::sendReplyUpdateDevices(int n, const QByteArray& serializedDevices)
{
    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds << n;
    device()->write(data);
    device()->write(serializedDevices);
}

void QVRClient::sendCmdEvent(const QVREvent* e)
{
    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds << *e;
    device()->putChar('e');
    device()->write(data);
}

void QVRClient::sendCmdSync()
{
    device()->putChar('s');
}

void QVRClient::flush()
{
    if (_localSocket)
        _localSocket->flush();
    else
        _tcpSocket->flush();
}

bool QVRClient::receiveCmd(QVRClientCmd* cmd, bool waitForIt)
{
    if (waitForIt)
        device()->waitForReadyRead(QVRTimeoutMsecs);
    char c;
    bool r = device()->getChar(&c);
    if (r) {
        switch (c) {
        case 'i': *cmd = QVRClientCmdInit; break;
        case 'u': *cmd = QVRClientCmdUpdateDevices; break;
        case 'd': *cmd = QVRClientCmdDevice; break;
        case 'w': *cmd = QVRClientCmdWasdqeState; break;
        case 'o': *cmd = QVRClientCmdObserver; break;
        case 'r': *cmd = QVRClientCmdRender; break;
        case 'q': *cmd = QVRClientCmdQuit; break;
        default:  *cmd = QVRClientCmdInvalid; break;
        }
    }
    return r;
}

void QVRClient::receiveCmdInitArgs(QVRApp* app)
{
    QDataStream ds(device());
    int size;
    ds >> size;
    waitForBytesAvailable(size);
    app->deserializeStaticData(ds);
}

void QVRClient::receiveCmdDeviceArgs(QVRDevice* dev)
{
    QDataStream ds(device());
    ds >> *dev;
}

void QVRClient::receiveCmdWasdqeStateArgs(int* wasdqeMouseProcessIndex, int* wasdqeMouseWindowIndex, bool* wasdqeMouseInitialized)
{
    QDataStream ds(device());
    ds >> *wasdqeMouseProcessIndex >> *wasdqeMouseWindowIndex >> *wasdqeMouseInitialized;
}

void QVRClient::receiveCmdObserverArgs(QVRObserver* obs)
{
    QDataStream ds(device());
    ds >> *obs;
}

void QVRClient::receiveCmdRenderArgs(float* n, float* f, QVRApp* app)
{
    QDataStream ds(device());
    int size;
    ds >> *n >> *f >> size;
    waitForBytesAvailable(size);
    app->deserializeDynamicData(ds);
}

QVRServer::QVRServer() : _localServer(NULL), _tcpServer(NULL)
{
}

QVRServer::~QVRServer()
{
    delete _localServer; // also deletes all local sockets
    delete _tcpServer;   // also deletes all tcp sockets
}

int QVRServer::devices() const
{
    return _localServer ? _localSockets.size() : _tcpSockets.size();
}

QIODevice* QVRServer::device(int i)
{
    return (_localServer
            ? dynamic_cast<QIODevice*>(_localSockets[i])
            : dynamic_cast<QIODevice*>(_tcpSockets[i]));
}

bool QVRServer::startLocal()
{
    QLocalServer* server = new QLocalServer;
    server->setSocketOptions(QLocalServer::UserAccessOption);
    if (!server->listen("qvr")) {
        QVR_FATAL("cannot initialize local server: %s", qPrintable(server->errorString()));
        delete server;
        return false;
    }
    _localServer = server;
    return true;
}

bool QVRServer::startTcp(const QString& address)
{
    QTcpServer* server = new QTcpServer;
    QHostAddress hostAddress;
    if (!address.isEmpty()) {
        if (!hostAddress.setAddress(address)) {
            QVR_FATAL("invalid address specification %s", qPrintable(address));
            return false;
        }
    }
    if (!server->listen(hostAddress)) {
        QVR_FATAL("cannot initialize tcp server on %s: %s",
                qPrintable(address.isEmpty() ? QString("default address") : address),
                qPrintable(server->errorString()));
        delete server;
        return false;
    }
    QVR_INFO("started tcp server on %s port %d",
            qPrintable(server->serverAddress().toString()), server->serverPort());
    _tcpServer = server;
    return true;
}

QString QVRServer::name()
{
    QString s;
    if (_localServer) {
        s = "local,";
        s += _localServer->serverName();
    } else {
        s = "tcp,";
        if (_tcpServer->serverAddress().isNull()
                || _tcpServer->serverAddress().toString() == "0.0.0.0")
            s += QHostInfo::fromName(QHostInfo::localHostName()).hostName();
        else
            s += _tcpServer->serverAddress().toString();
        s += ',';
        s += QString::number(_tcpServer->serverPort());
    }
    return s;
}

bool QVRServer::waitForClients(int clients)
{
    if (_localServer) {
        for (int i = 0; i < clients; i++) {
            if (!_localServer->waitForNewConnection(QVRTimeoutMsecs)) {
                QVR_FATAL("client %d out of %d did not connect", i + 1, clients);
                return false;
            }
            _localSockets.append(_localServer->nextPendingConnection());
        }
    } else {
        for (int i = 0; i < clients; i++) {
            if (!_tcpServer->waitForNewConnection(QVRTimeoutMsecs)) {
                QVR_FATAL("client %d out of %d did not connect", i + 1, clients);
                return false;
            }
            QTcpSocket* socket = _tcpServer->nextPendingConnection();
            socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
            _tcpSockets.append(socket);
        }
    }
    return true;
}

void QVRServer::sendCmd(const char cmd, const QByteArray& data0, const QByteArray& data1)
{
    for (int i = 0; i < devices(); i++) {
        device(i)->putChar(cmd);
        device(i)->write(data0);
        device(i)->write(data1);
    }
}

void QVRServer::sendCmdInit(const QByteArray& serializedStatData)
{
    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds << serializedStatData.size();
    sendCmd('i', data, serializedStatData);
}

void QVRServer::sendCmdUpdateDevices()
{
    sendCmd('u');
}

void QVRServer::sendCmdDevice(const QByteArray& serializedDevice)
{
    sendCmd('d', serializedDevice);
}

void QVRServer::sendCmdWasdqeState(const QByteArray& serializedWasdqeState)
{
    sendCmd('w', serializedWasdqeState);
}

void QVRServer::sendCmdObserver(const QByteArray& serializedObserver)
{
    sendCmd('o', serializedObserver);
}

void QVRServer::sendCmdRender(float n, float f, const QByteArray& serializedDynData)
{
    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds << n << f << serializedDynData.size();
    sendCmd('r', data, serializedDynData);
}

void QVRServer::sendCmdQuit()
{
    sendCmd('q');
}

void QVRServer::flush()
{
    if (_localServer) {
        for (int i = 0; i < _localSockets.size(); i++)
            _localSockets[i]->flush();
    } else {
        for (int i = 0; i < _tcpSockets.size(); i++)
            _tcpSockets[i]->flush();
    }
}

void QVRServer::receiveReplyUpdateDevices(QList<QVRDevice*> deviceList)
{
    for (int i = 0; i < devices(); i++) {
        device(i)->waitForReadyRead(QVRTimeoutMsecs);
        QDataStream ds(device(i));
        QVRDevice dev;
        int n;
        ds >> n;
        for (int j = 0; j < n; j++) {
            ds >> dev;
            *(deviceList.at(dev.index())) = dev;
        }
    }
}

bool QVRServer::receiveCmdsEventAndSync(QList<QVREvent>* eventList)
{
    for (int i = 0; i < devices(); i++) {
        for (;;) {
            // get command from client
            char c;
            bool r = device(i)->getChar(&c);
            if (!r) {
                // no command available: wait for client, then try again
                device(i)->waitForReadyRead(QVRTimeoutMsecs);
                continue;
            } else {
                // we have a command, it must be 'e' or 's'
                if (c == 'e') {
                    // an event; there might be more of these
                    QDataStream ds(device(i));
                    QVREvent e;
                    ds >> e;
                    eventList->append(e);
                    continue;
                } else if (c == 's') {
                    // hooray we're done; on to the next client
                    break;
                } else {
                    QVR_FATAL("unknown command from slave!? %d '%c'", c, c);
                }
            }
            return false;
        }
    }
    return true;
}
