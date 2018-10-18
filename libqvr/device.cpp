/*
 * Copyright (C) 2016, 2017, 2018 Computer Graphics Group, University of Siegen
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
#include <QtMath>

#include "manager.hpp"
#include "device.hpp"
#include "logging.hpp"
#include "internalglobals.hpp"

#ifdef HAVE_QGAMEPAD
# include <QGamepad>
#endif
#ifdef HAVE_VRPN
# include <vrpn_Tracker.h>
# include <vrpn_Analog.h>
# include <vrpn_Button.h>
#endif
#ifdef HAVE_OCULUS
# if (OVR_PRODUCT_VERSION >= 1)
#  include <QTimer>
# endif
#endif


struct QVRDeviceInternals {
    // Last known position and orientation, with timestamp.
    // These are used to calculate velocity and angular velocity.
    // If the timestamp is -1 then we do not have known values yet.
    qint64 currentTimestamp;
    qint64 lastTimestamp;
    QVector3D lastPosition;
    QQuaternion lastOrientation;
#ifdef HAVE_QGAMEPAD
    QGamepad* buttonsGamepad; // these pointers might actually...
    QGamepad* analogsGamepad; // ...point to the same gamepad object!
#endif
#ifdef HAVE_VRPN
    QVector3D* vrpnPositionPtr; // pointer to _position
    QQuaternion* vrpnOrientationPtr; // pointer to _orientation
    QVector3D* vrpnVelocityPtr; // pointer to _velocity
    QVector3D* vrpnAngularVelocityPtr; // pointer to _angularVelocity;
    bool vrpnHaveVelocity;
    QVector<bool>* vrpnButtonsPtr; // pointer to _buttons
    QVector<float>* vrpnAnalogsPtr; // pointer to _analogs
    vrpn_Tracker_Remote* vrpnTrackerRemote;
    vrpn_Button_Remote* vrpnButtonRemote;
    vrpn_Analog_Remote* vrpnAnalogRemote;
#endif
#ifdef HAVE_OCULUS
    int oculusTrackedEntity; // -1 = none, 0 = center/head, 1 = left eye, 2 = right eye, 3 = left controller, 4 = right controller
    int oculusButtonsEntity; // -1 = none, 0 = xbox, 1 = touch left, 2 = touch right
    int oculusAnalogsEntity; // -1 = none, 0 = xbox, 1 = touch left, 2 = touch right
#endif
#ifdef HAVE_OPENVR
    int openVrTrackedEntity; // -1 = none, 0 = center/head, 1 = left eye, 2 = right eye, 3 = controller0, 4 = controller1
    int openVrButtonsEntity; // -1 = none, 0 = controller0, 1 = controller1
    int openVrAnalogsEntity; // -1 = none, 0 = controller0, 1 = controller1
#endif
#ifdef ANDROID
    int googleVrTrackedEntity; // -1 = none, 0 = left eye, 1 = right eye, 2 = head
#endif
};

static QVector3D QVRAngularVelocityFromDiffQuaternion(const QQuaternion& q, double seconds)
{
    QVector3D axis;
    float angle;
    q.getAxisAndAngle(&axis, &angle);
    return axis * qDegreesToRadians(angle) / seconds;
}

#ifdef HAVE_VRPN
void QVRVrpnTrackerChangeHandler(void* userdata, const vrpn_TRACKERCB info)
{
    struct QVRDeviceInternals* d = reinterpret_cast<struct QVRDeviceInternals*>(userdata);
    *(d->vrpnPositionPtr) = QVector3D(info.pos[0], info.pos[1], info.pos[2]);
    *(d->vrpnOrientationPtr) = QQuaternion(info.quat[3], info.quat[0], info.quat[1], info.quat[2]);
}
void QVRVrpnTrackerVelocityChangeHandler(void* userdata, const vrpn_TRACKERVELCB info)
{
    struct QVRDeviceInternals* d = reinterpret_cast<struct QVRDeviceInternals*>(userdata);
    *(d->vrpnVelocityPtr) = QVector3D(info.vel[0], info.vel[1], info.vel[2]);
    *(d->vrpnAngularVelocityPtr) = QVRAngularVelocityFromDiffQuaternion(
            QQuaternion(info.vel_quat[3], info.vel_quat[0], info.vel_quat[1], info.vel_quat[2]),
            info.vel_quat_dt);
    d->vrpnHaveVelocity = true;
}
void QVRVrpnButtonChangeHandler(void* userdata, const vrpn_BUTTONCB info)
{
    struct QVRDeviceInternals* d = reinterpret_cast<struct QVRDeviceInternals*>(userdata);
    if (info.button >= 0 && info.button < d->vrpnButtonsPtr->size())
        (*(d->vrpnButtonsPtr))[info.button] = info.state;
}
void QVRVrpnAnalogChangeHandler(void* userdata, const vrpn_ANALOGCB info)
{
    struct QVRDeviceInternals* d = reinterpret_cast<struct QVRDeviceInternals*>(userdata);
    for (int i = 0; i < d->vrpnAnalogsPtr->size(); i++) {
        if (i < info.num_channel)
            (*(d->vrpnAnalogsPtr))[i] = info.channel[i];
    }
}
#endif

static bool QVRButtonFromName(const QString& name, QVRButton* btn)
{
    bool ret = true;
    if (name == "l1")
        *btn = QVR_Button_L1;
    else if (name == "l2")
        *btn = QVR_Button_L2;
    else if (name == "l3")
        *btn = QVR_Button_L3;
    else if (name == "r1")
        *btn = QVR_Button_R1;
    else if (name == "r2")
        *btn = QVR_Button_R2;
    else if (name == "r3")
        *btn = QVR_Button_R3;
    else if (name == "a")
        *btn = QVR_Button_A;
    else if (name == "b")
        *btn = QVR_Button_B;
    else if (name == "x")
        *btn = QVR_Button_X;
    else if (name == "y")
        *btn = QVR_Button_Y;
    else if (name == "up")
        *btn = QVR_Button_Up;
    else if (name == "down")
        *btn = QVR_Button_Down;
    else if (name == "left")
        *btn = QVR_Button_Left;
    else if (name == "right")
        *btn = QVR_Button_Right;
    else if (name == "center")
        *btn = QVR_Button_Center;
    else if (name == "select")
        *btn = QVR_Button_Select;
    else if (name == "start")
        *btn = QVR_Button_Start;
    else if (name == "menu")
        *btn = QVR_Button_Menu;
    else if (name == "back")
        *btn = QVR_Button_Back;
    else if (name == "trigger")
        *btn = QVR_Button_Trigger;
    else
        ret = false;
    return ret;
}

static bool QVRAnalogFromName(const QString& name, QVRAnalog* anlg)
{
    bool ret = true;
    if (name == "trigger")
        *anlg = QVR_Analog_Trigger;
    else if (name == "left-trigger")
        *anlg = QVR_Analog_Left_Trigger;
    else if (name == "right-trigger")
        *anlg = QVR_Analog_Right_Trigger;
    else if (name == "grip")
        *anlg = QVR_Analog_Grip;
    else if (name == "left-grip")
        *anlg = QVR_Analog_Left_Grip;
    else if (name == "right-grip")
        *anlg = QVR_Analog_Right_Grip;
    else if (name == "axis-x")
        *anlg = QVR_Analog_Axis_X;
    else if (name == "axis-y")
        *anlg = QVR_Analog_Axis_Y;
    else if (name == "left-axis-x")
        *anlg = QVR_Analog_Left_Axis_X;
    else if (name == "left-axis-y")
        *anlg = QVR_Analog_Left_Axis_Y;
    else if (name == "right-axis-x")
        *anlg = QVR_Analog_Right_Axis_X;
    else if (name == "right-axis-y")
        *anlg = QVR_Analog_Right_Axis_Y;
    else
        ret = false;
    return ret;
}

static const int QVRDeviceMaxButtons = QVR_Button_Unknown;
static const int QVRDeviceMaxAnalogs = QVR_Analog_Unknown;

QVRDevice::QVRDevice() :
    _index(-1),
    _internals(NULL)
{
    for (int i = 0; i < QVRDeviceMaxButtons; i++)
        _buttonsMap[i] = -1;
    for (int i = 0; i < QVRDeviceMaxAnalogs; i++)
        _analogsMap[i] = -1;
}

QVRDevice::QVRDevice(int deviceIndex) :
    _index(deviceIndex)
{
    for (int i = 0; i < QVRDeviceMaxButtons; i++)
        _buttonsMap[i] = -1;
    for (int i = 0; i < QVRDeviceMaxAnalogs; i++)
        _analogsMap[i] = -1;
    _internals = new struct QVRDeviceInternals;
    _internals->currentTimestamp = -1;
#ifdef HAVE_QGAMEPAD
    _internals->buttonsGamepad = NULL;
    _internals->analogsGamepad = NULL;
#endif
#ifdef HAVE_VRPN
    _internals->vrpnPositionPtr = &_position;
    _internals->vrpnOrientationPtr = &_orientation;
    _internals->vrpnVelocityPtr = &_velocity;
    _internals->vrpnAngularVelocityPtr = &_angularVelocity;
    _internals->vrpnHaveVelocity = false;
    _internals->vrpnButtonsPtr = &_buttons;
    _internals->vrpnAnalogsPtr = &_analogs;
    _internals->vrpnTrackerRemote = NULL;
    _internals->vrpnAnalogRemote = NULL;
    _internals->vrpnButtonRemote = NULL;
#endif
#ifdef HAVE_OCULUS
    _internals->oculusTrackedEntity = -1;
    _internals->oculusButtonsEntity = -1;
    _internals->oculusAnalogsEntity = -1;
#endif
#ifdef HAVE_OPENVR
    _internals->openVrTrackedEntity = -1;
    _internals->openVrButtonsEntity = -1;
    _internals->openVrAnalogsEntity = -1;
#endif
#ifdef ANDROID
    _internals->googleVrTrackedEntity = -1;
#endif

    switch (config().trackingType()) {
    case QVR_Device_Tracking_None:
        break;
    case QVR_Device_Tracking_Static:
        {
            QStringList args = config().trackingParameters().split(' ', QString::SkipEmptyParts);
            if (args.length() == 3 || args.length() == 6)
                _position = QVector3D(args[0].toFloat(), args[1].toFloat(), args[2].toFloat());
            if (args.length() == 6)
                _orientation = QQuaternion::fromEulerAngles(args[3].toFloat(), args[4].toFloat(), args[5].toFloat());
        }
        break;
    case QVR_Device_Tracking_VRPN:
#ifdef HAVE_VRPN
        if (QVRManager::processIndex() == config().processIndex()) {
            QStringList args = config().trackingParameters().split(' ', QString::SkipEmptyParts);
            QString name = (args.length() >= 1 ? args[0] : config().trackingParameters());
            int sensor = (args.length() >= 2 ? args[1].toInt() : vrpn_ALL_SENSORS);
            _internals->vrpnTrackerRemote = new vrpn_Tracker_Remote(qPrintable(name));
            vrpn_System_TextPrinter.set_ostream_to_use(stderr);
            _internals->vrpnTrackerRemote->register_change_handler(_internals, QVRVrpnTrackerChangeHandler, sensor);
            _internals->vrpnTrackerRemote->register_change_handler(_internals, QVRVrpnTrackerVelocityChangeHandler, sensor);
        }
#endif
        break;
    case QVR_Device_Tracking_Oculus:
#ifdef HAVE_OCULUS
        if (QVRManager::processIndex() == config().processIndex()) {
            QString arg = config().trackingParameters().trimmed();
            if (arg == "head")
                _internals->oculusTrackedEntity = 0;
            else if (arg == "eye-left")
                _internals->oculusTrackedEntity = 1;
            else if (arg == "eye-right")
                _internals->oculusTrackedEntity = 2;
            else if (arg == "controller-left")
                _internals->oculusTrackedEntity = 3;
            else if (arg == "controller-right")
                _internals->oculusTrackedEntity = 4;
            else
                QVR_WARNING("device %s: invalid Oculus tracking parameter", qPrintable(id()));
        }
#endif
        break;
    case QVR_Device_Tracking_OpenVR:
#ifdef HAVE_OPENVR
        if (QVRManager::processIndex() == config().processIndex()) {
            QString arg = config().trackingParameters().trimmed();
            if (arg == "head")
                _internals->openVrTrackedEntity = 0;
            else if (arg == "eye-left")
                _internals->openVrTrackedEntity = 1;
            else if (arg == "eye-right")
                _internals->openVrTrackedEntity = 2;
            else if (arg == "controller-0")
                _internals->openVrTrackedEntity = 3;
            else if (arg == "controller-1")
                _internals->openVrTrackedEntity = 4;
            else
                QVR_WARNING("device %s: invalid OpenVR tracking parameter", qPrintable(id()));
        }
#endif
        break;
    case QVR_Device_Tracking_GoogleVR:
#ifdef ANDROID
        if (QVRManager::processIndex() == config().processIndex()) {
            QString arg = config().trackingParameters().trimmed();
            if (arg == "head")
                _internals->googleVrTrackedEntity = 2;
            else if (arg == "eye-left")
                _internals->googleVrTrackedEntity = 0;
            else if (arg == "eye-right")
                _internals->googleVrTrackedEntity = 1;
            else if (arg == "daydream")
                _internals->googleVrTrackedEntity = 3;
            else
                QVR_WARNING("device %s: invalid Google VR tracking parameter", qPrintable(id()));
        }
#endif
        break;
    }

    switch (config().buttonsType()) {
    case QVR_Device_Buttons_None:
        break;
    case QVR_Device_Buttons_Static:
        {
            QStringList args = config().buttonsParameters().split(' ', QString::SkipEmptyParts);
            int n = qMin(QVRDeviceMaxButtons, args.length() / 2);
            _buttons.resize(n);
            for (int i = 0; i < _buttons.length(); i++) {
                QString name = args[2 * i + 0];
                QVRButton btn;
                if (QVRButtonFromName(name, &btn))
                    _buttonsMap[btn] = i;
                _buttons[i] = args[2 * i + 1].toInt();
            }
        }
        break;
    case QVR_Device_Buttons_Gamepad:
#ifdef HAVE_QGAMEPAD
        {
            if (QVRManager::processIndex() == config().processIndex()) {
                QString arg = config().buttonsParameters().trimmed();
                int padId = arg.toInt();
                if (padId < 0 || padId >= QVRGamepads.size()) {
                    QVR_WARNING("device %s: buttons gamepad %d is not connected", qPrintable(id()), padId);
                } else {
                    int padDeviceId = QVRGamepads[padId];
                    _internals->buttonsGamepad = new QGamepad(padDeviceId);
                    QVR_DEBUG("device %s uses gamepad %d for buttons", qPrintable(id()), padId);
                }
            }
            _buttons.resize(18);
            _buttonsMap[QVR_Button_L1] = 0;
            _buttonsMap[QVR_Button_L2] = 1;
            _buttonsMap[QVR_Button_L3] = 2;
            _buttonsMap[QVR_Button_R1] = 3;
            _buttonsMap[QVR_Button_R2] = 4;
            _buttonsMap[QVR_Button_R3] = 5;
            _buttonsMap[QVR_Button_A] = 6;
            _buttonsMap[QVR_Button_B] = 7;
            _buttonsMap[QVR_Button_X] = 8;
            _buttonsMap[QVR_Button_Y] = 9;
            _buttonsMap[QVR_Button_Up] = 10;
            _buttonsMap[QVR_Button_Down] = 11;
            _buttonsMap[QVR_Button_Left] = 12;
            _buttonsMap[QVR_Button_Right] = 13;
            _buttonsMap[QVR_Button_Center] = 14;
            _buttonsMap[QVR_Button_Select] = 15;
            _buttonsMap[QVR_Button_Start] = 16;
            _buttonsMap[QVR_Button_Menu] = 17;
        }
#endif
        break;
    case QVR_Device_Buttons_VRPN:
#ifdef HAVE_VRPN
        {
            QStringList args = config().buttonsParameters().split(' ', QString::SkipEmptyParts);
            QString name = (args.length() >= 1 ? args[0] : config().buttonsParameters());
            if (args.length() > 1) {
                _buttons.resize(qMin(QVRDeviceMaxButtons, args.length() - 1));
                QVRButton btn;
                for (int i = 0; i < _buttons.length(); i++)
                    if (QVRButtonFromName(args[i + 1], &btn))
                        _buttonsMap[btn] = i;
            } else {
                _buttons.resize(QVRDeviceMaxButtons);
            }
            if (QVRManager::processIndex() == config().processIndex()) {
                _internals->vrpnButtonRemote = new vrpn_Button_Remote(qPrintable(name));
                vrpn_System_TextPrinter.set_ostream_to_use(stderr);
                _internals->vrpnButtonRemote->register_change_handler(_internals, QVRVrpnButtonChangeHandler);
            }
        }
#endif
        break;
    case QVR_Device_Buttons_Oculus:
#ifdef HAVE_OCULUS
        {
            QString arg = config().buttonsParameters().trimmed();
            if (arg == "xbox") {
                _buttons.resize(12);
                _buttonsMap[QVR_Button_Up] = 0;
                _buttonsMap[QVR_Button_Down] = 1;
                _buttonsMap[QVR_Button_Left] = 2;
                _buttonsMap[QVR_Button_Right] = 3;
                _buttonsMap[QVR_Button_A] = 4;
                _buttonsMap[QVR_Button_B] = 5;
                _buttonsMap[QVR_Button_X] = 6;
                _buttonsMap[QVR_Button_Y] = 7;
                _buttonsMap[QVR_Button_L1] = 8;
                _buttonsMap[QVR_Button_R1] = 9;
                _buttonsMap[QVR_Button_Menu] = 10;
                _buttonsMap[QVR_Button_Back] = 11;
                if (QVRManager::processIndex() == config().processIndex()) {
                    Q_ASSERT(QVROculus);
                    _internals->oculusButtonsEntity = 0;
                }
            } else if (arg == "controller-left") {
                _buttons.resize(8);
                _buttonsMap[QVR_Button_Up] = 0;
                _buttonsMap[QVR_Button_Down] = 1;
                _buttonsMap[QVR_Button_Left] = 2;
                _buttonsMap[QVR_Button_Right] = 3;
                _buttonsMap[QVR_Button_X] = 4;
                _buttonsMap[QVR_Button_Y] = 5;
                _buttonsMap[QVR_Button_L1] = 6;
                _buttonsMap[QVR_Button_Menu] = 7;
                if (QVRManager::processIndex() == config().processIndex()) {
                    Q_ASSERT(QVROculus);
                    _internals->oculusButtonsEntity = 1;
                }
            } else if (arg == "controller-right") {
                _buttons.resize(8);
                _buttonsMap[QVR_Button_Up] = 0;
                _buttonsMap[QVR_Button_Down] = 1;
                _buttonsMap[QVR_Button_Left] = 2;
                _buttonsMap[QVR_Button_Right] = 3;
                _buttonsMap[QVR_Button_A] = 4;
                _buttonsMap[QVR_Button_B] = 5;
                _buttonsMap[QVR_Button_R1] = 6;
                _buttonsMap[QVR_Button_Menu] = 7;
                if (QVRManager::processIndex() == config().processIndex()) {
                    Q_ASSERT(QVROculus);
                    _internals->oculusButtonsEntity = 2;
                }
            } else {
                QVR_WARNING("device %s: invalid Oculus buttons parameter", qPrintable(id()));
            }
        }
#endif
        break;
    case QVR_Device_Buttons_OpenVR:
#ifdef HAVE_OPENVR
        {
            QString arg = config().buttonsParameters().trimmed();
            if (arg == "controller-0") {
                _buttons.resize(6);
                _buttonsMap[QVR_Button_Up] = 0;
                _buttonsMap[QVR_Button_Down] = 1;
                _buttonsMap[QVR_Button_Left] = 2;
                _buttonsMap[QVR_Button_Right] = 3;
                _buttonsMap[QVR_Button_Menu] = 4;
                _buttonsMap[QVR_Button_Trigger] = 5;
                if (QVRManager::processIndex() == config().processIndex()) {
                    Q_ASSERT(QVROpenVRSystem);
                    _internals->openVrButtonsEntity = 0;
                }
            } else if (arg == "controller-1") {
                _buttons.resize(6);
                _buttonsMap[QVR_Button_Up] = 0;
                _buttonsMap[QVR_Button_Down] = 1;
                _buttonsMap[QVR_Button_Left] = 2;
                _buttonsMap[QVR_Button_Right] = 3;
                _buttonsMap[QVR_Button_Menu] = 4;
                _buttonsMap[QVR_Button_Trigger] = 5;
                if (QVRManager::processIndex() == config().processIndex()) {
                    Q_ASSERT(QVROpenVRSystem);
                    _internals->openVrButtonsEntity = 1;
                }
            } else {
                QVR_WARNING("device %s: invalid OpenVR buttons parameter", qPrintable(id()));
            }
        }
#endif
        break;
    case QVR_Device_Buttons_GoogleVR:
#ifdef ANDROID
        {
            QString arg = config().buttonsParameters().trimmed();
            if (arg == "touch") {
                _buttons.resize(1);
                _buttonsMap[QVR_Button_Trigger] = 0;
            } else if (arg == "daydream") {
                _buttons.resize(3);
                _buttonsMap[QVR_Button_Trigger] = 0;
                _buttonsMap[QVR_Button_Menu] = 1;
                _buttonsMap[QVR_Button_Select] = 2;
            } else {
                QVR_WARNING("device %s: invalid GoogleVR buttons parameter", qPrintable(id()));
            }
        }
#endif
        break;
    }

    switch (config().analogsType()) {
    case QVR_Device_Analogs_None:
        break;
    case QVR_Device_Analogs_Static:
        {
            QStringList args = config().analogsParameters().split(' ', QString::SkipEmptyParts);
            _analogs.resize(args.length());
            int n = qMin(QVRDeviceMaxAnalogs, args.length() / 2);
            _buttons.resize(n);
            for (int i = 0; i < _analogs.length(); i++) {
                QString name = args[2 * i + 0];
                QVRAnalog anlg;
                if (QVRAnalogFromName(name, &anlg))
                    _analogsMap[anlg] = i;
                _analogs[i] = args[2 * i + 1].toFloat();
            }
        }
        break;
    case QVR_Device_Analogs_Gamepad:
#ifdef HAVE_QGAMEPAD
        {
            if (QVRManager::processIndex() == config().processIndex()) {
                QString arg = config().analogsParameters().trimmed();
                int padId = arg.toInt();
                if (padId < 0 || padId >= QVRGamepads.size()) {
                    QVR_WARNING("device %s: analogs gamepad %d is not connected", qPrintable(id()), padId);
                } else {
                    int padDeviceId = QVRGamepads[padId];
                    if (_internals->buttonsGamepad && padDeviceId == _internals->buttonsGamepad->deviceId())
                        _internals->analogsGamepad = _internals->buttonsGamepad;
                    else
                        _internals->analogsGamepad = new QGamepad(padDeviceId);
                    QVR_DEBUG("device %s uses gamepad %d for analogs", qPrintable(id()), padId);
                }
            }
            _analogs.resize(6);
            _analogsMap[QVR_Analog_Right_Axis_Y] = 0;
            _analogsMap[QVR_Analog_Right_Axis_X] = 1;
            _analogsMap[QVR_Analog_Left_Axis_Y] = 2;
            _analogsMap[QVR_Analog_Left_Axis_X] = 3;
            _analogsMap[QVR_Analog_Right_Trigger] = 4;
            _analogsMap[QVR_Analog_Left_Trigger] = 5;
        }
#endif
        break;
    case QVR_Device_Analogs_VRPN:
#ifdef HAVE_VRPN
        {
            QStringList args = config().analogsParameters().split(' ', QString::SkipEmptyParts);
            QString name = (args.length() >= 1 ? args[0] : config().analogsParameters());
            if (args.length() > 1) {
                _analogs.resize(qMin(QVRDeviceMaxAnalogs, args.length() - 1));
                QVRAnalog anlg;
                for (int i = 0; i < _analogs.length(); i++)
                    if (QVRAnalogFromName(args[i + 1], &anlg))
                        _analogsMap[anlg] = i;
            } else {
                _analogs.resize(QVRDeviceMaxAnalogs);
            }
            if (QVRManager::processIndex() == config().processIndex()) {
                _internals->vrpnAnalogRemote = new vrpn_Analog_Remote(qPrintable(name));
                vrpn_System_TextPrinter.set_ostream_to_use(stderr);
                _internals->vrpnAnalogRemote->register_change_handler(_internals, QVRVrpnAnalogChangeHandler);
            }
        }
#endif
        break;
    case QVR_Device_Analogs_Oculus:
#ifdef HAVE_OCULUS
        {
            QString arg = config().analogsParameters().trimmed();
            if (arg == "xbox") {
                _analogs.resize(8);
                _analogsMap[QVR_Analog_Left_Axis_Y] = 0;
                _analogsMap[QVR_Analog_Left_Axis_X] = 1;
                _analogsMap[QVR_Analog_Right_Axis_Y] = 2;
                _analogsMap[QVR_Analog_Right_Axis_X] = 3;
                _analogsMap[QVR_Analog_Left_Trigger] = 4;
                _analogsMap[QVR_Analog_Right_Trigger] = 5;
                _analogsMap[QVR_Analog_Left_Grip] = 6;
                _analogsMap[QVR_Analog_Right_Grip] = 7;
                if (QVRManager::processIndex() == config().processIndex()) {
                    Q_ASSERT(QVROculus);
                    _internals->oculusAnalogsEntity = 0;
                }
            } else if (arg == "controller-left") {
                _analogs.resize(4);
                _analogsMap[QVR_Analog_Axis_Y] = 0;
                _analogsMap[QVR_Analog_Axis_X] = 1;
                _analogsMap[QVR_Analog_Trigger] = 2;
                _analogsMap[QVR_Analog_Grip] = 3;
                if (QVRManager::processIndex() == config().processIndex()) {
                    Q_ASSERT(QVROculus);
                    _internals->oculusAnalogsEntity = 1;
                }
            } else if (arg == "controller-right") {
                _analogs.resize(4);
                _analogsMap[QVR_Analog_Axis_Y] = 0;
                _analogsMap[QVR_Analog_Axis_X] = 1;
                _analogsMap[QVR_Analog_Trigger] = 2;
                _analogsMap[QVR_Analog_Grip] = 3;
                if (QVRManager::processIndex() == config().processIndex()) {
                    Q_ASSERT(QVROculus);
                    _internals->oculusAnalogsEntity = 2;
                }
            } else {
                QVR_WARNING("device %s: invalid Oculus analogs parameter", qPrintable(id()));
            }
        }
#endif
        break;
    case QVR_Device_Analogs_OpenVR:
#ifdef HAVE_OPENVR
        {
            QString arg = config().analogsParameters().trimmed();
            if (arg == "controller-0") {
                _analogs.resize(3);
                _analogsMap[QVR_Analog_Axis_Y] = 0;
                _analogsMap[QVR_Analog_Axis_X] = 1;
                _analogsMap[QVR_Analog_Trigger] = 2;
                if (QVRManager::processIndex() == config().processIndex()) {
                    Q_ASSERT(QVROpenVRSystem);
                    _internals->openVrAnalogsEntity = 0;
                }
            } else if (arg == "controller-1") {
                _analogs.resize(3);
                _analogsMap[QVR_Analog_Axis_Y] = 0;
                _analogsMap[QVR_Analog_Axis_X] = 1;
                _analogsMap[QVR_Analog_Trigger] = 2;
                if (QVRManager::processIndex() == config().processIndex()) {
                    Q_ASSERT(QVROpenVRSystem);
                    _internals->openVrAnalogsEntity = 1;
                }
            } else {
                QVR_WARNING("device %s: invalid OpenVR analogs parameter", qPrintable(id()));
            }
        }
#endif
        break;
    case QVR_Device_Analogs_GoogleVR:
#ifdef ANDROID
        {
            QString arg = config().buttonsParameters().trimmed();
            if (arg == "daydream") {
                _analogs.resize(2);
                _analogsMap[QVR_Analog_Axis_Y] = 0;
                _analogsMap[QVR_Analog_Axis_X] = 1;
            } else {
                QVR_WARNING("device %s: invalid GoogleVR analogs parameter", qPrintable(id()));
            }
        }
#endif
        break;
    }
}

QVRDevice::QVRDevice(const QVRDevice& d) : _internals(NULL)
{
    _index = d._index;
    _position = d._position;
    _orientation = d._orientation;
    std::memcpy(_buttonsMap, d._buttonsMap, sizeof(_buttonsMap));
    _buttons = d._buttons;
    std::memcpy(_analogsMap, d._analogsMap, sizeof(_analogsMap));
    _analogs = d._analogs;
}

QVRDevice::~QVRDevice()
{
    if (_internals) {
#ifdef HAVE_QGAMEPAD
        delete _internals->buttonsGamepad;
        if (_internals->analogsGamepad != _internals->buttonsGamepad)
            delete _internals->analogsGamepad;
#endif
#ifdef HAVE_VRPN
        delete _internals->vrpnTrackerRemote;
        delete _internals->vrpnAnalogRemote;
        delete _internals->vrpnButtonRemote;
#endif
#ifdef ANDROID
        // nothing to do here; we do not own Google VR objects
#endif
        delete _internals;
    }
}

const QVRDevice& QVRDevice::operator=(const QVRDevice& d)
{
    _index = d._index;
    _position = d._position;
    _orientation = d._orientation;
    std::memcpy(_buttonsMap, d._buttonsMap, sizeof(_buttonsMap));
    _buttons = d._buttons;
    std::memcpy(_analogsMap, d._analogsMap, sizeof(_analogsMap));
    _analogs = d._analogs;
    return *this;
}

int QVRDevice::index() const
{
    return _index;
}

const QString& QVRDevice::id() const
{
    return config().id();
}

const QVRDeviceConfig& QVRDevice::config() const
{
    return QVRManager::config().deviceConfigs().at(index());
}

int QVRDevice::modelNodeCount() const
{
    int ret = 0;
#ifdef HAVE_OPENVR
    if (_internals) {
        if (_internals->openVrTrackedEntity == 3)
            ret = QVROpenVRControllerModelPositions[0].size();
        else if (_internals->openVrTrackedEntity == 4)
            ret = QVROpenVRControllerModelPositions[1].size();
    }
#endif
    return ret;
}

QVector3D QVRDevice::modelNodePosition(int nodeIndex) const
{
    Q_UNUSED(nodeIndex);
    QVector3D ret;
#ifdef HAVE_OPENVR
    if (_internals) {
        if (_internals->openVrTrackedEntity == 3)
            ret = QVROpenVRControllerModelPositions[0].at(nodeIndex);
        else if (_internals->openVrTrackedEntity == 4)
            ret = QVROpenVRControllerModelPositions[1].at(nodeIndex);
    }
#endif
    return ret;
}

QQuaternion QVRDevice::modelNodeOrientation(int nodeIndex) const
{
    Q_UNUSED(nodeIndex);
    QQuaternion ret;
#ifdef HAVE_OPENVR
    if (_internals) {
        if (_internals->openVrTrackedEntity == 3)
            ret = QVROpenVRControllerModelOrientations[0].at(nodeIndex);
        else if (_internals->openVrTrackedEntity == 4)
            ret = QVROpenVRControllerModelOrientations[1].at(nodeIndex);
    }
#endif
    return ret;
}

int QVRDevice::modelNodeVertexDataIndex(int nodeIndex) const
{
    Q_UNUSED(nodeIndex);
    int ret = -1;
#ifdef HAVE_OPENVR
    if (_internals) {
        if (_internals->openVrTrackedEntity == 3)
            ret = QVROpenVRControllerModelVertexDataIndices[0].at(nodeIndex);
        else if (_internals->openVrTrackedEntity == 4)
            ret = QVROpenVRControllerModelVertexDataIndices[1].at(nodeIndex);
    }
#endif
    return ret;
}

int QVRDevice::modelNodeTextureIndex(int nodeIndex) const
{
    Q_UNUSED(nodeIndex);
    int ret = -1;
#ifdef HAVE_OPENVR
    if (_internals) {
        if (_internals->openVrTrackedEntity == 3)
            ret = QVROpenVRControllerModelTextureIndices[0].at(nodeIndex);
        else if (_internals->openVrTrackedEntity == 4)
            ret = QVROpenVRControllerModelTextureIndices[1].at(nodeIndex);
    }
#endif
    return ret;
}

bool QVRDevice::supportsHapticPulse() const
{
    int ret = false;
#ifdef HAVE_OCULUS
# if (OVR_PRODUCT_VERSION >= 1)
    if (_internals && (_internals->oculusTrackedEntity == 3 || _internals->oculusTrackedEntity == 4))
        ret = true;
# endif
#endif
#ifdef HAVE_OPENVR
    if (_internals && (_internals->openVrTrackedEntity == 3 || _internals->openVrTrackedEntity == 4))
        ret = true;
#endif
    return ret;
}

void QVRDevice::triggerHapticPulse(int microseconds) const
{
    Q_UNUSED(microseconds);
#ifdef HAVE_OCULUS
# if (OVR_PRODUCT_VERSION >= 1)
    if (_internals && (_internals->oculusTrackedEntity == 3 || _internals->oculusTrackedEntity == 4)) {
        ovrControllerType ct = (_internals->oculusTrackedEntity == 3 ?
                ovrControllerType_LTouch : ovrControllerType_RTouch);
        ovr_SetControllerVibration(QVROculus, ct, 0.5f, 1.0f);
        QTimer::singleShot(std::round(microseconds / 1e3f), [ct]() { ovr_SetControllerVibration(QVROculus, ct, 0.5f, 0.0f); } );
    }
# endif
#endif
#ifdef HAVE_OPENVR
    if (_internals && (_internals->openVrTrackedEntity == 3 || _internals->openVrTrackedEntity == 4)) {
        int deviceIndex = QVROpenVRControllerIndices[_internals->openVrTrackedEntity - 3];
        QVROpenVRSystem->TriggerHapticPulse(deviceIndex, 0, microseconds);
    }
#endif
}

#ifdef HAVE_OCULUS
static QVector3D QVROculusConvert(const ovrVector3f& v)
{
    return QVector3D(v.x, v.y, v.z);
}
static QQuaternion QVROculusConvert(const ovrQuatf& q)
{
    return QQuaternion(q.w, q.x, q.y, q.z);
}
#endif

void QVRDevice::update()
{
    if (config().processIndex() == QVRManager::processIndex()) {
        bool wantVelocityCalculation = (config().trackingType() != QVR_Device_Tracking_None
                && config().trackingType() != QVR_Device_Tracking_Static);
        if (wantVelocityCalculation) {
            _internals->lastTimestamp = _internals->currentTimestamp;
            _internals->lastPosition = _position;
            _internals->lastOrientation = _orientation;
            _internals->currentTimestamp = QVRTimer.nsecsElapsed();
        }
#ifdef HAVE_QGAMEPAD
        if (_internals->buttonsGamepad) {
            _buttons[_buttonsMap[QVR_Button_L1]] = _internals->buttonsGamepad->buttonL1();
            _buttons[_buttonsMap[QVR_Button_L2]] = (_internals->buttonsGamepad->buttonL2() >= 1);
            _buttons[_buttonsMap[QVR_Button_L3]] = _internals->buttonsGamepad->buttonL3();
            _buttons[_buttonsMap[QVR_Button_R1]] = _internals->buttonsGamepad->buttonR1();
            _buttons[_buttonsMap[QVR_Button_R2]] = (_internals->buttonsGamepad->buttonR2() >= 1);
            _buttons[_buttonsMap[QVR_Button_R3]] = _internals->buttonsGamepad->buttonR3();
            _buttons[_buttonsMap[QVR_Button_A]] = _internals->buttonsGamepad->buttonA();
            _buttons[_buttonsMap[QVR_Button_B]] = _internals->buttonsGamepad->buttonB();
            _buttons[_buttonsMap[QVR_Button_X]] = _internals->buttonsGamepad->buttonX();
            _buttons[_buttonsMap[QVR_Button_Y]] = _internals->buttonsGamepad->buttonY();
            _buttons[_buttonsMap[QVR_Button_Up]] = _internals->buttonsGamepad->buttonUp();
            _buttons[_buttonsMap[QVR_Button_Down]] = _internals->buttonsGamepad->buttonDown();
            _buttons[_buttonsMap[QVR_Button_Left]] = _internals->buttonsGamepad->buttonLeft();
            _buttons[_buttonsMap[QVR_Button_Right]] = _internals->buttonsGamepad->buttonRight();
            _buttons[_buttonsMap[QVR_Button_Center]] = _internals->buttonsGamepad->buttonCenter();
            _buttons[_buttonsMap[QVR_Button_Select]] = _internals->buttonsGamepad->buttonSelect();
            _buttons[_buttonsMap[QVR_Button_Start]] = _internals->buttonsGamepad->buttonStart();
            _buttons[_buttonsMap[QVR_Button_Menu]] = _internals->buttonsGamepad->buttonGuide();
        }
        if (_internals->analogsGamepad) {
            _analogs[_analogsMap[QVR_Analog_Right_Axis_Y]] = -_internals->analogsGamepad->axisRightY();
            _analogs[_analogsMap[QVR_Analog_Right_Axis_X]] = _internals->analogsGamepad->axisRightX();
            _analogs[_analogsMap[QVR_Analog_Left_Axis_Y]] = -_internals->analogsGamepad->axisLeftY();
            _analogs[_analogsMap[QVR_Analog_Left_Axis_X]] = _internals->analogsGamepad->axisLeftX();
            _analogs[_analogsMap[QVR_Analog_Right_Trigger]] = _internals->analogsGamepad->buttonR2();
            _analogs[_analogsMap[QVR_Analog_Left_Trigger]] = _internals->analogsGamepad->buttonL2();
        }
#endif
#ifdef HAVE_VRPN
        if (_internals->vrpnTrackerRemote) {
            _internals->vrpnHaveVelocity = false;
            _internals->vrpnTrackerRemote->mainloop();
            if (_internals->vrpnHaveVelocity)
                wantVelocityCalculation = false;
        }
        if (_internals->vrpnButtonRemote)
            _internals->vrpnButtonRemote->mainloop();
        if (_internals->vrpnAnalogRemote)
            _internals->vrpnAnalogRemote->mainloop();
#endif
#ifdef HAVE_OCULUS
        if (_internals->oculusTrackedEntity >= 0) {
            if (_internals->oculusTrackedEntity == 0) {
                _position = QVROculusConvert(QVROculusTrackingState.HeadPose.ThePose.Position);
                _orientation = QVROculusConvert(QVROculusTrackingState.HeadPose.ThePose.Orientation);
                _velocity = QVROculusConvert(QVROculusTrackingState.HeadPose.LinearVelocity);
                _angularVelocity = QVROculusConvert(QVROculusTrackingState.HeadPose.AngularVelocity);
                wantVelocityCalculation = false;
            } else if (_internals->oculusTrackedEntity == 1) {
                _position = QVROculusConvert(QVROculusRenderPoses[0].Position);
                _orientation = QVROculusConvert(QVROculusRenderPoses[0].Orientation);
            } else if (_internals->oculusTrackedEntity == 2) {
                _position = QVROculusConvert(QVROculusRenderPoses[1].Position);
                _orientation = QVROculusConvert(QVROculusRenderPoses[1].Orientation);
# if (OVR_PRODUCT_VERSION >= 1)
            } else if (_internals->oculusTrackedEntity == 3) {
                _position = QVROculusConvert(QVROculusTrackingState.HandPoses[ovrHand_Left].ThePose.Position);
                _orientation = QVROculusConvert(QVROculusTrackingState.HandPoses[ovrHand_Left].ThePose.Orientation);
                _velocity = QVROculusConvert(QVROculusTrackingState.HandPoses[ovrHand_Left].LinearVelocity);
                _angularVelocity = QVROculusConvert(QVROculusTrackingState.HandPoses[ovrHand_Left].AngularVelocity);
                wantVelocityCalculation = false;
            } else if (_internals->oculusTrackedEntity == 4) {
                _position = QVROculusConvert(QVROculusTrackingState.HandPoses[ovrHand_Right].ThePose.Position);
                _orientation = QVROculusConvert(QVROculusTrackingState.HandPoses[ovrHand_Right].ThePose.Orientation);
                _velocity = QVROculusConvert(QVROculusTrackingState.HandPoses[ovrHand_Right].LinearVelocity);
                _angularVelocity = QVROculusConvert(QVROculusTrackingState.HandPoses[ovrHand_Right].AngularVelocity);
                wantVelocityCalculation = false;
# endif
            }
            // This Y offset moves the sitting user's eyes to a default standing height in the virtual world:
            _position.setY(_position.y() + QVRObserverConfig::defaultEyeHeight);
        }
# if (OVR_PRODUCT_VERSION >= 1)
        if (_internals->oculusButtonsEntity >= 0) {
            if (_internals->oculusButtonsEntity == 1) {
                _buttons[0] = QVROculusInputState.Thumbstick[ovrHand_Left].y > +0.5f;
                _buttons[1] = QVROculusInputState.Thumbstick[ovrHand_Left].y < -0.5f;
                _buttons[2] = QVROculusInputState.Thumbstick[ovrHand_Left].x > +0.5f;
                _buttons[3] = QVROculusInputState.Thumbstick[ovrHand_Left].x < -0.5f;
                _buttons[4] = QVROculusInputState.Buttons & ovrButton_X;
                _buttons[5] = QVROculusInputState.Buttons & ovrButton_Y;
                _buttons[6] = QVROculusInputState.Buttons & ovrButton_LShoulder;
                _buttons[7] = QVROculusInputState.Buttons & ovrButton_Enter;
            } else if (_internals->oculusButtonsEntity == 2) {
                _buttons[0] = QVROculusInputState.Thumbstick[ovrHand_Right].y > +0.5f;
                _buttons[1] = QVROculusInputState.Thumbstick[ovrHand_Right].y < -0.5f;
                _buttons[2] = QVROculusInputState.Thumbstick[ovrHand_Right].x > +0.5f;
                _buttons[3] = QVROculusInputState.Thumbstick[ovrHand_Right].x < -0.5f;
                _buttons[4] = QVROculusInputState.Buttons & ovrButton_A;
                _buttons[5] = QVROculusInputState.Buttons & ovrButton_B;
                _buttons[6] = QVROculusInputState.Buttons & ovrButton_RShoulder;
                _buttons[7] = QVROculusInputState.Buttons & ovrButton_Enter;
            } else {
                _buttons[0] = QVROculusInputState.Buttons & ovrButton_Up;
                _buttons[1] = QVROculusInputState.Buttons & ovrButton_Down;
                _buttons[2] = QVROculusInputState.Buttons & ovrButton_Left;
                _buttons[3] = QVROculusInputState.Buttons & ovrButton_Right;
                _buttons[4] = QVROculusInputState.Buttons & ovrButton_A;
                _buttons[5] = QVROculusInputState.Buttons & ovrButton_B;
                _buttons[6] = QVROculusInputState.Buttons & ovrButton_X;
                _buttons[7] = QVROculusInputState.Buttons & ovrButton_Y;
                _buttons[8] = QVROculusInputState.Buttons & ovrButton_LShoulder;
                _buttons[9] = QVROculusInputState.Buttons & ovrButton_RShoulder;
                _buttons[10] = QVROculusInputState.Buttons & ovrButton_Enter;
                _buttons[11] = QVROculusInputState.Buttons & ovrButton_Back;
            }
        }
        if (_internals->oculusAnalogsEntity >= 0) {
            if (_internals->oculusAnalogsEntity == 1) {
                _analogs[0] = QVROculusInputState.Thumbstick[ovrHand_Left].y;
                _analogs[1] = QVROculusInputState.Thumbstick[ovrHand_Left].x;
                _analogs[2] = QVROculusInputState.IndexTrigger[ovrHand_Left];
                _analogs[3] = QVROculusInputState.HandTrigger[ovrHand_Left];
            } else if (_internals->oculusAnalogsEntity == 2) {
                _analogs[0] = QVROculusInputState.Thumbstick[ovrHand_Right].y;
                _analogs[1] = QVROculusInputState.Thumbstick[ovrHand_Right].x;
                _analogs[2] = QVROculusInputState.IndexTrigger[ovrHand_Right];
                _analogs[3] = QVROculusInputState.HandTrigger[ovrHand_Right];
            } else {
                _analogs[0] = QVROculusInputState.Thumbstick[ovrHand_Left].y;
                _analogs[1] = QVROculusInputState.Thumbstick[ovrHand_Left].x;
                _analogs[2] = QVROculusInputState.Thumbstick[ovrHand_Right].y;
                _analogs[3] = QVROculusInputState.Thumbstick[ovrHand_Right].x;
                _analogs[4] = QVROculusInputState.IndexTrigger[ovrHand_Left];
                _analogs[5] = QVROculusInputState.IndexTrigger[ovrHand_Right];
                _analogs[6] = QVROculusInputState.HandTrigger[ovrHand_Right];
                _analogs[7] = QVROculusInputState.HandTrigger[ovrHand_Left];
            }
        }
# endif
#endif
#ifdef HAVE_OPENVR
        if (_internals->openVrTrackedEntity >= 0) {
            _orientation = QVROpenVRTrackedOrientations[_internals->openVrTrackedEntity];
            _position = QVROpenVRTrackedPositions[_internals->openVrTrackedEntity];
            if (QVROpenVRHaveTrackedVelocities[_internals->openVrTrackedEntity]) {
                _velocity = QVROpenVRTrackedVelocities[_internals->openVrTrackedEntity];
                _angularVelocity = QVROpenVRTrackedAngularVelocities[_internals->openVrTrackedEntity];
                wantVelocityCalculation = false;
            }
        }
        if (_internals->openVrButtonsEntity >= 0) {
            _buttons[0] = QVROpenVRControllerStates[_internals->openVrButtonsEntity].rAxis[0].y > +0.5f;
            _buttons[1] = QVROpenVRControllerStates[_internals->openVrButtonsEntity].rAxis[0].y < -0.5f;
            _buttons[2] = QVROpenVRControllerStates[_internals->openVrButtonsEntity].rAxis[0].x < -0.5f;
            _buttons[3] = QVROpenVRControllerStates[_internals->openVrButtonsEntity].rAxis[0].x > +0.5f;
            unsigned long buttonPressed = QVROpenVRControllerStates[_internals->openVrButtonsEntity].ulButtonPressed;
            _buttons[4] = buttonPressed & vr::ButtonMaskFromId(vr::k_EButton_ApplicationMenu);
            _buttons[5] = buttonPressed & vr::ButtonMaskFromId(vr::k_EButton_Grip);
        }
        if (_internals->openVrAnalogsEntity >= 0) {
            _analogs[0] = QVROpenVRControllerStates[_internals->openVrAnalogsEntity].rAxis[0].y;
            _analogs[1] = QVROpenVRControllerStates[_internals->openVrAnalogsEntity].rAxis[0].x;
            _analogs[2] = QVROpenVRControllerStates[_internals->openVrAnalogsEntity].rAxis[1].x;
        }
#endif
#ifdef ANDROID
        if (_internals->googleVrTrackedEntity >= 0) {
            _orientation = QVRGoogleVROrientations[_internals->googleVrTrackedEntity];
            _position = QVRGoogleVRPositions[_internals->googleVrTrackedEntity];
        }
        if (config().buttonsType() == QVR_Device_Buttons_GoogleVR) {
            if (_buttons.size() == 1) {
                // Consume a touch event generated on the Android thread.
                // We set the button status to "pressed" until the next call of this function (typically 1 frame).
                _buttons[0] = QVRGoogleVRTouchEvent.testAndSetRelaxed(1, 0);
            } else {
                _buttons[0] = QVRGoogleVRButtons[0];
                _buttons[1] = QVRGoogleVRButtons[1];
                _buttons[2] = QVRGoogleVRButtons[2];
            }
        }
        if (config().analogsType() == QVR_Device_Analogs_GoogleVR) {
            _analogs[0] = QVRGoogleVRAxes[0];
            _analogs[1] = QVRGoogleVRAxes[1];
        }
#endif
        if (wantVelocityCalculation && _internals->lastTimestamp >= 0) {
            qint64 usecs = (_internals->currentTimestamp - _internals->lastTimestamp) / 1000;
            double secs = usecs / 1e6;
            _velocity = (_position - _internals->lastPosition) / secs;
            _angularVelocity = QVRAngularVelocityFromDiffQuaternion(
                    _orientation * _internals->lastOrientation.conjugated(), secs);
        }
    }
}

QDataStream &operator<<(QDataStream& ds, const QVRDevice& d)
{
    ds << d._index << d._position << d._orientation << d._velocity << d._angularVelocity << d._buttons << d._analogs;
    ds.writeRawData(reinterpret_cast<const char*>(d._buttonsMap), sizeof(d._buttonsMap));
    ds.writeRawData(reinterpret_cast<const char*>(d._analogsMap), sizeof(d._analogsMap));
    return ds;
}

QDataStream &operator>>(QDataStream& ds, QVRDevice& d)
{
    ds >> d._index >> d._position >> d._orientation >> d._velocity >> d._angularVelocity >> d._buttons >> d._analogs;
    ds.readRawData(reinterpret_cast<char*>(d._buttonsMap), sizeof(d._buttonsMap));
    ds.readRawData(reinterpret_cast<char*>(d._analogsMap), sizeof(d._analogsMap));
    return ds;
}
