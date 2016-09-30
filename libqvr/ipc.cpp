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


QVRClient::QVRClient() : _localSocket(NULL), _tcpSocket(NULL)
{
}

QVRClient::~QVRClient()
{
    delete _localSocket;
    delete _tcpSocket;
}

bool QVRClient::init(const QString& serverName, int timeoutMsecs)
{
    Q_ASSERT(!_localSocket);
    Q_ASSERT(!_tcpSocket);

    QStringList args = serverName.split(',');
    if (args.length() == 2 && args[0] == "local") {
        QLocalSocket* socket = new QLocalSocket;
        socket->connectToServer(args[1]);
        if (!socket->waitForConnected(timeoutMsecs)) {
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
        if (!socket->waitForConnected(timeoutMsecs)) {
            QVR_FATAL("cannot connect to tcp server %s port %s", qPrintable(args[1]), qPrintable(args[2]));
            delete socket;
            return false;
        }
        socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
        QVR_INFO("connected to tcp server %s port %d", qPrintable(socket->peerName()), socket->peerPort());
        _tcpSocket = socket;
    } else {
        QVR_FATAL("cannot connect to invalid server %s", qPrintable(serverName));
        return false;
    }
    return true;
}

void QVRClient::sendCmdEvent(const QVREvent* e)
{
    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds << *e;
    if (_localSocket) {
        _localSocket->putChar('e');
        _localSocket->write(data);
    } else {
        _tcpSocket->putChar('e');
        _tcpSocket->write(data);
    }
}

void QVRClient::sendCmdSync()
{
    if (_localSocket)
        _localSocket->putChar('s');
    else
        _tcpSocket->putChar('s');
}

bool QVRClient::receiveCmd(char* cmd, bool waitForIt)
{
    if (_localSocket) {
        if (waitForIt)
            _localSocket->waitForReadyRead();
        return _localSocket->getChar(cmd);
    } else {
        if (waitForIt)
            _tcpSocket->waitForReadyRead();
        return _tcpSocket->getChar(cmd);
    }
}

void QVRClient::receiveCmdInit(QVRApp* app)
{
    QDataStream ds;
    if (_localSocket)
        ds.setDevice(_localSocket);
    else
        ds.setDevice(_tcpSocket);
    app->deserializeStaticData(ds);
}

void QVRClient::receiveCmdDevice(QVRDevice* dev)
{
    QDataStream ds;
    if (_localSocket)
        ds.setDevice(_localSocket);
    else
        ds.setDevice(_tcpSocket);
    ds >> *dev;
}

void QVRClient::receiveCmdWasdqeState(int* wasdqeMouseProcessIndex, int* wasdqeMouseWindowIndex, bool* wasdqeMouseInitialized)
{
    QDataStream ds;
    if (_localSocket)
        ds.setDevice(_localSocket);
    else
        ds.setDevice(_tcpSocket);
    ds >> *wasdqeMouseProcessIndex >> *wasdqeMouseWindowIndex >> *wasdqeMouseInitialized;
}

void QVRClient::receiveCmdObserver(QVRObserver* obs)
{
    QDataStream ds;
    if (_localSocket)
        ds.setDevice(_localSocket);
    else
        ds.setDevice(_tcpSocket);
    ds >> *obs;
}

void QVRClient::receiveCmdRender(float* n, float* f, QVRApp* app)
{
    QDataStream ds;
    if (_localSocket)
        ds.setDevice(_localSocket);
    else
        ds.setDevice(_tcpSocket);
    ds >> *n >> *f;
    app->deserializeDynamicData(ds);
}

void QVRClient::flush()
{
    if (_localSocket)
        _localSocket->flush();
    else
        _tcpSocket->flush();
}

QVRServer::QVRServer() : _localServer(NULL), _tcpServer(NULL)
{
}

QVRServer::~QVRServer()
{
    for (int i = 0; i < _localSockets.length(); i++)
        delete _localSockets[i];
    delete _localServer;
    for (int i = 0; i < _tcpSockets.length(); i++)
        delete _tcpSockets[i];
    delete _tcpServer;
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

bool QVRServer::waitForClients(int clients, int timeoutMsecs)
{
    if (_localServer) {
        for (int i = 0; i < clients; i++) {
            if (!_localServer->waitForNewConnection(timeoutMsecs)) {
                QVR_FATAL("client %d out of %d did not connect", i + 1, clients);
                return false;
            }
            _localSockets.append(_localServer->nextPendingConnection());
        }
    } else {
        for (int i = 0; i < clients; i++) {
            if (!_tcpServer->waitForNewConnection(timeoutMsecs)) {
                QVR_FATAL("client %d out of %d did not connect", i + 1, clients);
                return false;
            }
            _tcpSockets.append(_tcpServer->nextPendingConnection());
        }
    }
    return true;
}

void QVRServer::sendCmd(const char cmd, const QByteArray& data)
{
    if (_localServer) {
        for (int i = 0; i < _localSockets.size(); i++) {
            _localSockets[i]->putChar(cmd);
            _localSockets[i]->write(data);
        }
    } else {
        for (int i = 0; i < _tcpSockets.size(); i++) {
            _tcpSockets[i]->putChar(cmd);
            _tcpSockets[i]->write(data);
        }
    }
}

void QVRServer::sendCmdInit(const QByteArray& serializedStatData)
{
    sendCmd('i', serializedStatData);
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
    ds << n << f;
    data.append(serializedDynData);
    sendCmd('r', data);
}

void QVRServer::sendCmdQuit()
{
    sendCmd('q', QByteArray());
}

bool QVRServer::receiveCmdSync(QList<QVREvent>* eventList)
{
    int n = (_localServer ? _localSockets.size() : _tcpSockets.size());
    for (int i = 0; i < n; i++) {
        for (;;) {
            char c = '\0';
            bool r;
            // get command from client
            if (_localServer)
                r = _localSockets[i]->getChar(&c);
            else
                r = _tcpSockets[i]->getChar(&c);
            if (!r) {
                // no command available: wait for client, then try again
                if (_localServer)
                    _localSockets[i]->waitForReadyRead();
                else
                    _tcpSockets[i]->waitForReadyRead();
                continue;
            } else {
                // we have a command, it must be 'e' or 's'
                if (c == 'e') {
                    // an event; there might be more of these
                    QDataStream ds;
                    if (_localServer)
                        ds.setDevice(_localSockets[i]);
                    else
                        ds.setDevice(_tcpSockets[i]);
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
