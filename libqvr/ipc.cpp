/*
 * Copyright (C) 2016, 2017 Computer Graphics Group, University of Siegen
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

#include <cstring>

#include <QTcpSocket>
#include <QTcpServer>
#include <QLocalSocket>
#include <QLocalServer>
#include <QSharedMemory>
#include <QBuffer>
#include <QHostInfo>
#include <QUuid>
#include <QThread>
#include <QElapsedTimer>

#include "event.hpp"
#include "app.hpp"
#include "device.hpp"
#include "observer.hpp"
#include "logging.hpp"
#include "ipc.hpp"


int QVRTimeoutMsecs = -1; // the default is to never timeout

/* QVRSharedMemoryDevice
 *
 * This implements a sequential device with one writer and n>=1 readers as a ringbuffer
 * in a fixed memory area.
 *
 * This allows to use QSharedMemory as a QIODevice, which unfortunately is not possible
 * in Qt since you cannot have QByteArray use a fixed memory area *and* modify that memory
 * (i.e. no writing to the shared memory is possible).
 */

class QVRSharedMemoryDevice : public QIODevice {
private:
    int _readers;            // number of readers
    int _reader;             // index of this reader, or -1 if this is a writer
    char* _readerConnected;  // points to first _readers bytes of the buffer that is passed to the constructor
    int* _writePosition;     // points to next 8 bytes of that buffer
    int* _readPositions;     // points to next _readers * 8 bytes of that buffer
    char* _buffer;           // points to the rest of that buffer
    int _size;               // remaining size of that buffer, used for data

    int& writePos() { return *_writePosition; }
    const int& writePos() const { return *_writePosition; }
    int& readPos(int readerIndex) { Q_ASSERT(readerIndex >= 0 && readerIndex < _readers); return _readPositions[readerIndex]; }
    const int& readPos(int readerIndex) const { Q_ASSERT(readerIndex >= 0 && readerIndex < _readers); return _readPositions[readerIndex]; }
    int& readPos() { return readPos(_reader); }
    const int& readPos() const { return readPos(_reader); }

    int bytesAvailable(int wP, int readerIndex) const;
    int bytesAvailableForWriting() const;

protected:
    virtual qint64 readData(char *data, qint64 maxSize);
    virtual qint64 writeData(const char *data, qint64 maxSize);

public:
    QVRSharedMemoryDevice(int readers, char* buffer, int size);
    virtual ~QVRSharedMemoryDevice();
    int readers() { return _readers; }
    virtual bool open(OpenMode /* mode */) { return false; } // you need to use openWriter() or openReader()
    bool openWriter();
    bool openReader(int readerIndex);
    bool waitForReaderConnection(int readerIndex);
    virtual bool isSequential() const { return true; }
    virtual qint64 bytesAvailable() const { return bytesAvailable(writePos(), _reader); }
    virtual bool waitForReadyRead(int msecs);
    virtual bool waitForBytesWritten(int msecs);
};

QVRSharedMemoryDevice::QVRSharedMemoryDevice(int readers, char* buffer, int size) : QIODevice(),
    _readers(readers),
    _reader(-1),
    _readerConnected(buffer),
    _writePosition(reinterpret_cast<int*>(buffer + _readers)),
    _readPositions(_writePosition + 1),
    _buffer(reinterpret_cast<char*>(_readPositions + _readers)),
    _size(size - (_buffer - buffer))
{
    Q_ASSERT(readers >= 1);
    Q_ASSERT(buffer);
    Q_ASSERT(_size > 0);
}

QVRSharedMemoryDevice::~QVRSharedMemoryDevice()
{
}

bool QVRSharedMemoryDevice::openWriter()
{
    writePos() = 0;
    return QIODevice::open(QIODevice::WriteOnly | QIODevice::Unbuffered);
}

bool QVRSharedMemoryDevice::openReader(int readerIndex)
{
    Q_ASSERT(readerIndex >= 0 && readerIndex < _readers);
    _reader = readerIndex;
    _readerConnected[_reader] = 1;
    readPos() = 0;
    return QIODevice::open(QIODevice::ReadOnly | QIODevice::Unbuffered);
}

bool QVRSharedMemoryDevice::waitForReaderConnection(int i)
{
    Q_ASSERT(i >= 0 && i < _readers);
    if (QVRTimeoutMsecs <= 0) {
        while (_readerConnected[i] == 0)
            QThread::msleep(10);
        return true;
    } else {
        if (_readerConnected[i] == 0)
            QThread::msleep(QVRTimeoutMsecs);
        return _readerConnected[i];
    }
}

int QVRSharedMemoryDevice::bytesAvailable(int wP, int readerIndex) const
{
    int rP = readPos(readerIndex);
    if (wP >= rP) {
        return wP - rP;
    } else {
        return (_size - rP) + wP;
    }
}

int QVRSharedMemoryDevice::bytesAvailableForWriting() const
{
    int wP = writePos();
    int maxBytesAvailable = bytesAvailable(wP, 0);
    for (int i = 1; i < _readers; i++) {
        int ba = bytesAvailable(wP, i);
        if (ba > maxBytesAvailable)
            maxBytesAvailable = ba;
    }
    int freeBytes = _size - maxBytesAvailable;
    return freeBytes - 1;
}

bool QVRSharedMemoryDevice::waitForReadyRead(int msecs)
{
    if (msecs == 0) {
        return (bytesAvailable() > 0);
    } else if (msecs < 0) {
        while (bytesAvailable() <= 0)
            QThread::yieldCurrentThread();
        return true;
    } else {
        QElapsedTimer t;
        t.start();
        int bA = bytesAvailable();
        while (bA <= 0 && t.elapsed() < msecs) {
            QThread::yieldCurrentThread();
            bA = bytesAvailable();
        }
        return (bA > 0);
    }
}

bool QVRSharedMemoryDevice::waitForBytesWritten(int msecs)
{
    if (msecs == 0) {
        return (bytesAvailableForWriting() > 0);
    } else if (msecs < 0) {
        while (bytesAvailableForWriting() <= 0)
            QThread::yieldCurrentThread();
        return true;
    } else {
        QElapsedTimer t;
        t.start();
        int bAFW = bytesAvailableForWriting();
        while (bAFW <= 0 && t.elapsed() < msecs) {
            QThread::yieldCurrentThread();
            bAFW = bytesAvailableForWriting();
        }
        return (bAFW > 0);
    }
}

qint64 QVRSharedMemoryDevice::readData(char* data, qint64 maxSize)
{
    if (maxSize <= 0) {
        return 0;
    } else {
        int rP = readPos();
        int wP = writePos();
        qint64 chunkSize = (wP >= rP ? wP : _size) - rP;
        qint64 s = std::min(maxSize, chunkSize);
        std::memcpy(data, _buffer + rP, s);
        if (rP + s < _size) {
            readPos() = rP + s;
        } else {
            if (s < maxSize) {
                qint64 t = std::min(maxSize - s, static_cast<qint64>(writePos()));
                std::memcpy(data + s, _buffer, t);
                readPos() = t;
                s += t;
            } else {
                readPos() = 0;
            }
        }
        return s;
    }
}

qint64 QVRSharedMemoryDevice::writeData(const char* data, qint64 maxSize)
{
    if (maxSize <= 0) {
        return 0;
    } else {
        int maxSizeBuffer = bytesAvailableForWriting();
        if (maxSize > maxSizeBuffer)
            maxSize = maxSizeBuffer;
        int wP = writePos();
        qint64 s = std::min(maxSize, static_cast<qint64>(_size - wP));
        std::memcpy(_buffer + wP, data, s);
        if (wP + s < _size) {
            writePos() = wP + s;
        } else {
            if (s < maxSize) {
                qint64 t = maxSize - s;
                std::memcpy(_buffer, data + s, t);
                writePos() = t;
                s += t;
            } else {
                writePos() = 0;
            }
        }
        return s;
    }
}

/* Internal helper functions that specify how much shared memory is required for
 * inter-process communication, and which area in that shared memory each
 * QVRSharedMemoryDevice uses. */

// The following sizes should avoid read/write synchronization waits in most cases.
// Such waits happen when the size of transferred data exceeds the data buffer size
// of the QVRSharedMemoryDevice. This usually only happens with applications that
// serialize a lot of dynamic data.
static const int QVRSharedMemoryServerDeviceSize = 1024 * 1024; // Shared memory size for server->client device
static const int QVRSharedMemoryClientDeviceSize = 2048; // Shared memory size for client->server device

static void QVRGetSharedMemServerConfigs(int* serverDeviceCount, int* coupledClientCount,
        int* serverIndexForThisProcess, int* coupledClientIndexForThisProcess)
{
    *serverDeviceCount = 0;
    *coupledClientCount = 0;
    *serverIndexForThisProcess = 0;
    *coupledClientIndexForThisProcess = 0;
    for (int p = 1; p < QVRManager::processCount(); p++) {
        if (QVRManager::processConfig(p).decoupledRendering()) {
            (*serverDeviceCount)++;
        } else {
            if (*coupledClientCount == 0)
                (*serverDeviceCount)++;
            if (p == QVRManager::processIndex())
                (*coupledClientIndexForThisProcess) = (*coupledClientCount);
            (*coupledClientCount)++;
        }
    }
    if (QVRManager::processConfig(QVRManager::processIndex()).decoupledRendering()) {
        if (*coupledClientCount > 0)
            *serverIndexForThisProcess = 1;
        for (int p = 1; p < QVRManager::processIndex(); p++)
            if (QVRManager::processConfig(p).decoupledRendering())
                (*serverIndexForThisProcess)++;
    }
}

/* Internal helper functions to read and write a given number of bytes from/to
 * a sequential QIODevice. These functions wait until all data is read or written. */

static bool QVRReadData(QIODevice* device, char* data, int size)
{
    int i = 0;
    int remaining = size;
    while (remaining > 0) {
        int r = device->read(data + i, remaining);
        if (r < 0) {
            return false;
        } else if (r == 0) {
            device->waitForReadyRead(QVRTimeoutMsecs);
        } else {
            i += r;
            remaining -= r;
        }
    }
    return true;
}

static bool QVRReadData(QIODevice* device, QByteArray& array)
{
    int s;
    if (!QVRReadData(device, reinterpret_cast<char*>(&s), sizeof(int)))
        return false;
    array.resize(s);
    return QVRReadData(device, array.data(), s);
}

static bool QVRWriteData(QIODevice* device, const char* data, int size)
{
    int i = 0;
    int remaining = size;
    while (remaining > 0) {
        int r = device->write(data + i, remaining);
        if (r < 0) {
            return false;
        } else if (r == 0) {
            device->waitForBytesWritten(QVRTimeoutMsecs);
        } else {
            i += r;
            remaining -= r;
        }
    }
    return true;
}

static bool QVRWriteData(QIODevice* device, const QByteArray& array)
{
    int s = array.size();
    return (QVRWriteData(device, reinterpret_cast<char*>(&s), sizeof(int))
            && QVRWriteData(device, array.data(), s));
}

/* The QVR client */

QVRClient::QVRClient() :
    _tcpSocket(NULL),
    _localSocket(NULL),
    _sharedMem(NULL),
    _sharedMemServerDevice(NULL),
    _sharedMemClientDevice(NULL)
{
    _data.reserve(QVRSharedMemoryServerDeviceSize);
}

QVRClient::~QVRClient()
{
    delete _tcpSocket;
    delete _localSocket;
    delete _sharedMemServerDevice;
    delete _sharedMemClientDevice;
}

QIODevice* QVRClient::inputDevice()
{
    QIODevice* dev;
    if (_tcpSocket)
        dev = _tcpSocket;
    else if (_localSocket)
        dev = _localSocket;
    else
        dev = _sharedMemServerDevice;
    return dev;
}

QIODevice* QVRClient::outputDevice()
{
    QIODevice* dev;
    if (_tcpSocket)
        dev = _tcpSocket;
    else if (_localSocket)
        dev = _localSocket;
    else
        dev = _sharedMemClientDevice;
    return dev;
}

bool QVRClient::start(const QString& serverName)
{
    Q_ASSERT(!_tcpSocket);
    Q_ASSERT(!_localSocket);
    Q_ASSERT(!_sharedMem);

    QStringList args = serverName.split(',');
    if (args.length() == 3 && args[0] == "tcp") {
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
        int pI = QVRManager::processIndex();
        QVRWriteData(outputDevice(), reinterpret_cast<char*>(&pI), sizeof(pI));
        flush();
    } else if (args.length() == 2 && args[0] == "local") {
        QLocalSocket* socket = new QLocalSocket;
        socket->connectToServer(args[1]);
        if (!socket->waitForConnected(QVRTimeoutMsecs)) {
            QVR_FATAL("cannot connect to local server %s", qPrintable(args[1]));
            delete socket;
            return false;
        }
        QVR_INFO("connected to local server %s", qPrintable(socket->fullServerName()));
        _localSocket = socket;
        int pI = QVRManager::processIndex();
        QVRWriteData(outputDevice(), reinterpret_cast<char*>(&pI), sizeof(pI));
        flush();
    } else if (args.length() == 2 && args[0] == "shmem") {
        QSharedMemory* sharedMem = new QSharedMemory(args[1]);
        if (!sharedMem->attach(QSharedMemory::ReadWrite)) {
            QVR_FATAL("cannot attach to shared memory %s", qPrintable(args[1]));
            delete sharedMem;
            return false;
        }
        QVR_INFO("connected to shared memory %s", qPrintable(args[1]));
        _sharedMem = sharedMem;
        int serverDeviceCount;
        int coupledClientCount;
        int serverIndexForThisProcess;
        int coupledClientIndexForThisProcess;
        QVRGetSharedMemServerConfigs(
                &serverDeviceCount,
                &coupledClientCount,
                &serverIndexForThisProcess,
                &coupledClientIndexForThisProcess);
        _sharedMemServerDevice = new QVRSharedMemoryDevice(
                QVRManager::processConfig().decoupledRendering() ? 1 : coupledClientCount,
                static_cast<char*>(sharedMem->data()) + serverIndexForThisProcess * QVRSharedMemoryServerDeviceSize,
                QVRSharedMemoryServerDeviceSize);
        _sharedMemServerDevice->openReader(QVRManager::processConfig().decoupledRendering() ? 0
                : coupledClientIndexForThisProcess);
        _sharedMemClientDevice = new QVRSharedMemoryDevice(1,
                static_cast<char*>(sharedMem->data()) + serverDeviceCount * QVRSharedMemoryServerDeviceSize
                + (QVRManager::processIndex() - 1) * QVRSharedMemoryClientDeviceSize,
                QVRSharedMemoryClientDeviceSize);
        _sharedMemClientDevice->openWriter();
    } else {
        QVR_FATAL("invalid server specification %s", qPrintable(serverName));
        return false;
    }
    return true;
}

void QVRClient::sendReplyUpdateDevices(int n, const QByteArray& serializedDevices)
{
    QVRWriteData(outputDevice(), reinterpret_cast<char*>(&n), sizeof(n));
    QVRWriteData(outputDevice(), serializedDevices);
}

void QVRClient::sendCmdSync(int n, const QByteArray& serializedEvents)
{
    QVRWriteData(outputDevice(), reinterpret_cast<char*>(&n), sizeof(n));
    QVRWriteData(outputDevice(), serializedEvents);
}

void QVRClient::flush()
{
    if (_tcpSocket)
        _tcpSocket->flush();
    else if (_localSocket)
        _localSocket->flush();
}

bool QVRClient::receiveCmd(QVRClientCmd* cmd, bool waitForIt)
{
    if (waitForIt && inputDevice()->bytesAvailable() == 0)
        inputDevice()->waitForReadyRead(QVRTimeoutMsecs);
    char c;
    bool r = inputDevice()->getChar(&c);
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
    QVRReadData(inputDevice(), _data);
    QDataStream ds(_data);
    app->deserializeStaticData(ds);
}

void QVRClient::receiveCmdDeviceArgs(QVRDevice* dev)
{
    QVRReadData(inputDevice(), _data);
    QDataStream ds(_data);
    ds >> *dev;
}

void QVRClient::receiveCmdWasdqeStateArgs(int* wasdqeMouseProcessIndex, int* wasdqeMouseWindowIndex, bool* wasdqeMouseInitialized)
{
    QVRReadData(inputDevice(), _data);
    QDataStream ds(_data);
    ds >> *wasdqeMouseProcessIndex >> *wasdqeMouseWindowIndex >> *wasdqeMouseInitialized;
}

void QVRClient::receiveCmdObserverArgs(QVRObserver* obs)
{
    QVRReadData(inputDevice(), _data);
    QDataStream ds(_data);
    ds >> *obs;
}

void QVRClient::receiveCmdRenderArgs(float* n, float* f, QVRApp* app)
{
    QVRReadData(inputDevice(), _data);
    Q_ASSERT(_data.size() == 2 * sizeof(float));
    std::memcpy(n, _data.data(), sizeof(float));
    std::memcpy(f, _data.data() + sizeof(float), sizeof(float));
    QVRReadData(inputDevice(), _data);
    QDataStream ds(_data);
    app->deserializeDynamicData(ds);
}

/* The QVR Server */

QVRServer::QVRServer() :
    _tcpServer(NULL),
    _localServer(NULL),
    _sharedMem(NULL)
{
    _data.reserve(QVRSharedMemoryClientDeviceSize);
}

QVRServer::~QVRServer()
{
    delete _tcpServer;   // also deletes all tcp sockets
    delete _localServer; // also deletes all local sockets
    for (int i = 0; i < _sharedMemServerDevices.size(); i++)
        delete _sharedMemServerDevices[i];
    for (int i = 0; i < _sharedMemClientDevices.size(); i++)
        delete _sharedMemClientDevices[i];
    delete _sharedMem;
}

int QVRServer::inputDevices() const
{
    return _clientIsSynced.length();
}

QIODevice* QVRServer::inputDevice(int i)
{
    QIODevice* dev;
    if (_tcpServer)
        dev = _tcpSockets[i];
    else if (_localServer)
        dev = _localSockets[i];
    else
        dev = _sharedMemClientDevices[i];
    return dev;
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

bool QVRServer::startLocal()
{
    QString name = QString("qvr-") + QUuid::createUuid().toString().mid(1, 36);
    QLocalServer* server = new QLocalServer;
    server->setSocketOptions(QLocalServer::UserAccessOption);
    if (!server->listen(name)) {
        QVR_FATAL("cannot initialize local server: %s", qPrintable(server->errorString()));
        delete server;
        return false;
    }
    _localServer = server;
    return true;
}

bool QVRServer::startSharedMemory()
{
    int clientCount = QVRManager::processCount() - 1;
    int serverDeviceCount;
    int coupledClientCount;
    int serverIndexForThisProcess;
    int coupledClientIndexForThisProcess;
    QVRGetSharedMemServerConfigs(
            &serverDeviceCount,
            &coupledClientCount,
            &serverIndexForThisProcess,
            &coupledClientIndexForThisProcess);

    QString name = QUuid::createUuid().toString().mid(1, 36);
    QSharedMemory* sharedMemory = new QSharedMemory(name);
    bool r = sharedMemory->create(serverDeviceCount * QVRSharedMemoryServerDeviceSize
            + clientCount * QVRSharedMemoryClientDeviceSize);
    if (!r) {
        QVR_FATAL("cannot initialize shared memory: %s", qPrintable(sharedMemory->errorString()));
        delete sharedMemory;
        return false;
    }
    _sharedMem = sharedMemory;

    // create server devices: one for all coupled clients (if any), and one for each decoupled client
    _sharedMemHaveCoupledClients = (coupledClientCount > 0);
    if (coupledClientCount > 0) {
        _sharedMemServerDevices.append(new QVRSharedMemoryDevice(coupledClientCount,
                    static_cast<char*>(_sharedMem->data()), QVRSharedMemoryServerDeviceSize));
        _sharedMemServerDevices.last()->openWriter();
    }
    _sharedMemServerForClientMap.resize(clientCount);
    int decoupledProcessServerIndex = (_sharedMemHaveCoupledClients ? 1 : 0);
    for (int p = 1; p < QVRManager::processCount(); p++) {
        if (QVRManager::processConfig(p).decoupledRendering()) {
            _sharedMemServerDevices.append(new QVRSharedMemoryDevice(1, static_cast<char*>(_sharedMem->data())
                        + _sharedMemServerDevices.length() * QVRSharedMemoryServerDeviceSize,
                        QVRSharedMemoryServerDeviceSize));
            _sharedMemServerDevices.last()->openWriter();
            _sharedMemServerForClientMap[p - 1] = decoupledProcessServerIndex++;
        } else {
            _sharedMemServerForClientMap[p - 1] = 0;
        }
    }
    // create client devices
    for (int p = 1; p < QVRManager::processCount(); p++) {
        _sharedMemClientDevices.append(new QVRSharedMemoryDevice(1, static_cast<char*>(_sharedMem->data())
                    + serverDeviceCount * QVRSharedMemoryServerDeviceSize
                    + (p - 1) * QVRSharedMemoryClientDeviceSize,
                    QVRSharedMemoryClientDeviceSize));
        _sharedMemClientDevices.last()->openReader(0);
    }

    return true;
}

QString QVRServer::name()
{
    QString s;
    if (_tcpServer) {
        s = "tcp,";
        if (_tcpServer->serverAddress().isNull()
                || _tcpServer->serverAddress().toString() == "0.0.0.0")
            s += QHostInfo::fromName(QHostInfo::localHostName()).hostName();
        else
            s += _tcpServer->serverAddress().toString();
        s += ',';
        s += QString::number(_tcpServer->serverPort());
    } else if (_localServer) {
        s = "local,";
        s += _localServer->serverName();
    } else {
        s = "shmem,";
        s += _sharedMem->key();
    }
    return s;
}

bool QVRServer::waitForClients()
{
    int clientCount = QVRManager::processCount() - 1;
    if (_tcpServer) {
        _tcpSockets.resize(clientCount);
        for (int i = 0; i < clientCount; i++) {
            QTcpSocket* socket = _tcpServer->nextPendingConnection();
            if (!socket) {
                if (!_tcpServer->waitForNewConnection(QVRTimeoutMsecs)) {
                    QVR_FATAL("client did not connect");
                    return false;
                }
                socket = _tcpServer->nextPendingConnection();
                Q_ASSERT(socket);
            }
            socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
            int clientProcessIndex;
            QVRReadData(socket, reinterpret_cast<char*>(&clientProcessIndex), sizeof(int));
            if (clientProcessIndex < 1 || clientProcessIndex >= QVRManager::processCount()) {
                QVR_FATAL("client sent invalid process index");
                delete socket;
                return false;
            }
            QVR_DEBUG("client with process index %d connected", clientProcessIndex);
            _tcpSockets[clientProcessIndex - 1] = socket;
        }
    } else if (_localServer) {
        _localSockets.resize(clientCount);
        for (int i = 0; i < clientCount; i++) {
            QLocalSocket* socket = _localServer->nextPendingConnection();
            if (!socket) {
                if (!_localServer->waitForNewConnection(QVRTimeoutMsecs)) {
                    QVR_FATAL("client did not connect");
                    return false;
                }
                socket = _localServer->nextPendingConnection();
                Q_ASSERT(socket);
            }
            int clientProcessIndex;
            QVRReadData(socket, reinterpret_cast<char*>(&clientProcessIndex), sizeof(int));
            if (clientProcessIndex < 1 || clientProcessIndex >= QVRManager::processCount()) {
                QVR_FATAL("client sent invalid process index");
                delete socket;
                return false;
            }
            QVR_DEBUG("client with process index %d connected", clientProcessIndex);
            _localSockets[clientProcessIndex - 1] = socket;
        }
    } else {
        for (int d = 0; d < _sharedMemServerDevices.length(); d++) {
            for (int i = 0; i < _sharedMemServerDevices[d]->readers(); i++) {
                if (!_sharedMemServerDevices[d]->waitForReaderConnection(i)) {
                    QVR_FATAL("client did not connect");
                    return false;
                }
            }
        }
    }
    _clientIsSynced.resize(clientCount);
    for (int i = 0; i < clientCount; i++)
        _clientIsSynced[i] = true;
    return true;
}

void QVRServer::sendCmd(const char cmd, const QByteArray& data0, const QByteArray& data1)
{
    bool wroteToCoupledServerDevice = false;
    for (int i = 0; i < inputDevices(); i++) {
        if (_clientIsSynced[i]) {
            bool doWrite = true;
            QIODevice* dev;
            if (_tcpServer) {
                dev = _tcpSockets[i];
            } else if (_localServer) {
                dev = _localSockets[i];
            } else {
                dev = _sharedMemServerDevices[_sharedMemServerForClientMap[i]];
                if (_sharedMemServerForClientMap[i] == 0 && _sharedMemHaveCoupledClients) {
                    if (wroteToCoupledServerDevice) {
                        doWrite = false;
                    } else {
                        wroteToCoupledServerDevice = true;
                    }
                }
            }
            if (doWrite) {
                QVRWriteData(dev, &cmd, sizeof(char));
                if (!data0.isNull())
                    QVRWriteData(dev, data0);
                if (!data1.isNull())
                    QVRWriteData(dev, data1);
            }
        }
    }
}

void QVRServer::sendCmdInit(const QByteArray& serializedStatData)
{
    sendCmd('i', serializedStatData);
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
    float data[2] = { n, f };
    sendCmd('r', QByteArray::fromRawData(reinterpret_cast<char*>(data), sizeof(data)), serializedDynData);
    for (int i = 0; i < _clientIsSynced.length(); i++) {
        if (QVRManager::processConfig(i + 1).decoupledRendering()) {
            _clientIsSynced[i] = false;
        }
    }
}

void QVRServer::sendCmdQuit()
{
    for (int i = 0; i < _clientIsSynced.length(); i++)
        _clientIsSynced[i] = true;
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
    for (int i = 0; i < inputDevices(); i++) {
        if (_clientIsSynced[i]) {
            int n;
            QVRReadData(inputDevice(i), reinterpret_cast<char*>(&n), sizeof(int));
            QVRReadData(inputDevice(i), _data);
            QDataStream ds(_data);
            QVRDevice dev;
            for (int j = 0; j < n; j++) {
                ds >> dev;
                *(deviceList.at(dev.index())) = dev;
            }
        }
    }
}

static void QVRServerReceiveCmdSyncHelper(QIODevice* device, QByteArray& data, QList<QVREvent>* eventList)
{
    int n;
    QVRReadData(device, reinterpret_cast<char*>(&n), sizeof(int));
    QVRReadData(device, data);
    QDataStream ds(data);
    QVREvent e;
    for (int j = 0; j < n; j++) {
        ds >> e;
        eventList->append(e);
    }
}

void QVRServer::receiveCmdSync(QList<QVREvent>* eventList)
{
    // We make two passes over the input devices: first we wait
    // for all coupled devices, then we check if decoupled devices
    // are ready. This avoids an order-dependency of slave process
    // definitions in the configuration.
    for (int i = 0; i < inputDevices(); i++) {
        if (_clientIsSynced[i]) { // true at this point only for coupled processes
            QVRServerReceiveCmdSyncHelper(inputDevice(i), _data, eventList);
        }
    }
    for (int i = 0; i < inputDevices(); i++) {
        if (!_clientIsSynced[i] && inputDevice(i)->bytesAvailable() > 0) {
            QVRServerReceiveCmdSyncHelper(inputDevice(i), _data, eventList);
            _clientIsSynced[i] = true;
        }
    }
}
