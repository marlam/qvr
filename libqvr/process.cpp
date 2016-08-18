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

#include <QCoreApplication>
#include <QDir>

#include "manager.hpp"
#include "process.hpp"
#include "app.hpp"
#include "device.hpp"
#include "observer.hpp"
#include "event.hpp"
#include "logging.hpp"

QVRProcess::QVRProcess(int index) :
    _index(index), _stdin(NULL), _stdout(NULL)
{
    setProcessChannelMode(QProcess::ForwardedErrorChannel);
    if (_index > 0) {
        // this is a slave process; it talks via stdin/stdout with the master
        _stdin = new QFile;
        _stdin->open(stdin, QIODevice::ReadOnly, QFile::DontCloseHandle);
        _stdout = new QFile;
        _stdout->open(stdout, QIODevice::WriteOnly, QFile::DontCloseHandle);
    }
}

QVRProcess::~QVRProcess()
{
    delete _stdin;
    delete _stdout;
}

int QVRProcess::index() const
{
    return _index;
}

const QString& QVRProcess::id() const
{
    return config().id();
}

const QVRProcessConfig& QVRProcess::config() const
{
    return QVRManager::config().processConfigs().at(index());
}

const QString& QVRProcess::windowId(int windowIndex) const
{
    return windowConfig(windowIndex).id();
}

const QVRWindowConfig& QVRProcess::windowConfig(int windowIndex) const
{
    return config().windowConfigs().at(windowIndex);
}

bool QVRProcess::launch(const QString& configFilename, QVRLogLevel logLevel, int processIndex,
        bool syncToVblank, const QStringList& appArgs)
{
    QString prg = QCoreApplication::applicationFilePath();
    QStringList args;
    const QVRProcessConfig& processConf = QVRManager::config().processConfigs().at(processIndex);
    if (!processConf.display().isEmpty()) {
        args << "-display" << processConf.display();
    }
    args << QString("--qvr-log-level=%1").arg(
            logLevel == QVR_Log_Level_Fatal ? "fatal"
            : logLevel == QVR_Log_Level_Warning ? "warning"
            : logLevel == QVR_Log_Level_Info ? "info"
            : logLevel == QVR_Log_Level_Debug ? "debug"
            : "firehose");
    args << QString("--qvr-wd=%1").arg(QDir::currentPath());
    args << QString("--qvr-config=%1").arg(configFilename);
    args << QString("--qvr-process=%1").arg(processIndex);
    args << QString("--qvr-sync-to-vblank=%1").arg(syncToVblank ? 1 : 0);
    args << appArgs;
    if (!processConf.launcher().isEmpty()) {
        QStringList ll = processConf.launcher().split(' ', QString::SkipEmptyParts);
        args.prepend(prg);
        prg = ll[0];
        for (int i = ll.size() - 1; i >= 1; i--)
            args.prepend(ll[i]);
    }
    start(prg, args, QIODevice::ReadWrite);
    if (!waitForStarted(2 * _timeoutMsecs)) {
        QVR_FATAL("failed to launch process %d", processIndex);
        return false;
    }
    return true;
}

bool QVRProcess::exit()
{
    QVR_DEBUG("waiting for process %d to finish... ", index());
    if (!waitForFinished(2 * _timeoutMsecs)) {
        QVR_FATAL("failed to terminate process %d", index());
        return false;
    }
    QVR_DEBUG("... process %d finished", index());
    return true;
}

void QVRProcess::sendCmdInit(const QByteArray& serializedStatData)
{
    Q_ASSERT(processId());
    putChar('i');
    write(serializedStatData);
}

void QVRProcess::sendCmdDevice(const QByteArray& serializedDevice)
{
    Q_ASSERT(processId());
    putChar('d');
    write(serializedDevice);
}

void QVRProcess::sendCmdWasdqeState(const QByteArray& serializedWasdqeState)
{
    Q_ASSERT(processId());
    putChar('w');
    write(serializedWasdqeState);
}

void QVRProcess::sendCmdObserver(const QByteArray& serializedObserver)
{
    Q_ASSERT(processId());
    putChar('o');
    write(serializedObserver);
}

void QVRProcess::sendCmdRender(float n, float f, const QByteArray& serializedDynData)
{
    Q_ASSERT(processId());
    putChar('r');
    { QDataStream ds(this); ds << n << f; }
    write(serializedDynData);
}

void QVRProcess::sendCmdQuit()
{
    Q_ASSERT(processId());
    putChar('q');
}

void QVRProcess::waitForSlaveData()
{
    Q_ASSERT(processId());
    waitForReadyRead(_timeoutMsecs);
}

bool QVRProcess::receiveCmdEvent(QVREvent* e)
{
    Q_ASSERT(processId());
    char c;
    bool r = false;
    if (getChar(&c)) {
        if (c == 'e') {
            QDataStream ds(this);
            ds >> *e;
            r = true;
        } else {
            ungetChar(c);
        }
    }
    return r;
}

bool QVRProcess::receiveCmdSync()
{
    Q_ASSERT(processId());
    char c = '\0';
    if (!getChar(&c) || c != 's') {
        QVR_FATAL("cannot sync with slave process");
        return false;
    }
    return true;
}

void QVRProcess::sendCmdEvent(const QVREvent* e)
{
    _stdout->putChar('e');
    QDataStream ds(_stdout);
    ds << *e;
}

void QVRProcess::sendCmdSync()
{
    _stdout->putChar('s');
    _stdout->flush();
}

bool QVRProcess::receiveCmd(char* cmd)
{
    return _stdin->getChar(cmd);
}

void QVRProcess::receiveCmdInit(QVRApp* app)
{
    QDataStream ds(_stdin);
    app->deserializeStaticData(ds);
}

void QVRProcess::receiveCmdDevice(QVRDevice* dev)
{
    QDataStream ds(_stdin);
    ds >> *dev;
}

void QVRProcess::receiveCmdWasdqeState(int* wasdqeMouseProcessIndex, int* wasdqeMouseWindowIndex, bool* wasdqeMouseInitialized)
{
    QDataStream ds(_stdin);
    ds >> *wasdqeMouseProcessIndex >> *wasdqeMouseWindowIndex >> *wasdqeMouseInitialized;
}

void QVRProcess::receiveCmdObserver(QVRObserver* obs)
{
    QDataStream ds(_stdin);
    ds >> *obs;
}

void QVRProcess::receiveCmdRender(float* n, float* f, QVRApp* app)
{
    QDataStream ds(_stdin);
    ds >> *n >> *f;
    app->deserializeDynamicData(ds);
}

void QVRProcess::flush()
{
    _stdout->flush();
}
