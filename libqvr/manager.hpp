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

#ifndef QVR_MANAGER_HPP
#define QVR_MANAGER_HPP

#include <QObject>
#include <QVector3D>

template <typename T> class QList;
class QTimer;
class QOpenGLContext;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;

class QVRApp;
class QVRObserverConfig;
class QVRProcessConfig;
class QVRWindowConfig;
class QVRConfig;
class QVRObserver;
class QVRWindow;
class QVRProcess;

typedef enum {
    QVR_Log_Level_Fatal = 0,    // print only fatal errors
    QVR_Log_Level_Warning = 1,  // additionally print warnings (default)
    QVR_Log_Level_Info = 2,     // additionally print informational messages
    QVR_Log_Level_Debug = 3,    // additionally print debugging information
    QVR_Log_Level_Firehose = 4  // additionally print verbose per-frame debugging information
} QVRLogLevel; // you can set environment variable QVR_LOG_LEVEL=FATAL|WARN|INFO|DEBUG|FIREHOSE

class QVRManager : public QObject
{
Q_OBJECT

private:
    // Data initialized by the constructor:
    QTimer* _triggerTimer;
    QTimer* _fpsTimer;
    QVRLogLevel _logLevel;
    QString _workingDir;
    int _processIndex;
    unsigned int _fpsMsecs;
    unsigned int _fpsCounter;
    QString _configFilename;
    QStringList _appArgs;
    // Data initialized by init():
    QVRApp* _app;
    QVRConfig* _config;
    QList<QVRObserver*> _observers;
    QList<QVRObserver*> _customObservers; // subset of _observers
    QVRWindow* _masterWindow;
    QOpenGLContext* _masterGLContext;
    QList<QVRWindow*> _windows;
    QVRProcess* _thisProcess;
    QList<QVRProcess*> _slaveProcesses;
    bool _wantExit;
    bool _haveWasdqeObservers;    // WASDQE observers: do we have at least one?
    bool _wasdqeIsPressed[6];     // WASDQE observers: keys
    int _wasdqeMouseProcessIndex; // WASDQE observers: process with mouse grab
    int _wasdqeMouseWindowIndex;  // WASDQE observers: window with mouse grab
    bool _wasdqeMouseInitialized; // WASDQE observers: was mouse grab initialized
    QVector3D _wasdqePos;         // WASDQE observers: position
    float _wasdqeHorzAngle;       // WASDQE observers: angle around the y axis
    float _wasdqeVertAngle;       // WASDQE observers: angle around the x axis
    bool _haveVrpnObservers;

    void processEventQueue();

    void render();
    void waitForBufferSwaps();
    void quit();

private slots:
    void masterLoop();
    void slaveLoop();
    void printFps();

public:
    QVRManager(int& argc, char* argv[]);
    ~QVRManager();

    bool init(QVRApp* app);

    static QVRManager* instance();
    static QVRLogLevel logLevel();

    static int processIndex();

    static const QVRConfig& config();
    static const QVRObserverConfig& observerConfig(int observerIndex);
    static const QVRProcessConfig& processConfig(int processIndex = processIndex());
    static const QVRWindowConfig& windowConfig(int processIndex, int windowIndex);

    static void enqueueKeyPressEvent(int pi, int wi, const QRect& wG, const QRect& sG, const float* f, const QMatrix4x4& vM, QKeyEvent* event);
    static void enqueueKeyReleaseEvent(int pi, int wi, const QRect& wG, const QRect& sG, const float* f, const QMatrix4x4& vM, QKeyEvent* event);
    static void enqueueMouseMoveEvent(int pi, int wi, const QRect& wG, const QRect& sG, const float* f, const QMatrix4x4& vM, QMouseEvent* event);
    static void enqueueMousePressEvent(int pi, int wi, const QRect& wG, const QRect& sG, const float* f, const QMatrix4x4& vM, QMouseEvent* event);
    static void enqueueMouseReleaseEvent(int pi, int wi, const QRect& wG, const QRect& sG, const float* f, const QMatrix4x4& vM, QMouseEvent* event);
    static void enqueueMouseDoubleClickEvent(int pi, int wi, const QRect& wG, const QRect& sG, const float* f, const QMatrix4x4& vM, QMouseEvent* event);
    static void enqueueWheelEvent(int pi, int wi, const QRect& wG, const QRect& sG, const float* f, const QMatrix4x4& vM, QWheelEvent* event);
};

#endif
