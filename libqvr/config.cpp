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

#ifdef HAVE_OCULUS
# include <OVR.h>
# include <OVR_CAPI_GL.h>
#endif

#include "config.hpp"
#include "manager.hpp"
#include "logging.hpp"


QVRObserverConfig::QVRObserverConfig() :
    _id(),
    _type(QVR_Observer_Stationary),
    _parameters(),
    _eyeDistance(defaultEyeDistance),
    _initialPosition(QVector3D(0.0f, defaultEyeHeight, 0.0f)),
    _initialForwardDirection(QVector3D(0.0f, 0.0f, -1.0f)),
    _initialUpDirection(QVector3D(0.0f, 1.0f, 0.0f))
{
}

QMatrix4x4 QVRObserverConfig::initialEyeMatrix(QVREye eye) const
{
    QMatrix4x4 viewMatrix;
    viewMatrix.lookAt(initialPosition(), initialPosition() + initialForwardDirection(), initialUpDirection());
    QMatrix4x4 eyeMatrix = viewMatrix.inverted();
    if (eye == QVR_Eye_Left) {
        eyeMatrix.translate(-0.5f * eyeDistance(), 0.0f, 0.0f);
    } else if (eye == QVR_Eye_Right) {
        eyeMatrix.translate(+0.5f * eyeDistance(), 0.0f, 0.0f);
    }
    return eyeMatrix;
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
    _launcher(),
    _windowConfigs()
{
}

QVRConfig::QVRConfig()
{
}

void QVRConfig::createDefault()
{
    QVR_INFO("creating default configuration ...");

    bool oculus = false;
#ifdef HAVE_OCULUS
    ovrInitParams oculusInitParams;
    std::memset(&oculusInitParams, 0, sizeof(oculusInitParams));
    oculus = ovr_Initialize(&oculusInitParams) && ovrHmd_Detect() > 0;
    ovr_Shutdown();
#endif
    if (oculus) {
        // One observer
        QVRObserverConfig observerConf;
        observerConf._id = "default";
        observerConf._type = QVR_Observer_Oculus;
        // One window
        QVRWindowConfig windowConf;
        windowConf._id = "default";
        windowConf._observerIndex = 0;
        windowConf._outputMode = QVR_Output_Stereo_Oculus;
        // One process
        QVRProcessConfig processConf;
        processConf._id = "default";
        processConf._windowConfigs.append(windowConf);
        // Put it together
        _observerConfigs.append(observerConf);
        _processConfigs.append(processConf);
    } else {
        // One observer
        QVRObserverConfig observerConf;
        observerConf._id = "default";
        observerConf._type = QVR_Observer_WASDQE;
        // One window
        QVRWindowConfig windowConf;
        windowConf._id = "default";
        windowConf._observerIndex = 0;
        windowConf._outputMode = QVR_Output_Center;
        // One process
        QVRProcessConfig processConf;
        processConf._id = "default";
        processConf._windowConfigs.append(windowConf);
        // Put it together
        _observerConfigs.append(observerConf);
        _processConfigs.append(processConf);
    }

    QVR_INFO("... done");
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
        if (observerIndex == -1) {
            // expect 'observer' keyword
            if (cmd == "observer" && arglist.length() == 1) {
                observerConfig._id = arg;
                observerIndex++;
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
            if (cmd == "type" && arglist.length() == 1
                    && (arg == "stationary" || arg == "wasdqe"
                        || arg == "vrpn" || arg == "oculus" || arg == "custom")) {
                observerConfig._type = (
                        arg == "stationary" ? QVR_Observer_Stationary
                        : arg == "wasdqe" ? QVR_Observer_WASDQE
                        : arg == "vrpn" ? QVR_Observer_VRPN
                        : arg == "oculus" ? QVR_Observer_Oculus
                        : QVR_Observer_Custom);
                continue;
            } else if (cmd == "parameters") {
                observerConfig._parameters = arg;
                continue;
            } else if (cmd == "eye_distance" && arglist.length() == 1) {
                observerConfig._eyeDistance = arg.toFloat();
                continue;
            } else if (cmd == "position" && arglist.size() == 3) {
                observerConfig._initialPosition.setX(arglist[0].toFloat());
                observerConfig._initialPosition.setY(arglist[1].toFloat());
                observerConfig._initialPosition.setZ(arglist[2].toFloat());
                continue;
            } else if (cmd == "forward" && arglist.size() == 3) {
                observerConfig._initialForwardDirection.setX(arglist[0].toFloat());
                observerConfig._initialForwardDirection.setY(arglist[1].toFloat());
                observerConfig._initialForwardDirection.setZ(arglist[2].toFloat());
                continue;
            } else if (cmd == "up" && arglist.size() == 3) {
                observerConfig._initialUpDirection.setX(arglist[0].toFloat());
                observerConfig._initialUpDirection.setY(arglist[1].toFloat());
                observerConfig._initialUpDirection.setZ(arglist[2].toFloat());
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
                            || (arglist[0] == "stereo" && arglist.length() >= 2))) {
                    windowConfig._outputMode = (
                            arglist[0] == "center" ? QVR_Output_Center
                            : arglist[0] == "left" ? QVR_Output_Left
                            : arglist[0] == "right" ? QVR_Output_Right
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
    for (int i = 0; i < _observerConfigs.size(); i++) {
        for (int j = i + 1; j < _observerConfigs.size(); j++) {
            if (_observerConfigs[i]._id == _observerConfigs[j]._id) {
                QVR_FATAL("config file %s: observer id %s is not unique",
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
        bool haveOculusWindow = false;
        for (int j = 0; j < _processConfigs[i]._windowConfigs.size(); j++) {
            if (_processConfigs[i]._windowConfigs[j]._outputMode == QVR_Output_Stereo_Oculus) {
                if (i > 0) {
                    QVR_FATAL("config file %s: currently only the master process "
                            "can have a window with output stereo oculus", qPrintable(filename));
                    return false;
                }
                if (haveOculusWindow) {
                    QVR_FATAL("config file %s: process %s: "
                            "more than one window has stereo_mode oculus",
                            qPrintable(filename), qPrintable(_processConfigs[i]._id));
                    return false;
                }
                haveOculusWindow = true;
            }
            if (_processConfigs[i]._windowConfigs[j]._observerIndex < 0) {
                QVR_FATAL("config file %s: window id %s does not have a valid observer",
                        qPrintable(filename), qPrintable(_processConfigs[i]._windowConfigs[j]._id));
                return false;
            }
            if (_observerConfigs[_processConfigs[i]._windowConfigs[j]._observerIndex]._type == QVR_Observer_Oculus
                    && _processConfigs[i]._windowConfigs[j]._outputMode != QVR_Output_Stereo_Oculus) {
                    QVR_FATAL("config file %s: process %s: "
                            "only windows with output stereo oculus can have observers of type oculus",
                            qPrintable(filename), qPrintable(_processConfigs[i]._id));
                    return false;
            }
        }
    }
    return true;
}

