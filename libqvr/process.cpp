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

#include <QCoreApplication>
#include <QDir>

#include "manager.hpp"
#include "process.hpp"
#include "app.hpp"
#include "device.hpp"
#include "observer.hpp"
#include "event.hpp"
#include "logging.hpp"
#include "ipc.hpp"


QVRProcess::QVRProcess(int index) : _index(index)
{
    setProcessChannelMode(QProcess::ForwardedErrorChannel);
}

QVRProcess::~QVRProcess()
{
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

bool QVRProcess::launch(const QString& masterName, const QString& configFilename,
        bool syncToVblank, const QStringList& appArgs)
{
    QString prg = QCoreApplication::applicationFilePath();
    QStringList args;
    if (!config().display().isEmpty()) {
        args << "-display" << config().display();
    }
    args << QString("--qvr-server=%1").arg(masterName);
    args << QString("--qvr-timeout=%1").arg(QVRTimeoutMsecs);
    args << QString("--qvr-process=%1").arg(index());
    args << QString("--qvr-log-level=%1").arg(
            QVRManager::logLevel() == QVR_Log_Level_Fatal ? "fatal"
            : QVRManager::logLevel() == QVR_Log_Level_Warning ? "warning"
            : QVRManager::logLevel() == QVR_Log_Level_Info ? "info"
            : QVRManager::logLevel() == QVR_Log_Level_Debug ? "debug"
            : "firehose");
    args << QString("--qvr-log-file=%1").arg(QVRGetLogFile());
    args << QString("--qvr-wd=%1").arg(QDir::currentPath());
    args << QString("--qvr-sync-to-vblank=%1").arg(syncToVblank ? 1 : 0);
    args << QString("--qvr-config=%1").arg(configFilename);
    args << appArgs;
    if (config().launcher() == "manual") {
        QString s = args.join(' ');
        QVR_FATAL("start process %s manually with the following options:", qPrintable(id()));
        QVR_FATAL("%s", qPrintable(s));
    } else {
        if (!config().launcher().isEmpty()) {
            QStringList ll = config().launcher().split(' ', QString::SkipEmptyParts);
            args.prepend(prg);
            prg = ll[0];
            for (int i = ll.size() - 1; i >= 1; i--)
                args.prepend(ll[i]);
        }
        start(prg, args, QIODevice::ReadWrite);
        if (!waitForStarted(QVRTimeoutMsecs)) {
            QVR_FATAL("failed to launch process %s", qPrintable(id()));
            return false;
        }
    }
    return true;
}

bool QVRProcess::exit()
{
    if (config().launcher() == "manual") {
        // The process should have exited cleanly because it received a 'quit' command.
        // We have no means of checking this or killing a misbehaving process.
    } else {
        QVR_DEBUG("waiting for process %d to finish... ", index());
        if (!waitForFinished(QVRTimeoutMsecs)) {
            QVR_FATAL("failed to terminate process %d", index());
            return false;
        }
        QVR_DEBUG("... process %d finished", index());
    }
    return true;
}
