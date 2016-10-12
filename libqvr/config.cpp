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

#include <cstring>

#include <QFile>
#include <QTextStream>

#include "config.hpp"
#include "manager.hpp"
#include "logging.hpp"


QVRDeviceConfig::QVRDeviceConfig() :
    _id(),
    _processIndex(0),
    _trackingType(QVR_Device_Tracking_None),
    _trackingParameters(),
    _buttonsType(QVR_Device_Buttons_None),
    _buttonsParameters(),
    _analogsType(QVR_Device_Analogs_None),
    _analogsParameters()
{
}

QVRObserverConfig::QVRObserverConfig() :
    _id(),
    _navigationType(QVR_Navigation_Stationary),
    _navigationParameters(),
    _trackingType(QVR_Tracking_Stationary),
    _trackingParameters(),
    _initialNavigationPosition(QVector3D(0.0f, 0.0f, 0.0f)),
    _initialNavigationForwardDirection(QVector3D(0.0f, 0.0f, -1.0f)),
    _initialNavigationUpDirection(QVector3D(0.0f, 1.0f, 0.0f)),
    _initialEyeDistance(defaultEyeDistance),
    _initialTrackingPosition(QVector3D(0.0f, defaultEyeHeight, 0.0f)),
    _initialTrackingForwardDirection(QVector3D(0.0f, 0.0f, -1.0f)),
    _initialTrackingUpDirection(QVector3D(0.0f, 1.0f, 0.0f))
{
}

QVRWindowConfig::QVRWindowConfig() :
    _id(),
    _observerIndex(-1),
    _outputMode(QVR_Output_Center),
    _outputPlugin(),
    _initialDisplayScreen(-1),
    _initialFullscreen(false),
    _initialPosition(-1, -1),
    _initialSize(800, 600),
    _screenIsFixedToObserver(true),
    _screenCornerBottomLeft(QVector3D(0.0f, 0.0f, 0.0f)),
    _screenCornerBottomRight(QVector3D(0.0f, 0.0f, 0.0f)),
    _screenCornerTopLeft(QVector3D(0.0f, 0.0f, 0.0f)),
    _screenIsGivenByCenter(true),
    _screenCenter(QVector3D(0.0f, 0.0f, -1.0f)),
    _renderResolutionFactor(1.0f)
{
}

QVRProcessConfig::QVRProcessConfig() :
    _id(),
    _display(),
    _address(),
    _launcher(),
    _windowConfigs()
{
}

QVRConfig::QVRConfig()
{
}

void QVRConfig::createDefault(QVRNavigationType preferredNavigationType)
{
    QVR_INFO("creating default configuration");

#ifdef HAVE_OCULUS
    QVRAttemptOculusInitialization();
    if (QVROculus) {
        // Devices
        QVRDeviceConfig deviceConf0, deviceConf1, deviceConf2;
        deviceConf0._id = "oculus-head";
        deviceConf0._trackingType = QVR_Device_Tracking_Oculus;
        deviceConf1._trackingParameters = "head";
        deviceConf1._id = "oculus-eye-left";
        deviceConf1._trackingType = QVR_Device_Tracking_Oculus;
        deviceConf1._trackingParameters = "eye-left";
        deviceConf2._id = "oculus-eye-right";
        deviceConf2._trackingType = QVR_Device_Tracking_Oculus;
        deviceConf2._trackingParameters = "eye-right";
        // One observer
        QVRObserverConfig observerConf;
        observerConf._id = "oculus-observer";
        observerConf._navigationType = preferredNavigationType;
        observerConf._trackingType = QVR_Tracking_Device;
        observerConf._trackingParameters = "oculus-eye-left oculus-eye-right";
        // One window
        QVRWindowConfig windowConf;
        windowConf._id = "oculus-window";
        windowConf._observerIndex = 0;
        windowConf._outputMode = QVR_Output_Stereo_Oculus;
        // One process
        QVRProcessConfig processConf;
        processConf._id = "oculus-process";
        processConf._windowConfigs.append(windowConf);
        // Put it together
        _deviceConfigs.append(deviceConf0);
        _deviceConfigs.append(deviceConf1);
        _deviceConfigs.append(deviceConf2);
        _observerConfigs.append(observerConf);
        _processConfigs.append(processConf);
        return;
    }
#endif
#ifdef HAVE_OSVR
    QVRAttemptOSVRInitialization();
    if (QVROsvrClientContext) {
        OSVR_EyeCount eyes;
        osvrClientGetNumEyesForViewer(QVROsvrDisplayConfig, 0, &eyes);
        // Devices
        QVRDeviceConfig deviceConf0, deviceConf1, deviceConf2;
        deviceConf0._id = "osvr-head";
        deviceConf0._trackingType = QVR_Device_Tracking_OSVR;
        deviceConf0._trackingParameters = "/me/head";
        if (eyes == 2) {
            deviceConf1._id = "osvr-eye-left";
            deviceConf1._trackingType = QVR_Device_Tracking_OSVR;
            deviceConf1._trackingParameters = "eye-left";
            deviceConf2._id = "osvr-eye-right";
            deviceConf2._trackingType = QVR_Device_Tracking_OSVR;
            deviceConf2._trackingParameters = "eye-right";
        } else { // eyes == 1
            deviceConf1._id = "osvr-eye-center";
            deviceConf1._trackingType = QVR_Device_Tracking_OSVR;
            deviceConf1._trackingParameters = "eye-center";
        }
        // One observer
        QVRObserverConfig observerConf;
        observerConf._id = "osvr-observer";
        observerConf._navigationType = preferredNavigationType;
        observerConf._trackingType = QVR_Tracking_Device;
        if (eyes == 2)
            observerConf._trackingParameters = "osvr-eye-left osvr-eye-right";
        else
            observerConf._trackingParameters = "osvr-eye-center";
        // One window
        QVRWindowConfig windowConf;
        windowConf._id = "osvr-window";
        windowConf._observerIndex = 0;
        windowConf._outputMode = QVR_Output_OSVR;
        // One process
        QVRProcessConfig processConf;
        processConf._id = "osvr-process";
        processConf._windowConfigs.append(windowConf);
        // Put it together
        _deviceConfigs.append(deviceConf0);
        _deviceConfigs.append(deviceConf1);
        if (eyes == 2)
            _deviceConfigs.append(deviceConf2);
        _observerConfigs.append(observerConf);
        _processConfigs.append(processConf);
        return;
    }
#endif
    // One observer
    QVRObserverConfig observerConf;
    observerConf._id = "qvr-default";
    observerConf._navigationType = preferredNavigationType;
    observerConf._trackingType = QVR_Tracking_Custom;
    // One window
    QVRWindowConfig windowConf;
    windowConf._id = "qvr-default";
    windowConf._observerIndex = 0;
    windowConf._outputMode = QVR_Output_Center;
    // One process
    QVRProcessConfig processConf;
    processConf._id = "qvr-default";
    processConf._windowConfigs.append(windowConf);
    // Put it together
    _observerConfigs.append(observerConf);
    _processConfigs.append(processConf);
}

bool QVRConfig::readFromFile(const QString& filename)
{
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly)) {
        QVR_FATAL("config file %s: cannot open", qPrintable(filename));
        return false;
    }
    QTextStream in(&f);

    int lineCounter = 0;

    int deviceIndex = -1;
    QVRDeviceConfig deviceConfig;
    QString deviceProcessId;
    QStringList deviceProcessIds;
    int observerIndex = -1;
    QVRObserverConfig observerConfig;
    int processIndex = -1;
    QVRProcessConfig processConfig;
    int windowIndex = -1;
    QVRWindowConfig windowConfig;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        lineCounter++;
        if (line.isEmpty() || line.startsWith('#'))
            continue;
        QString cmd, arg;
        QStringList arglist;
        int i = 0;
        while (line[i] != ' ' && line[i] != '\t')
            cmd.append(line[i++]);
        arg = line.mid(i).trimmed();
        arglist = arg.split(' ', QString::SkipEmptyParts);
        if (deviceIndex == -1 && observerIndex == -1) {
            // expect 'device' keyword...
            if (cmd == "device" && arglist.length() == 1) {
                deviceConfig._id = arg;
                deviceProcessId.clear();
                deviceIndex++;
                continue;
            }
            // ... or 'observer' keyword
            if (cmd == "observer" && arglist.length() == 1) {
                observerConfig._id = arg;
                observerIndex++;
                continue;
            }
        }
        if (deviceIndex >= 0 && observerIndex == -1) {
            // expect 'device' keyword...
            if (cmd == "device" && arglist.length() == 1) {
                // commit current device
                _deviceConfigs.append(deviceConfig);
                deviceProcessIds.append(deviceProcessId);
                // start new device
                deviceConfig = QVRDeviceConfig();
                deviceConfig._id = arg;
                deviceProcessId.clear();
                deviceIndex++;
                continue;
            }
            // ... or 'observer' keyword...
            if (cmd == "observer" && arglist.length() == 1) {
                // commit current device
                _deviceConfigs.append(deviceConfig);
                deviceProcessIds.append(deviceProcessId);
                // start first observer
                observerConfig._id = arg;
                observerIndex++;
                continue;
            }
            // ... or device properties.
            if (cmd == "process" && arglist.length() >= 1) {
                deviceProcessId = arg;
                continue;
            } else if (cmd == "tracking" && arglist.length() >= 1
                    && (arglist[0] == "none" || arglist[0] == "static"
                        || arglist[0] == "oculus" || arglist[0] == "vrpn"
                        || arglist[0] == "osvr")) {
                deviceConfig._trackingType = (
                        arglist[0] == "none" ? QVR_Device_Tracking_None
                        : arglist[0] == "static" ? QVR_Device_Tracking_Static
                        : arglist[0] == "oculus" ? QVR_Device_Tracking_Oculus
                        : arglist[0] == "vrpn" ? QVR_Device_Tracking_VRPN
                        : QVR_Device_Tracking_OSVR);
                deviceConfig._trackingParameters = QStringList(arglist.mid(1)).join(' ');
                continue;
            } else if (cmd == "buttons" && arglist.length() >= 1
                    && (arglist[0] == "none" || arglist[0] == "static" || arglist[0] == "vrpn")) {
                deviceConfig._buttonsType = (
                        arglist[0] == "none" ? QVR_Device_Buttons_None
                        : arglist[0] == "static" ? QVR_Device_Buttons_Static
                        : QVR_Device_Buttons_VRPN);
                deviceConfig._buttonsParameters = QStringList(arglist.mid(1)).join(' ');
                continue;
            } else if (cmd == "analogs" && arglist.length() >= 1
                    && (arglist[0] == "none" || arglist[0] == "static" || arglist[0] == "vrpn")) {
                deviceConfig._analogsType = (
                        arglist[0] == "none" ? QVR_Device_Analogs_None
                        : arglist[0] == "static" ? QVR_Device_Analogs_Static
                        : QVR_Device_Analogs_VRPN);
                deviceConfig._analogsParameters = QStringList(arglist.mid(1)).join(' ');
                continue;
            }
        }
        if (observerIndex >= 0 && processIndex == -1) {
            // expect 'observer' keyword...
            if (cmd == "observer" && arglist.length() == 1) {
                // commit current observer
                _observerConfigs.append(observerConfig);
                // start new observer
                observerConfig = QVRObserverConfig();
                observerConfig._id = arg;
                observerIndex++;
                continue;
            }
            // ... or 'process' keyword...
            if (cmd == "process" && arglist.length() == 1) {
                // commit current observer
                _observerConfigs.append(observerConfig);
                // start first process
                processConfig._id = arg;
                processIndex++;
                continue;
            }
            // ... or observer properties.
            if (cmd == "navigation" && arglist.length() >= 1
                    && (arglist[0] == "stationary" || arglist[0] == "device"
                        || arglist[0] == "wasdqe" || arglist[0] == "custom")) {
                observerConfig._navigationType = (
                        arglist[0] == "stationary" ? QVR_Navigation_Stationary
                        : arglist[0] == "device" ? QVR_Navigation_Device
                        : arglist[0] == "wasdqe" ? QVR_Navigation_WASDQE
                        : QVR_Navigation_Custom);
                observerConfig._navigationParameters = QStringList(arglist.mid(1)).join(' ');
                continue;
            } else if (cmd == "tracking" && arglist.length() >= 1
                    && (arglist[0] == "stationary" || arglist[0] == "device"
                        || arglist[0] == "oculus" || arglist[0] == "custom")) {
                observerConfig._trackingType = (
                        arglist[0] == "stationary" ? QVR_Tracking_Stationary
                        : arglist[0] == "device" ? QVR_Tracking_Device
                        : QVR_Tracking_Custom);
                observerConfig._trackingParameters = QStringList(arglist.mid(1)).join(' ');
                continue;
            } else if (cmd == "navigation_position" && arglist.size() == 3) {
                observerConfig._initialNavigationPosition.setX(arglist[0].toFloat());
                observerConfig._initialNavigationPosition.setY(arglist[1].toFloat());
                observerConfig._initialNavigationPosition.setZ(arglist[2].toFloat());
                continue;
            } else if (cmd == "navigation_forward" && arglist.size() == 3) {
                observerConfig._initialNavigationForwardDirection.setX(arglist[0].toFloat());
                observerConfig._initialNavigationForwardDirection.setY(arglist[1].toFloat());
                observerConfig._initialNavigationForwardDirection.setZ(arglist[2].toFloat());
                continue;
            } else if (cmd == "navigation_up" && arglist.size() == 3) {
                observerConfig._initialNavigationUpDirection.setX(arglist[0].toFloat());
                observerConfig._initialNavigationUpDirection.setY(arglist[1].toFloat());
                observerConfig._initialNavigationUpDirection.setZ(arglist[2].toFloat());
                continue;
            } else if (cmd == "eye_distance" && arglist.length() == 1) {
                observerConfig._initialEyeDistance = arg.toFloat();
                continue;
            } else if (cmd == "tracking_position" && arglist.size() == 3) {
                observerConfig._initialTrackingPosition.setX(arglist[0].toFloat());
                observerConfig._initialTrackingPosition.setY(arglist[1].toFloat());
                observerConfig._initialTrackingPosition.setZ(arglist[2].toFloat());
                continue;
            } else if (cmd == "tracking_forward" && arglist.size() == 3) {
                observerConfig._initialTrackingForwardDirection.setX(arglist[0].toFloat());
                observerConfig._initialTrackingForwardDirection.setY(arglist[1].toFloat());
                observerConfig._initialTrackingForwardDirection.setZ(arglist[2].toFloat());
                continue;
            } else if (cmd == "tracking_up" && arglist.size() == 3) {
                observerConfig._initialTrackingUpDirection.setX(arglist[0].toFloat());
                observerConfig._initialTrackingUpDirection.setY(arglist[1].toFloat());
                observerConfig._initialTrackingUpDirection.setZ(arglist[2].toFloat());
                continue;
            }
        }
        if (processIndex >= 0) {
            // expect 'process' keyword...
            if (cmd == "process" && arglist.length() == 1) {
                // commit current window (if any)
                if (windowIndex >= 0)
                    processConfig._windowConfigs.append(windowConfig);
                // commit current process
                _processConfigs.append(processConfig);
                // start new process with no window
                processConfig = QVRProcessConfig();
                processConfig._id = arg;
                processIndex++;
                windowIndex = -1;
                continue;
            }
            // ... or 'window' keyword...
            if (cmd == "window" && arglist.length() == 1) {
                // commit current window (if any)
                if (windowIndex >= 0)
                    processConfig._windowConfigs.append(windowConfig);
                // start new window
                windowConfig = QVRWindowConfig();
                windowConfig._id = arg;
                windowIndex++;
                continue;
            }
            // or process or window properties.
            if (windowIndex == -1) {
                // process properties:
                if (cmd == "display" && arglist.length() == 1) {
                    processConfig._display = arg;
                    continue;
                }
                if (cmd == "address" && arglist.length() >= 1) {
                    processConfig._address = arg;
                    continue;
                }
                if (cmd == "launcher" && arglist.length() >= 1) {
                    processConfig._launcher = arg;
                    continue;
                }
            } else {
                // window properties:
                if (cmd == "observer" && arglist.length() == 1) {
                    int i;
                    QString oid = arg;
                    for (i = 0; i < _observerConfigs.size(); i++) {
                        if (_observerConfigs[i]._id == oid) {
                            windowConfig._observerIndex = i;
                            break;
                        }
                    }
                    continue;
                }
                if (cmd == "output" && arglist.length() >= 1
                        && (arglist[0] == "center"
                            || arglist[0] == "left"
                            || arglist[0] == "right"
                            || arglist[0] == "osvr"
                            || (arglist[0] == "stereo" && arglist.length() >= 2))) {
                    windowConfig._outputMode = (
                            arglist[0] == "center" ? QVR_Output_Center
                            : arglist[0] == "left" ? QVR_Output_Left
                            : arglist[0] == "right" ? QVR_Output_Right
                            : arglist[0] == "osvr" ? QVR_Output_OSVR
                            : arglist[1] == "gl" ? QVR_Output_Stereo_GL
                            : arglist[1] == "red_cyan" ? QVR_Output_Stereo_Red_Cyan
                            : arglist[1] == "green_magenta" ? QVR_Output_Stereo_Green_Magenta
                            : arglist[1] == "amber_blue" ? QVR_Output_Stereo_Amber_Blue
                            : arglist[1] == "oculus" ? QVR_Output_Stereo_Oculus
                            : QVR_Output_Stereo_Custom);
                    windowConfig._outputPlugin = QString();
                    if (arglist.length() > 1
                            && (arglist[0] != "stereo" || windowConfig._outputMode == QVR_Output_Stereo_Custom)) {
                        windowConfig._outputPlugin = arglist.mid(1).join(' ');
                    }
                    continue;
                }
                if (cmd == "display_screen" && arglist.length() == 1) {
                    windowConfig._initialDisplayScreen = arg.toInt();
                    continue;
                }
                if (cmd == "fullscreen" && arglist.length() == 1
                        && (arg == "true" || arg == "false")) {
                    windowConfig._initialFullscreen = (arg == "true");
                    continue;
                }
                if (cmd == "position" && arglist.length() == 2) {
                    windowConfig._initialPosition = QPoint(arglist[0].toInt(), arglist[1].toInt());
                    continue;
                }
                if (cmd == "size" && arglist.length() == 2) {
                    windowConfig._initialSize = QSize(arglist[0].toInt(), arglist[1].toInt());
                    continue;
                }
                if (cmd == "screen_is_fixed_to_observer" && arglist.length() == 1
                        && (arg == "true" || arg == "false")) {
                    windowConfig._screenIsFixedToObserver = (arg == "true");
                    continue;
                }
                if (cmd == "screen_wall" && arglist.length() == 9) {
                    windowConfig._screenCornerBottomLeft.setX(arglist[0].toFloat());
                    windowConfig._screenCornerBottomLeft.setY(arglist[1].toFloat());
                    windowConfig._screenCornerBottomLeft.setZ(arglist[2].toFloat());
                    windowConfig._screenCornerBottomRight.setX(arglist[3].toFloat());
                    windowConfig._screenCornerBottomRight.setY(arglist[4].toFloat());
                    windowConfig._screenCornerBottomRight.setZ(arglist[5].toFloat());
                    windowConfig._screenCornerTopLeft.setX(arglist[6].toFloat());
                    windowConfig._screenCornerTopLeft.setY(arglist[7].toFloat());
                    windowConfig._screenCornerTopLeft.setZ(arglist[8].toFloat());
                    continue;
                }
                if (cmd == "screen_is_given_by_center" && arglist.length() == 1
                        && (arg == "true" || arg == "false")) {
                    windowConfig._screenIsGivenByCenter = (arg == "true");
                    continue;
                }
                if (cmd == "screen_center" && arglist.length() == 3) {
                    windowConfig._screenCenter.setX(arglist[0].toFloat());
                    windowConfig._screenCenter.setY(arglist[1].toFloat());
                    windowConfig._screenCenter.setZ(arglist[2].toFloat());
                    continue;
                }
                if (cmd == "render_resolution_factor" && arglist.length() == 1) {
                    windowConfig._renderResolutionFactor = arg.toFloat();
                    continue;
                }
            }
        }
        QVR_FATAL("config file %s: invalid line %d", qPrintable(filename), lineCounter);
        return false;
    }
    f.close();
    if (observerIndex >= 0 && processIndex == -1) {
        _observerConfigs.append(observerConfig);
    }
    if (windowIndex >= 0) {
        processConfig._windowConfigs.append(windowConfig);
    }
    if (processIndex >= 0) {
        _processConfigs.append(processConfig);
    }

    // Sanity checks
    if (_observerConfigs.size() == 0) {
        QVR_FATAL("config file %s: no observers defined", qPrintable(filename));
        return false;
    }
    if (_processConfigs.size() == 0) {
        QVR_FATAL("config file %s: no processes defined", qPrintable(filename));
        return false;
    }
    int windowCount = 0;
    for (int i = 0; i < _processConfigs.size(); i++)
        windowCount += _processConfigs[i].windowConfigs().size();
    if (windowCount == 0) {
        QVR_FATAL("config file %s: no windows defined", qPrintable(filename));
        return false;
    }
    if (!_processConfigs[0]._display.isEmpty()) {
        QVR_FATAL("config file %s: the first process is the master process "
                "and must not have the display property set", qPrintable(filename));
        return false;
    }
    for (int i = 0; i < _deviceConfigs.size(); i++) {
        for (int j = i + 1; j < _deviceConfigs.size(); j++) {
            if (_deviceConfigs[i]._id == _deviceConfigs[j]._id) {
                QVR_FATAL("config file %s: device id %s is not unique",
                        qPrintable(filename), qPrintable(_deviceConfigs[i]._id));
                return false;
            }
        }
        // fill in process indices from our separate list of process ids
        if (!deviceProcessIds[i].isEmpty()) {
            int j;
            for (j = 0; j < _processConfigs.size(); j++) {
                if (_processConfigs[j]._id == deviceProcessIds[i])
                    break;
            }
            if (j == _processConfigs.size()) {
                QVR_FATAL("config file %s: device %s: process %s does not exist",
                        qPrintable(filename), qPrintable(_deviceConfigs[i]._id),
                        qPrintable(deviceProcessIds[i]));
                return false;
            } else {
                _deviceConfigs[i]._processIndex = j;
            }
        }
    }
    for (int i = 0; i < _observerConfigs.size(); i++) {
        for (int j = i + 1; j < _observerConfigs.size(); j++) {
            if (_observerConfigs[i]._id == _observerConfigs[j]._id) {
                QVR_FATAL("config file %s: observer id %s is not unique",
                        qPrintable(filename), qPrintable(_observerConfigs[i]._id));
                return false;
            }
        }
        if (_observerConfigs[i]._navigationType == QVR_Navigation_Device) {
            int j;
            for (j = 0; j < _deviceConfigs.size(); j++) {
                if (_observerConfigs[i]._navigationParameters == _deviceConfigs[j]._id)
                    break;
            }
            if (j == _deviceConfigs.size()) {
                QVR_FATAL("config file %s: observer %s uses nonexistent device for navigation",
                        qPrintable(filename), qPrintable(_observerConfigs[i]._id));
                return false;
            }
        }
        if (_observerConfigs[i]._trackingType == QVR_Tracking_Device) {
            QString devId = _observerConfigs[i]._trackingParameters.trimmed();
            QStringList devIdList = devId.split(' ', QString::SkipEmptyParts);
            int trackDev0 = -1;
            int trackDev1 = -1;
            for (int j = 0; j < _deviceConfigs.size(); j++) {
                if (devIdList.size() == 2) {
                    if (_deviceConfigs[j]._id == devIdList[0])
                        trackDev0 = j;
                    else if (_deviceConfigs[j]._id == devIdList[1])
                        trackDev1 = j;
                } else {
                    if (_deviceConfigs[j]._id == devId)
                        trackDev0 = j;
                }
            }
            if (trackDev0 == -1 || (devIdList.size() == 2 && trackDev1 == -1)) {
                QVR_FATAL("config file %s: observer %s uses nonexistent device for tracking",
                        qPrintable(filename), qPrintable(_observerConfigs[i]._id));
                return false;
            }
        }
    }
    for (int i = 0; i < _processConfigs.size(); i++) {
        for (int j = i + 1; j < _processConfigs.size(); j++) {
            if (_processConfigs[i]._id == _processConfigs[j]._id) {
                QVR_FATAL("config file %s: process id %s is not unique",
                        qPrintable(filename), qPrintable(_processConfigs[i]._id));
                return false;
            }
        }
    }
    QSet<QString> windowIds;
    for (int i = 0; i < _processConfigs.size(); i++) {
        for (int j = 0; j < _processConfigs[i].windowConfigs().size(); j++) {
            const QString& windowId = _processConfigs[i]._windowConfigs[j]._id;
            if (windowIds.contains(windowId)) {
                QVR_FATAL("config file %s: window id %s is not unique",
                        qPrintable(filename), qPrintable(windowId));
                return false;
            } else {
                windowIds.insert(windowId);
            }
        }
    }
    for (int i = 0; i < _processConfigs.size(); i++) {
        for (int j = 0; j < _processConfigs[i]._windowConfigs.size(); j++) {
            if (_processConfigs[i]._windowConfigs[j]._observerIndex < 0) {
                QVR_FATAL("config file %s: window %s does not have a valid observer",
                        qPrintable(filename), qPrintable(_processConfigs[i]._windowConfigs[j]._id));
                return false;
            }
        }
    }
    return true;
}

