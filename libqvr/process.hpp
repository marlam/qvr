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

#ifndef QVR_PROCESS_HPP
#define QVR_PROCESS_HPP

#include <QProcess>

#include "manager.hpp"

class QFile;

class QVREvent;
class QVRWindowConfig;
class QVRProcessConfig;

class QVRProcess : public QProcess
{
private:
    static const int _timeoutMsecs = 10000;
    int _index;
    QFile* _stdin;
    QFile* _stdout;

    // functions for the master to manage slave processes
    bool launch(const QString& configFilename, QVRLogLevel logLevel, int processIndex,
            const QStringList& appArgs);
    bool exit();

    // communication functions to be called by the master to apply to slave processes
    void sendCmdInit(const QByteArray& serializedStatData);           // starts with 'i'
    void sendCmdWasdqeState(const QByteArray& serializedWasdqeState); // starts with 'w'
    void sendCmdObserver(const QByteArray& serializedObserver);       // starts with 'o'
    void sendCmdRender(const QByteArray& serializedDynData);          // starts with 'r'
    void sendCmdQuit();                                               // is 'q'
    void waitForSlaveData();
    bool receiveCmdEvent(QVREvent* e);
    bool receiveCmdSync();

    // communication functions to be called by the slave to apply to the master
    void sendCmdEvent(const QVREvent* e);               // starts with 'e'
    void sendCmdSync();                                 // is 's'
    bool receiveCmd(char* cmd);                         // will only get the first character
    void receiveCmdInit(QVRApp* app);
    void receiveCmdWasdqeState(int*, int*, bool*);
    void receiveCmdObserver(QVRObserver* obs);
    void receiveCmdRender(QVRApp* app);

    // explicit flushing of stdout
    void flush();

    friend class QVRManager;

public:
    QVRProcess(int index);
    ~QVRProcess();

    int index() const;
    const QString& id() const;
    const QVRProcessConfig& config() const;

    const QString& windowId(int windowIndex) const;
    const QVRWindowConfig& windowConfig(int windowIndex) const;
};

#endif
