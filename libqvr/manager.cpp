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

#include <cmath>

#include <QDir>
#include <QQueue>
#include <QDesktopWidget>
#include <QApplication>
#include <QTimer>
#include <QElapsedTimer>

#include "manager.hpp"
#include "event.hpp"
#include "logging.hpp"
#include "config.hpp"
#include "app.hpp"
#include "device.hpp"
#include "observer.hpp"
#include "window.hpp"
#include "process.hpp"
#include "ipc.hpp"
#include "internalglobals.hpp"


static QVRManager* manager = NULL; // singleton instance
static QQueue<QVREvent>* eventQueue = NULL; // single event queue for the singleton

static bool parseLogLevel(const QString& ll, QVRLogLevel* logLevel)
{
    if (ll.compare("fatal", Qt::CaseInsensitive) == 0)
        *logLevel = QVR_Log_Level_Fatal;
    else if (ll.compare("warning", Qt::CaseInsensitive) == 0)
        *logLevel = QVR_Log_Level_Warning;
    else if (ll.compare("info", Qt::CaseInsensitive) == 0)
        *logLevel = QVR_Log_Level_Info;
    else if (ll.compare("debug", Qt::CaseInsensitive) == 0)
        *logLevel = QVR_Log_Level_Debug;
    else if (ll.compare("firehose", Qt::CaseInsensitive) == 0)
        *logLevel = QVR_Log_Level_Firehose;
    else {
        // fall back to default
        *logLevel = QVR_Log_Level_Info;
        return false;
    }
    return true;
}

static void removeTwoArgs(int& argc, char* argv[], int i)
{
    for (int j = i; j < argc - 2; j++)
        argv[j] = argv[j + 2];
    argc -= 2;
}

static void removeArg(int& argc, char* argv[], int i)
{
    for (int j = i; j < argc - 1; j++)
        argv[j] = argv[j + 1];
    argc -= 1;
}

QVRManager::QVRManager(int& argc, char* argv[]) :
    _triggerTimer(new QTimer),
    _fpsTimer(new QTimer),
    _logLevel(QVR_Log_Level_Warning),
    _workingDir(),
    _processIndex(0),
    _syncToVblank(true),
    _fpsMsecs(0),
    _fpsCounter(0),
    _configFilename(),
    _server(NULL),
    _client(NULL),
    _app(NULL),
    _config(NULL),
    _devices(),
    _constDevices(),
    _observers(),
    _customObservers(),
    _masterWindow(NULL),
    _masterGLContext(NULL),
    _windows(),
    _thisProcess(NULL),
    _slaveProcesses(),
    _wantExit(false),
    _wandNavigationTimer(NULL),
    _wasdqeTimer(NULL)
{
    Q_ASSERT(!manager);  // there can be only one
    manager = this;
    eventQueue = new QQueue<QVREvent>;
    Q_INIT_RESOURCE(qvr);

    // set global timeout value (-1 means never timeout)
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--qvr-timeout") == 0 && i < argc - 1) {
            QVRTimeoutMsecs = ::atoi(argv[i + 1]);
            removeTwoArgs(argc, argv, i);
            break;
        } else if (strncmp(argv[i], "--qvr-timeout=", 14) == 0) {
            QVRTimeoutMsecs = ::atoi(argv[i] + 14);
            removeArg(argc, argv, i);
            break;
        }
    }

    // set log level
    if (::getenv("QVR_LOG_LEVEL"))
        parseLogLevel(::getenv("QVR_LOG_LEVEL"), &_logLevel);
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--qvr-log-level") == 0 && i < argc - 1) {
            parseLogLevel(argv[i + 1], &_logLevel);
            removeTwoArgs(argc, argv, i);
            break;
        } else if (strncmp(argv[i], "--qvr-log-level=", 16) == 0) {
            parseLogLevel(argv[i] + 16, &_logLevel);
            removeArg(argc, argv, i);
            break;
        }
    }

    // set log file
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--qvr-log-file") == 0 && i < argc - 1) {
            QVRSetLogFile(argv[i + 1]);
            removeTwoArgs(argc, argv, i);
            break;
        } else if (strncmp(argv[i], "--qvr-log-file=", 15) == 0) {
            QVRSetLogFile(argv[i] + 15);
            removeArg(argc, argv, i);
            break;
        }
    }

    // set working directory
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--qvr-wd=", 9) == 0) {
            _workingDir = argv[i] + 9;
            removeArg(argc, argv, i);
            break;
        }
    }

    // get process index (it is 0 == master by default)
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--qvr-process=", 14) == 0) {
            _processIndex = ::atoi(argv[i] + 14);
            removeArg(argc, argv, i);
            break;
        }
    }

    // set sync-to-vblank
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--qvr-sync-to-vblank") == 0 && i < argc - 1) {
            _syncToVblank = ::atoi(argv[i + 1]);
            removeTwoArgs(argc, argv, i);
            break;
        } else if (strncmp(argv[i], "--qvr-sync-to-vblank=", 21) == 0) {
            _syncToVblank = ::atoi(argv[i] + 21);
            removeArg(argc, argv, i);
            break;
        }
    }

    // set FPS printing (only on master)
    if (_processIndex == 0) {
        if (::getenv("QVR_FPS"))
            _fpsMsecs = ::atoi(::getenv("QVR_FPS"));
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "--qvr-fps") == 0 && i < argc - 1) {
                _fpsMsecs = ::atoi(argv[i + 1]);
                removeTwoArgs(argc, argv, i);
                break;
            } else if (strncmp(argv[i], "--qvr-fps=", 10) == 0) {
                _fpsMsecs = ::atoi(argv[i] + 10);
                removeArg(argc, argv, i);
                break;
            }
        }
    }

    // get configuration file name (if any)
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--qvr-config") == 0 && i < argc - 1) {
            _configFilename = argv[i + 1];
            removeTwoArgs(argc, argv, i);
            break;
        } else if (strncmp(argv[i], "--qvr-config=", 13) == 0) {
            _configFilename = argv[i] + 13;
            removeArg(argc, argv, i);
            break;
        }
    }

    // get master name (if any)
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--qvr-server=", 13) == 0) {
            _masterName = QString(argv[i] + 13);
            removeArg(argc, argv, i);
            break;
        }
    }

    // preserve remaining command line content for slave processes
    for (int i = 1; i < argc; i++)
        _appArgs << argv[i];
}

QVRManager::~QVRManager()
{
#ifdef HAVE_OCULUS
    if (QVROculus) {
# if (OVR_PRODUCT_VERSION >= 1)
        ovr_Shutdown();
# else
        ovrHmd_Destroy(QVROculus);
# endif
    }
#endif
#ifdef HAVE_OPENVR
    if (QVROpenVRSystem) {
        vr::VR_Shutdown();
    }
#endif
#ifdef HAVE_OSVR
    if (QVROsvrClientContext) {
        osvrDestroyRenderManager(QVROsvrRenderManager);
        QVROsvrRenderManager = NULL;
        osvrClientShutdown(QVROsvrClientContext);
        QVROsvrDisplayConfig = NULL;
        QVROsvrClientContext = NULL;
    }
#endif
    for (int i = 0; i < _devices.size(); i++)
        delete _devices.at(i);
    for (int i = 0; i < _observers.size(); i++)
        delete _observers.at(i);
    for (int i = 0; i < _windows.size(); i++)
        delete _windows.at(i);
    for (int i = 0; i < _slaveProcesses.size(); i++)
        delete _slaveProcesses.at(i);
    delete _masterWindow;
    delete _thisProcess;
    delete _config;
    delete _triggerTimer;
    delete _fpsTimer;
    delete _wasdqeTimer;
    delete _wandNavigationTimer;
    delete eventQueue;
    delete _server;
    delete _client;
    eventQueue = NULL;
    manager = NULL;
}

bool QVRManager::init(QVRApp* app)
{
    Q_ASSERT(!_app);

    _app = app;

    if (!_workingDir.isEmpty())
        QDir::setCurrent(_workingDir);

    // Get configuration
    _config = new QVRConfig;
    if (_configFilename.isEmpty()) {
        _config->createDefault();
    } else {
        if (!_config->readFromFile(_configFilename)) {
            return false;
        }
    }

    // Initialize HMDs for this process if required by the configuration
    bool needToInitializeOculus = false;
    bool needToInitializeOpenVR = false;
    bool needToInitializeOSVR = false;
    for (int d = 0; d < _config->deviceConfigs().size(); d++) {
        if (_config->deviceConfigs()[d].processIndex() == _processIndex) {
            if (_config->deviceConfigs()[d].trackingType() == QVR_Device_Tracking_Oculus) {
                needToInitializeOculus = true;
            }
            if (_config->deviceConfigs()[d].trackingType() == QVR_Device_Tracking_OpenVR
                    || _config->deviceConfigs()[d].buttonsType() == QVR_Device_Buttons_OpenVR
                    || _config->deviceConfigs()[d].analogsType() == QVR_Device_Analogs_OpenVR) {
                needToInitializeOpenVR = true;
            }
            if (_config->deviceConfigs()[d].trackingType() == QVR_Device_Tracking_OSVR
                    || _config->deviceConfigs()[d].buttonsType() == QVR_Device_Buttons_OSVR
                    || _config->deviceConfigs()[d].analogsType() == QVR_Device_Analogs_OSVR) {
                needToInitializeOSVR = true;
            }
        }
    }
    for (int w = 0; w < processConfig().windowConfigs().size(); w++) {
        if (windowConfig(_processIndex, w).outputMode() == QVR_Output_Stereo_Oculus) {
            needToInitializeOculus = true;
        }
        if (windowConfig(_processIndex, w).outputMode() == QVR_Output_Stereo_OpenVR) {
            needToInitializeOpenVR = true;
        }
        if (windowConfig(_processIndex, w).outputMode() == QVR_Output_OSVR) {
            needToInitializeOSVR = true;
        }
    }
    if (needToInitializeOculus) {
#ifdef HAVE_OCULUS
        if (!QVROculus) {
            QVRAttemptOculusInitialization();
            if (!QVROculus) {
                QVR_FATAL("cannot initialize Oculus");
                return false;
            }
        }
#else
        QVR_FATAL("configuration requires Oculus, but Oculus support is not available");
        return false;
#endif
    }
    if (needToInitializeOpenVR) {
#ifdef HAVE_OPENVR
        if (!QVROpenVRSystem) {
            QVRAttemptOpenVRInitialization();
            if (!QVROpenVRSystem) {
                QVR_FATAL("cannot initialize OpenVR");
                return false;
            }
        }
#else
        QVR_FATAL("configuration requires OpenVR, but OpenVR support is not available");
        return false;
#endif
    }
    if (needToInitializeOSVR) {
#ifdef HAVE_OSVR
        if (!QVROsvrClientContext) {
            QVRAttemptOSVRInitialization();
            if (!QVROsvrClientContext) {
                QVR_FATAL("cannot initialize OSVR");
                return false;
            }
        }
#else
        QVR_FATAL("configuration requires OSVR, but OSVR is not available");
        return false;
#endif
    }

    // Create devices
    bool haveVrpnDevices = false;
    for (int d = 0; d < _config->deviceConfigs().size(); d++) {
        _devices.append(new QVRDevice(d));
        _constDevices.append(_devices.last());
        if (_config->deviceConfigs()[d].trackingType() == QVR_Device_Tracking_VRPN
                || _config->deviceConfigs()[d].buttonsType() == QVR_Device_Buttons_VRPN
                || _config->deviceConfigs()[d].analogsType() == QVR_Device_Analogs_VRPN) {
            haveVrpnDevices = true;
        }
    }
    if (haveVrpnDevices) {
#ifdef HAVE_VRPN
#else
        QVR_FATAL("devices configured to use VRPN, but VRPN is not available");
        return false;
#endif
    }

    // Create observers
    _haveWasdqeObservers = false;
    bool haveWandNavigationObservers = false;
    for (int o = 0; o < _config->observerConfigs().size(); o++) {
        _observers.append(new QVRObserver(o));
        if (_config->observerConfigs()[o].navigationType() == QVR_Navigation_Custom
                || _config->observerConfigs()[o].trackingType() == QVR_Tracking_Custom)
            _customObservers.append(_observers[o]);
        if (_config->observerConfigs()[o].navigationType() == QVR_Navigation_WASDQE)
            _haveWasdqeObservers = true;
        int navDev = -1;
        if (_config->observerConfigs()[o].navigationType() == QVR_Navigation_Device) {
            haveWandNavigationObservers = true;
            const QString devId = _config->observerConfigs()[o].navigationParameters().trimmed();
            for (int d = 0; d < _devices.size(); d++) {
                if (_devices[d]->config().id() == devId) {
                    if (_devices[d]->buttons() < 4) {
                        QVR_FATAL("observer %s: navigation device %s has less than 4 buttons",
                                qPrintable(_observers[o]->id()), qPrintable(_devices[d]->id()));
                        return false;
                    }
                    if (_devices[d]->analogs() < 2) {
                        QVR_FATAL("observer %s: navigation device %s has less than 2 analog joystick elements",
                                qPrintable(_observers[o]->id()), qPrintable(_devices[d]->id()));
                        return false;
                    }
                    navDev = d;
                    break;
                }
            }
        }
        _observerNavigationDevices.append(navDev);
        int trackDev0 = -1;
        int trackDev1 = -1;
        if (_config->observerConfigs()[o].trackingType() == QVR_Tracking_Device) {
            const QString devId = _config->observerConfigs()[o].trackingParameters().trimmed();
            const QStringList devIdList = devId.split(' ', QString::SkipEmptyParts);
            for (int d = 0; d < _devices.size(); d++) {
                if (devIdList.size() == 2) {
                    if (_devices[d]->config().id() == devIdList[0])
                        trackDev0 = d;
                    else if (_devices[d]->config().id() == devIdList[1])
                        trackDev1 = d;
                } else {
                    if (_devices[d]->config().id() == devId)
                        trackDev0 = d;
                }
            }
        }
        _observerTrackingDevices0.append(trackDev0);
        _observerTrackingDevices1.append(trackDev1);
    }
    if (_haveWasdqeObservers) {
        _wasdqeTimer = new QElapsedTimer;
        for (int i = 0; i < 6; i++)
            _wasdqeIsPressed[i] = false;
        _wasdqeMouseProcessIndex = -1;
        _wasdqeMouseWindowIndex = -1;
        _wasdqeMouseInitialized = false;
        _wasdqePos = QVector3D(0.0f, 0.0f, 0.0f);
        _wasdqeHorzAngle = 0.0f;
        _wasdqeVertAngle = 0.0f;
    }
    if (haveWandNavigationObservers) {
        _wandNavigationTimer = new QElapsedTimer;
        _wandNavigationPos = QVector3D(0.0f, 0.0f, 0.0f);
        _wandNavigationRotY = 0.0f;
    }

    // Create processes
    _thisProcess = new QVRProcess(_processIndex);
    if (_processIndex == 0) {
        if (_config->processConfigs().size() > 1) {
            QVR_INFO("starting IPC server");
            _server = new QVRServer;
            bool local = true;
            for (int p = 1; p < _config->processConfigs().size(); p++) {
                if (!_config->processConfigs()[p].launcher().isEmpty()) {
                    // assume that the launcher starts the process on a remote
                    // host and we need a TCP server to communicate
                    local = false;
                    break;
                }
            }
            bool r = (local ? _server->startLocal()
                    : _server->startTcp(_config->processConfigs()[0].address()));
            if (!r) {
                QVR_FATAL("cannot start IPC server");
                return false;
            }
            QByteArray serializedStatData;
            QDataStream serializationDataStream(&serializedStatData, QIODevice::WriteOnly);
            _app->serializeStaticData(serializationDataStream);
            for (int p = 1; p < _config->processConfigs().size(); p++) {
                QVRProcess* process = new QVRProcess(p);
                _slaveProcesses.append(process);
                QVR_INFO("launching slave process %s (index %d) ...", qPrintable(process->id()), p);
                if (!process->launch(_server->name(), _configFilename, _syncToVblank, _appArgs)) {
                    return false;
                }
            }
            QVR_INFO("waiting for slave processes to connect to master ...");
            if (!_server->waitForClients(_config->processConfigs().size() - 1))
                return false;
            QVR_INFO("... all clients connected");
            QVR_INFO("initializing slave processes with %d bytes of static application data", serializedStatData.size());
            _server->sendCmdInit(serializedStatData);
            _server->flush();
        }
    } else {
        _client = new QVRClient;
        if (!_client->start(_masterName)) {
            QVR_FATAL("cannot connect to master");
            return false;
        }
        QVRClientCmd cmd;
        if (!_client->receiveCmd(&cmd, true) || cmd != QVRClientCmdInit) {
            QVR_FATAL("cannot receive init command from master");
            return false;
        }
        QVR_INFO("initializing slave process %s (index %d) ...", qPrintable(_thisProcess->id()), _processIndex);
        _client->receiveCmdInitArgs(_app);
        QVR_INFO("... done");
    }

    // Find out about our desktop and available screens
    QDesktopWidget* desktop = QApplication::desktop();
    QVR_INFO("process %s (index %d) uses %s which has %d screens",
            qPrintable(processConfig().id()), _processIndex,
            processConfig().display().isEmpty()
            ? "default display"
            : qPrintable(QString("display %1").arg(processConfig().display())),
            desktop->screenCount());
    for (int i = 0; i < desktop->screenCount(); i++) {
        QRect r = desktop->screenGeometry(i);
        QVR_INFO("  screen %d geometry: %d %d %dx%d", i, r.x(), r.y(), r.width(), r.height());
    }

    // Create windows
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setSwapInterval(_syncToVblank ? 1 : 0);
    QSurfaceFormat::setDefaultFormat(format);
    QVR_INFO("process %s (index %d) creating %d windows",
            qPrintable(processConfig().id()), _processIndex,
            processConfig().windowConfigs().size());
    if (processConfig().windowConfigs().size() > 0) {
        for (int w = 0; w < processConfig().windowConfigs().size(); w++) {
            int ds = windowConfig(_processIndex, w).initialDisplayScreen();
            if (ds < -1 || ds >= desktop->screenCount()) {
                QVR_FATAL("display has no screen %d", ds);
                return false;
            }
        }
        _masterGLContext = new QOpenGLContext();
        QVR_INFO("  master window...");
        _masterWindow = new QVRWindow(_masterGLContext, 0, -1, -1);
        if (!_masterWindow->isValid())
            return false;
        for (int w = 0; w < processConfig().windowConfigs().size(); w++) {
            QVR_INFO("  window %d...", w);
            QVRObserver* observer = _observers.at(windowConfig(_processIndex, w).observerIndex());
            QVRWindow* window = new QVRWindow(_masterGLContext, observer, _processIndex, w);
            if (!window->isValid())
                return false;
            _windows.append(window);
        }
    }

    // Initialize application process and windows
    if (_masterWindow)
        _masterWindow->winContext()->makeCurrent(_masterWindow);
    if (!_app->initProcess(_thisProcess))
        return false;
    for (int w = 0; w < _windows.size(); w++)
        if (!_app->initWindow(_windows[w]))
            return false;
    if (_masterWindow)
        _masterWindow->winContext()->doneCurrent();
    if (_processIndex == 0) {
        updateDevices();
        _app->update(_constDevices);
        _app->updateObservers(_constDevices, _customObservers);
    }

    // Initialize FPS printing
    if (_fpsMsecs > 0) {
        connect(_fpsTimer, SIGNAL(timeout()), this, SLOT(printFps()));
        _fpsTimer->start(_fpsMsecs);
    }

    // Initialize render loop (only on master process)
    if (_processIndex == 0) {
        // Set up timer to trigger master loop
        QObject::connect(_triggerTimer, SIGNAL(timeout()), this, SLOT(masterLoop()));
        _masterLoopFirstRun = true;
        _triggerTimer->start();
    } else {
        // Set up timer to trigger slave loop
        QObject::connect(_triggerTimer, SIGNAL(timeout()), this, SLOT(slaveLoop()));
        _triggerTimer->start();
    }

    QApplication::processEvents();

    return true;
}

#ifdef HAVE_OCULUS
static void QVRUpdateOculus()
{
# if (OVR_PRODUCT_VERSION >= 1)
    QVROculusFrameIndex++;
    double dt = ovr_GetPredictedDisplayTime(QVROculus, QVROculusFrameIndex);
    QVROculusTrackingState = ovr_GetTrackingState(QVROculus, dt, ovrTrue);
    ovr_CalcEyePoses(QVROculusTrackingState.HeadPose.ThePose, QVROculusHmdToEyeViewOffset, QVROculusRenderPoses);
    QVROculusLayer.SensorSampleTime = ovr_GetTimeInSeconds();
# else
    ovrHmd_BeginFrame(QVROculus, 0);
    ovrVector3f hmdToEyeViewOffset[2] = {
        QVROculusEyeRenderDesc[0].HmdToEyeViewOffset,
        QVROculusEyeRenderDesc[1].HmdToEyeViewOffset
    };
    ovrHmd_GetEyePoses(QVROculus, 0, hmdToEyeViewOffset,
            QVROculusRenderPoses, &QVROculusTrackingState);
# endif
}
#endif

void QVRManager::masterLoop()
{
    Q_ASSERT(_processIndex == 0);

    QVR_FIREHOSE("masterLoop() ...");

    if (_masterLoopFirstRun) {
        // XXX: without this ugly hack, some windows are still not exposed
        // when rendering and swapping buffers (at least in multi-process
        // configurations), even though we called
        // QApplication::processEvents() at the end of init().
        QApplication::processEvents();
        _masterLoopFirstRun = false;
    }

    if (_masterWindow)
        _masterWindow->winContext()->makeCurrent(_masterWindow);

    if (_wantExit || _app->wantExit()) {
        QVR_FIREHOSE("  ... exit now!");
        _triggerTimer->stop();
        if (_slaveProcesses.size() > 0) {
            _server->sendCmdQuit();
            _server->flush();
            for (int p = 0; p < _slaveProcesses.size(); p++)
                _slaveProcesses[p]->exit();
        }
        quit();
        return;
    }

    updateDevices();
    for (int o = 0; o < _observers.size(); o++) {
        QVRObserver* obs = _observers[o];
        QVR_FIREHOSE("  ... updating observer %d", o);
        if (obs->config().navigationType() == QVR_Navigation_WASDQE) {
            const float speed = 1.5f; // in meters per second; TODO: make this configurable?
            float seconds = 0.0f;
            if (_wasdqeTimer->isValid()) {
                seconds = _wasdqeTimer->nsecsElapsed() / 1e9f;
                _wasdqeTimer->restart();
            } else {
                _wasdqeTimer->start();
            }
            if (_wasdqeIsPressed[0] || _wasdqeIsPressed[1] || _wasdqeIsPressed[2] || _wasdqeIsPressed[3]) {
                QQuaternion viewerRot = obs->trackingOrientation() * obs->navigationOrientation();
                QVector3D dir;
                if (_wasdqeIsPressed[0])
                    dir = viewerRot * QVector3D(0.0f, 0.0f, -1.0f);
                else if (_wasdqeIsPressed[1])
                    dir = viewerRot * QVector3D(-1.0f, 0.0f, 0.0f);
                else if (_wasdqeIsPressed[2])
                    dir = viewerRot * QVector3D(0.0f, 0.0f, +1.0f);
                else if (_wasdqeIsPressed[3])
                    dir = viewerRot * QVector3D(+1.0f, 0.0f, 0.0f);
                dir.setY(0.0f);
                dir.normalize();
                _wasdqePos += speed * seconds * dir;
            }
            if (_wasdqeIsPressed[4]) {
                _wasdqePos += speed * seconds * QVector3D(0.0f, +1.0f, 0.0f);
            }
            if (_wasdqeIsPressed[5]) {
                _wasdqePos += speed * seconds * QVector3D(0.0f, -1.0f, 0.0f);
            }
            obs->setNavigation(_wasdqePos + obs->config().initialNavigationPosition(),
                    QQuaternion::fromEulerAngles(_wasdqeVertAngle, _wasdqeHorzAngle, 0.0f)
                    * obs->config().initialNavigationOrientation());
        }
        if (obs->config().navigationType() == QVR_Navigation_Device) {
            const QVRDevice* dev = _devices.at(_observerNavigationDevices[o]);
            const float speed = 1.5f; // in meters per second; TODO: make this configurable?
            float seconds = 0.0f;
            if (_wandNavigationTimer->isValid()) {
                seconds = _wandNavigationTimer->nsecsElapsed() / 1e9f;
                _wandNavigationTimer->restart();
            } else {
                _wandNavigationTimer->start();
            }
            if (std::abs(dev->analog(0)) > 0.0f || std::abs(dev->analog(1)) > 0.0f) {
                QQuaternion wandRot = dev->orientation() * QQuaternion::fromEulerAngles(0.0f, _wandNavigationRotY, 0.0f);
                QVector3D forwardDir = wandRot * QVector3D(0.0f, 0.0f, -1.0f);
                forwardDir.setY(0.0f);
                forwardDir.normalize();
                QVector3D rightDir = wandRot * QVector3D(1.0f, 0.0f, 0.0f);
                rightDir.setY(0.0f);
                rightDir.normalize();
                _wandNavigationPos += speed * seconds * (forwardDir * dev->analog(0) + rightDir * dev->analog(1));
            }
            if (dev->button(0)) {
                _wandNavigationPos += speed * seconds * QVector3D(0.0f, +1.0f, 0.0f);
            }
            if (dev->button(1)) {
                _wandNavigationPos += speed * seconds * QVector3D(0.0f, -1.0f, 0.0f);
            }
            if (dev->button(2)) {
                _wandNavigationRotY += 1.0f;
                if (_wandNavigationRotY >= 360.0f)
                    _wandNavigationRotY -= 360.0f;
            }
            if (dev->button(3)) {
                _wandNavigationRotY -= 1.0f;
                if (_wandNavigationRotY <= 0.0f)
                    _wandNavigationRotY += 360.0f;
            }
            obs->setNavigation(_wandNavigationPos + obs->config().initialNavigationPosition(),
                    QQuaternion::fromEulerAngles(0.0f, _wandNavigationRotY, 0.0f) * obs->config().initialNavigationOrientation());
        }
        if (obs->config().trackingType() == QVR_Tracking_Device) {
            int td0 = _observerTrackingDevices0[o];
            int td1 = _observerTrackingDevices1[o];
            if (td0 >= 0 && td1 >= 0) {
                const QVRDevice* dev0 = _devices.at(td0);
                const QVRDevice* dev1 = _devices.at(td1);
                obs->setTracking(dev0->position(), dev0->orientation(), dev1->position(), dev1->orientation());
            } else if (td0 >= 0) {
                const QVRDevice* dev = _devices.at(td0);
                obs->setTracking(dev->position(), dev->orientation());
            }
        }
    }

    _app->getNearFar(_near, _far);

    if (_slaveProcesses.size() > 0) {
        for (int d = 0; d < _devices.size(); d++) {
            QByteArray serializedDevice;
            QDataStream serializationDataStream(&serializedDevice, QIODevice::WriteOnly);
            serializationDataStream << (*_devices[d]);
            QVR_FIREHOSE("  ... sending device %d (%d bytes) to slave processes", d, serializedDevice.size());
            _server->sendCmdDevice(serializedDevice);
        }
        if (_haveWasdqeObservers) {
            QByteArray serializedWasdqeState;
            QDataStream serializationDataStream(&serializedWasdqeState, QIODevice::WriteOnly);
            serializationDataStream << _wasdqeMouseProcessIndex << _wasdqeMouseWindowIndex << _wasdqeMouseInitialized;
            QVR_FIREHOSE("  ... sending wasdqe state to slave processes");
            _server->sendCmdWasdqeState(serializedWasdqeState);
        }
        for (int o = 0; o < _observers.size(); o++) {
            QByteArray serializedObserver;
            QDataStream serializationDataStream(&serializedObserver, QIODevice::WriteOnly);
            serializationDataStream << (*_observers[o]);
            QVR_FIREHOSE("  ... sending observer %d (%d bytes) to slave processes", o, serializedObserver.size());
            _server->sendCmdObserver(serializedObserver);
        }
        QByteArray serializedDynData;
        QDataStream serializationDataStream(&serializedDynData, QIODevice::WriteOnly);
        _app->serializeDynamicData(serializationDataStream);
        QVR_FIREHOSE("  ... sending dynamic application data (%d bytes) to slave processes", serializedDynData.size());
        _server->sendCmdRender(_near, _far, serializedDynData);
        _server->flush();
        QVR_FIREHOSE("  ... rendering commands are on their way");
    }

    render();

    // process events and run application updates while the windows wait for the buffer swap
    QVR_FIREHOSE("  ... event processing");
    QApplication::processEvents();
    processEventQueue();
    QVR_FIREHOSE("  ... app update");
    _app->update(_constDevices);
    _app->updateObservers(_constDevices, _customObservers);

    // now wait for windows to finish buffer swap...
    waitForBufferSwaps();
    // ... and for the slaves to sync
    if (_slaveProcesses.size() > 0) {
        QVR_FIREHOSE("  ... waiting for slaves to sync");
        QList<QVREvent> slaveEvents;
        if (!_server->receiveCmdsEventAndSync(&slaveEvents)) {
            _wantExit = true;
        } else {
            QVR_FIREHOSE("  ... all slaves synced");
        }
        for (int e = 0; e < slaveEvents.size(); e++) {
            QVR_FIREHOSE("  ... got an event from process %d window %d",
                    slaveEvents[e].context.processIndex(), slaveEvents[e].context.windowIndex());
            eventQueue->enqueue(slaveEvents[e]);
        }
    }

    _fpsCounter++;
}

void QVRManager::slaveLoop()
{
    QVRClientCmd cmd;
    while (_client->receiveCmd(&cmd)) {
        if (cmd == QVRClientCmdUpdateDevices) {
            QVR_FIREHOSE("  ... got command 'update-devices' from master");
#ifdef HAVE_OCULUS
            if (QVROculus) {
                QVRUpdateOculus();
            }
#endif
#ifdef HAVE_OPENVR
            if (QVROpenVRSystem) {
                static bool firstRun = true;
                if (firstRun) {
                    QVRUpdateOpenVR();
                    firstRun = false;
                }
            }
#endif
#ifdef HAVE_OSVR
            if (QVROsvrClientContext) {
                osvrClientUpdate(QVROsvrClientContext);
            }
#endif
            int n = 0;
            QByteArray data;
            QDataStream ds(&data, QIODevice::WriteOnly);
            for (int d = 0; d < _config->deviceConfigs().size(); d++) {
                if (_devices[d]->config().processIndex() == processIndex()) {
                    _devices[d]->update();
                    ds << *(_devices[d]);
                    n++;
                }
            }
            QVR_FIREHOSE("  ... sending %d updated devices to master", n);
            _client->sendReplyUpdateDevices(n, data);
            _client->flush();
        } else if (cmd == QVRClientCmdDevice) {
            QVR_FIREHOSE("  ... got command 'device' from master");
            QVRDevice d;
            _client->receiveCmdDeviceArgs(&d);
            *(_devices.at(d.index())) = d;
        } else if (cmd == QVRClientCmdWasdqeState) {
            QVR_FIREHOSE("  ... got command 'wasdqestate' from master");
            _client->receiveCmdWasdqeStateArgs(&_wasdqeMouseProcessIndex,
                    &_wasdqeMouseWindowIndex, &_wasdqeMouseInitialized);
        } else if (cmd == QVRClientCmdObserver) {
            QVR_FIREHOSE("  ... got command 'observer' from master");
            QVRObserver o;
            _client->receiveCmdObserverArgs(&o);
            *(_observers.at(o.index())) = o;
        } else if (cmd == QVRClientCmdRender) {
            QVR_FIREHOSE("  ... got command 'render' from master");
            _client->receiveCmdRenderArgs(&_near, &_far, _app);
            render();
            QApplication::processEvents();
            while (!eventQueue->empty()) {
                QVR_FIREHOSE("  ... sending command 'event' to master");
                _client->sendCmdEvent(&eventQueue->front());
                _client->flush(); // XXX: seems to be necessary at least on remote tcp sockets!?
                eventQueue->dequeue();
            }
            QVR_FIREHOSE("  ... sending command 'sync' to master");
            _client->sendCmdSync();
            _client->flush();
            waitForBufferSwaps();
        } else if (cmd == QVRClientCmdQuit) {
            QVR_FIREHOSE("  ... got command 'quit' from master");
            _triggerTimer->stop();
            quit();
        } else {
            QVR_FATAL("  got unknown command from master!?");
            _triggerTimer->stop();
            quit();
        }
    }
}

void QVRManager::quit()
{
    QVR_DEBUG("quitting process %d...", _thisProcess->index());
    if (_masterWindow)
        _masterWindow->winContext()->makeCurrent(_masterWindow);

    for (int w = _windows.size() - 1; w >= 0; w--) {
        QVR_DEBUG("... exiting window %d", w);
        _app->exitWindow(_windows[w]);
        _windows[w]->exitGL();
        _windows[w]->close();
    }
    QVR_DEBUG("... exiting process");
    _app->exitProcess(_thisProcess);

    QVR_DEBUG("... quitting process %d done", _thisProcess->index());
}

void QVRManager::updateDevices()
{
    Q_ASSERT(_processIndex == 0);
#ifdef HAVE_OCULUS
    if (QVROculus) {
        QVRUpdateOculus();
    }
#endif
#ifdef HAVE_OPENVR
    if (QVROpenVRSystem) {
        // We only update OpenVR on the first call to this function,
        // since it will be updated at buffer swap time after each
        // rendered frame.
        static bool firstRun = true;
        if (firstRun) {
            QVRUpdateOpenVR();
            firstRun = false;
        }
    }
#endif
#ifdef HAVE_OSVR
    if (QVROsvrClientContext) {
        osvrClientUpdate(QVROsvrClientContext);
    }
#endif
    bool haveRemoteDevices = false;
    for (int d = 0; d < _config->deviceConfigs().size(); d++) {
        if (_devices[d]->config().processIndex() == 0)
            _devices[d]->update();
        else
            haveRemoteDevices = true;
    }
    if (haveRemoteDevices) {
        QVR_FIREHOSE("ordering slave processes to update devices");
        _server->sendCmdUpdateDevices();
        _server->flush();
        QVR_FIREHOSE("getting updated device info from slave processes");
        _server->receiveReplyUpdateDevices(_devices);
    }
}

void QVRManager::render()
{
    QVR_FIREHOSE("  render() ...");

    if (_masterWindow) {
        _masterWindow->winContext()->makeCurrent(_masterWindow);
        _masterWindow->glEnable(GL_FRAMEBUFFER_SRGB);
    }

    if (_windows.size() > 0) {
        QVR_FIREHOSE("  ... preRenderProcess()");
        _app->preRenderProcess(_thisProcess);
        // render
        for (int w = 0; w < _windows.size(); w++) {
            if (!_wasdqeMouseInitialized) {
                if (_wasdqeMouseProcessIndex == _windows[w]->processIndex()
                        && _wasdqeMouseWindowIndex == _windows[w]->index()) {
                    _windows[w]->setCursor(Qt::BlankCursor);
                    QCursor::setPos(_windows[w]->mapToGlobal(
                                QPoint(_windows[w]->width() / 2, _windows[w]->height() / 2)));
                } else {
                    _windows[w]->unsetCursor();
                }
            }
            QVR_FIREHOSE("  ... preRenderWindow(%d)", w);
            _app->preRenderWindow(_windows[w]);
            QVR_FIREHOSE("  ... render(%d)", w);
            unsigned int textures[2];
            const QVRRenderContext& renderContext = _windows[w]->computeRenderContext(_near, _far, textures);
            for (int i = 0; i < renderContext.viewPasses(); i++) {
                QVR_FIREHOSE("  ... pass %d", i);
                QVR_FIREHOSE("  ...   frustum: l=%g r=%g b=%g t=%g n=%g f=%g",
                        renderContext.frustum(i).leftPlane(),
                        renderContext.frustum(i).rightPlane(),
                        renderContext.frustum(i).bottomPlane(),
                        renderContext.frustum(i).topPlane(),
                        renderContext.frustum(i).nearPlane(),
                        renderContext.frustum(i).farPlane());
                QVR_FIREHOSE("  ...   viewmatrix: [%g %g %g %g] [%g %g %g %g] [%g %g %g %g] [%g %g %g %g]",
                        renderContext.viewMatrix(i)(0, 0), renderContext.viewMatrix(i)(0, 1),
                        renderContext.viewMatrix(i)(0, 2), renderContext.viewMatrix(i)(0, 3),
                        renderContext.viewMatrix(i)(1, 0), renderContext.viewMatrix(i)(1, 1),
                        renderContext.viewMatrix(i)(1, 2), renderContext.viewMatrix(i)(1, 3),
                        renderContext.viewMatrix(i)(2, 0), renderContext.viewMatrix(i)(2, 1),
                        renderContext.viewMatrix(i)(2, 2), renderContext.viewMatrix(i)(2, 3),
                        renderContext.viewMatrix(i)(3, 0), renderContext.viewMatrix(i)(3, 1),
                        renderContext.viewMatrix(i)(3, 2), renderContext.viewMatrix(i)(3, 3));
                _app->render(_windows[w], renderContext, i, textures[i]);
            }
            QVR_FIREHOSE("  ... postRenderWindow(%d)", w);
            _app->postRenderWindow(_windows[w]);
        }
        _wasdqeMouseInitialized = true;
        QVR_FIREHOSE("  ... postRenderProcess()");
        _app->postRenderProcess(_thisProcess);
        /* At this point, we must make sure that all textures actually contain
         * the current scene, otherwise artefacts are displayed when the window
         * threads render them. It seems that glFlush() is not enough for all
         * OpenGL implementations; to be safe, we use glFinish(). */
        _masterWindow->glFinish();
        for (int w = 0; w < _windows.size(); w++) {
            QVR_FIREHOSE("  ... renderToScreen(%d)", w);
            _windows[w]->renderToScreen();
        }
        for (int w = 0; w < _windows.size(); w++) {
            QVR_FIREHOSE("  ... asyncSwapBuffers(%d)", w);
            _windows[w]->asyncSwapBuffers();
        }
    }
}

void QVRManager::waitForBufferSwaps()
{
    // wait for windows to finish the buffer swap
    for (int w = 0; w < _windows.size(); w++) {
        QVR_FIREHOSE("  ... waiting for buffer swap %d...", w);
        _windows[w]->waitForSwapBuffers();
        QVR_FIREHOSE("  ... buffer swap %d done.", w);
    }
}

void QVRManager::printFps()
{
    if (_fpsCounter > 0) {
        QVR_FATAL("fps %.1f", _fpsCounter / (_fpsMsecs / 1000.0f));
        _fpsCounter = 0;
    }
}

QVRManager* QVRManager::instance()
{
    return manager;
}

const QVRConfig& QVRManager::config()
{
    Q_ASSERT(manager);
    return *(manager->_config);
}

const QVRDeviceConfig& QVRManager::deviceConfig(int deviceIndex)
{
    return config().deviceConfigs().at(deviceIndex);
}

const QVRObserverConfig& QVRManager::observerConfig(int observerIndex)
{
    return config().observerConfigs().at(observerIndex);
}

const QVRProcessConfig& QVRManager::processConfig(int processIndex)
{
    return config().processConfigs().at(processIndex);
}

const QVRWindowConfig& QVRManager::windowConfig(int processIndex, int windowIndex)
{
    return processConfig(processIndex).windowConfigs().at(windowIndex);
}

QVRLogLevel QVRManager::logLevel()
{
    Q_ASSERT(manager);
    return manager->_logLevel;
}

int QVRManager::processIndex()
{
    Q_ASSERT(manager);
    return manager->_processIndex;
}

void QVRManager::enqueueKeyPressEvent(const QVRRenderContext& c, QKeyEvent* event)
{
    eventQueue->enqueue(QVREvent(QVR_Event_KeyPress, c, *event));
}

void QVRManager::enqueueKeyReleaseEvent(const QVRRenderContext& c, QKeyEvent* event)
{
    eventQueue->enqueue(QVREvent(QVR_Event_KeyRelease, c, *event));
}

void QVRManager::enqueueMouseMoveEvent(const QVRRenderContext& c, QMouseEvent* event)
{
    eventQueue->enqueue(QVREvent(QVR_Event_MouseMove, c, *event));
}

void QVRManager::enqueueMousePressEvent(const QVRRenderContext& c, QMouseEvent* event)
{
    eventQueue->enqueue(QVREvent(QVR_Event_MousePress, c, *event));
}

void QVRManager::enqueueMouseReleaseEvent(const QVRRenderContext& c, QMouseEvent* event)
{
    eventQueue->enqueue(QVREvent(QVR_Event_MouseRelease, c, *event));
}

void QVRManager::enqueueMouseDoubleClickEvent(const QVRRenderContext& c, QMouseEvent* event)
{
    eventQueue->enqueue(QVREvent(QVR_Event_MouseDoubleClick, c, *event));
}

void QVRManager::enqueueWheelEvent(const QVRRenderContext& c, QWheelEvent* event)
{
    eventQueue->enqueue(QVREvent(QVR_Event_Wheel, c, *event));
}

void QVRManager::processEventQueue()
{
    while (!eventQueue->empty()) {
        QVREvent e = eventQueue->front();
        eventQueue->dequeue();
        if (_haveWasdqeObservers
                && observerConfig(windowConfig(e.context.processIndex(), e.context.windowIndex())
                    .observerIndex()).navigationType() == QVR_Navigation_WASDQE) {
            bool consumed = false;
            if (e.type == QVR_Event_KeyPress) {
                switch (e.keyEvent.key()) {
                case Qt::Key_Escape:
                    if (_wasdqeMouseProcessIndex >= 0) {
                        _wasdqeMouseProcessIndex = -1;
                        _wasdqeMouseWindowIndex = -1;
                        _wasdqeMouseInitialized = false;
                        consumed = true;
                    }
                    break;
                case Qt::Key_W:
                    _wasdqeIsPressed[0] = true;
                    consumed = true;
                    break;
                case Qt::Key_A:
                    _wasdqeIsPressed[1] = true;
                    consumed = true;
                    break;
                case Qt::Key_S:
                    _wasdqeIsPressed[2] = true;
                    consumed = true;
                    break;
                case Qt::Key_D:
                    _wasdqeIsPressed[3] = true;
                    consumed = true;
                    break;
                case Qt::Key_Q:
                    _wasdqeIsPressed[4] = true;
                    consumed = true;
                    break;
                case Qt::Key_E:
                    _wasdqeIsPressed[5] = true;
                    consumed = true;
                    break;
                }
            } else if (e.type == QVR_Event_KeyRelease) {
                switch (e.keyEvent.key())
                {
                case Qt::Key_W:
                    _wasdqeIsPressed[0] = false;
                    consumed = true;
                    break;
                case Qt::Key_A:
                    _wasdqeIsPressed[1] = false;
                    consumed = true;
                    break;
                case Qt::Key_S:
                    _wasdqeIsPressed[2] = false;
                    consumed = true;
                    break;
                case Qt::Key_D:
                    _wasdqeIsPressed[3] = false;
                    consumed = true;
                    break;
                case Qt::Key_Q:
                    _wasdqeIsPressed[4] = false;
                    consumed = true;
                    break;
                case Qt::Key_E:
                    _wasdqeIsPressed[5] = false;
                    consumed = true;
                    break;
                }
            } else if (e.type == QVR_Event_MousePress) {
                _wasdqeMouseProcessIndex = e.context.processIndex();
                _wasdqeMouseWindowIndex = e.context.windowIndex();
                _wasdqeMouseInitialized = false;
                consumed = true;
            } else if (e.type == QVR_Event_MouseMove) {
                if (_wasdqeMouseInitialized
                        && _wasdqeMouseProcessIndex == e.context.processIndex()
                        && _wasdqeMouseWindowIndex == e.context.windowIndex()) {
                    // Horizontal angle
                    float x = e.mouseEvent.pos().x();
                    float w = e.context.windowGeometry().width();
                    float xf = x / w * 2.0f - 1.0f;
                    _wasdqeHorzAngle = -xf * 180.0f;
                    // Vertical angle
                    // For HMDs, up/down views are realized via head movements. Additional
                    // mouse-based up/down views should be disabled since they lead to
                    // sickness fast ;)
                    if (windowConfig(e.context.processIndex(), e.context.windowIndex()).outputMode()
                            != QVR_Output_Stereo_Oculus) {
                        float y = e.mouseEvent.pos().y();
                        float h = e.context.windowGeometry().height();
                        float yf = y / h * 2.0f - 1.0f;
                        _wasdqeVertAngle = -yf * 90.0f;
                    }
                    consumed = true;
                }
            }
            if (consumed) {
                continue;
            }
        }
        switch (e.type) {
        case QVR_Event_KeyPress:
            _app->keyPressEvent(e.context, &e.keyEvent);
            break;
        case QVR_Event_KeyRelease:
            _app->keyReleaseEvent(e.context, &e.keyEvent);
            break;
        case QVR_Event_MouseMove:
            _app->mouseMoveEvent(e.context, &e.mouseEvent);
            break;
        case QVR_Event_MousePress:
            _app->mousePressEvent(e.context, &e.mouseEvent);
            break;
        case QVR_Event_MouseRelease:
            _app->mouseReleaseEvent(e.context, &e.mouseEvent);
            break;
        case QVR_Event_MouseDoubleClick:
            _app->mouseDoubleClickEvent(e.context, &e.mouseEvent);
            break;
        case QVR_Event_Wheel:
            _app->wheelEvent(e.context, &e.wheelEvent);
            break;
        }
    }
}
