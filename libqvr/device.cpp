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

#include "manager.hpp"
#include "device.hpp"
#include "logging.hpp"

#ifdef HAVE_OSVR
# include <osvr/ClientKit/InterfaceStateC.h>
# include <osvr/ClientKit/InterfaceC.h>
#endif


#ifdef HAVE_VRPN
void QVRVrpnTrackerChangeHandler(void* userdata, const vrpn_TRACKERCB info)
{
    QVRDevice* d = reinterpret_cast<QVRDevice*>(userdata);
    d->_position = QVector3D(info.pos[0], info.pos[1], info.pos[2]);
    d->_orientation = QQuaternion(info.quat[3], info.quat[0], info.quat[1], info.quat[2]);
}
void QVRVrpnButtonChangeHandler(void* userdata, const vrpn_BUTTONCB info)
{
    QVRDevice* d = reinterpret_cast<QVRDevice*>(userdata);
    for (int i = 0; i < d->_buttons.length(); i++) {
        if (info.button == d->_vrpnButtons[i])
            d->_buttons[i] = info.state;
    }
}
void QVRVrpnAnalogChangeHandler(void* userdata, const vrpn_ANALOGCB info)
{
    QVRDevice* d = reinterpret_cast<QVRDevice*>(userdata);
    for (int i = 0; i < d->_analogs.length(); i++) {
        int j = d->_vrpnAnalogs[i];
        if (j >= 0 && j < info.num_channel)
            d->_analogs[i] = info.channel[j];
    }
}
#endif


QVRDevice::QVRDevice() :
    _index(-1)
{
#ifdef HAVE_OCULUS
    _oculusTrackedEye = -1;
#endif
#ifdef HAVE_VRPN
    _vrpnTrackerRemote = NULL;
    _vrpnAnalogRemote = NULL;
    _vrpnButtonRemote = NULL;
#endif
#ifdef HAVE_OSVR
    _osvrTrackedEye = -1;
    _osvrTrackingInterface = NULL;
#endif
}

QVRDevice::QVRDevice(int deviceIndex) :
    _index(deviceIndex)
{
#ifdef HAVE_OCULUS
    _oculusTrackedEye = -1;
#endif
#ifdef HAVE_VRPN
    _vrpnTrackerRemote = NULL;
    _vrpnAnalogRemote = NULL;
    _vrpnButtonRemote = NULL;
#endif
#ifdef HAVE_OSVR
    _osvrTrackedEye = -1;
    _osvrTrackingInterface = NULL;
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
    case QVR_Device_Tracking_Oculus:
#ifdef HAVE_OCULUS
        if (QVRManager::processIndex() == config().processIndex()) {
            QString arg = config().trackingParameters().trimmed();
            if (arg == "eye-left")
                _oculusTrackedEye = 1;
            else if (arg == "eye-right")
                _oculusTrackedEye = 2;
            else if (arg == "head")
                _oculusTrackedEye = 0;
        }
#endif
        break;
    case QVR_Device_Tracking_VRPN:
#ifdef HAVE_VRPN
        if (QVRManager::processIndex() == config().processIndex()) {
            QStringList args = config().trackingParameters().split(' ', QString::SkipEmptyParts);
            QString name = (args.length() >= 1 ? args[0] : config().trackingParameters());
            int sensor = (args.length() >= 2 ? args[1].toInt() : vrpn_ALL_SENSORS);
            _vrpnTrackerRemote = new vrpn_Tracker_Remote(qPrintable(name));
            vrpn_System_TextPrinter.set_ostream_to_use(stderr);
            _vrpnTrackerRemote->register_change_handler(this, QVRVrpnTrackerChangeHandler, sensor);
        }
#endif
        break;
    case QVR_Device_Tracking_OSVR:
#ifdef HAVE_OSVR
        if (QVRManager::processIndex() == config().processIndex()) {
            Q_ASSERT(QVROsvrClientContext);
            const QString& osvrPath = config().trackingParameters();
            if (osvrPath == "eye-center") {
                _osvrTrackedEye = 0;
            } else if (osvrPath == "eye-left") {
                _osvrTrackedEye = 1;
            } else if (osvrPath == "eye-right") {
                _osvrTrackedEye = 2;
            } else {
                osvrClientGetInterface(QVROsvrClientContext, qPrintable(osvrPath), &_osvrTrackingInterface);
                if (!_osvrTrackingInterface) {
                    QVR_WARNING("device %s: OSVR interface path %s does not exist",
                            qPrintable(id()), qPrintable(osvrPath));
                }
            }
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
            _buttons.resize(args.length());
            for (int i = 0; i < _buttons.length(); i++)
                _buttons[i] = args[i].toInt();
        }
        break;
    case QVR_Device_Buttons_VRPN:
#ifdef HAVE_VRPN
        {
            QStringList args = config().buttonsParameters().split(' ', QString::SkipEmptyParts);
            QString name = (args.length() >= 1 ? args[0] : config().buttonsParameters());
            if (args.length() > 1) {
                _buttons.resize(args.length() - 1);
                _vrpnButtons.resize(_buttons.length());
                for (int i = 0; i < _buttons.length(); i++)
                    _vrpnButtons[i] = args[i + 1].toInt();
            } else {
                _buttons.resize(32);
                _vrpnButtons.resize(_buttons.length());
                for (int i = 0; i < _buttons.length(); i++)
                    _vrpnButtons[i] = i;
            }
            for (int i = 0; i < _buttons.length(); i++)
                _buttons[i] = false;
            if (QVRManager::processIndex() == config().processIndex()) {
                _vrpnButtonRemote = new vrpn_Button_Remote(qPrintable(name));
                vrpn_System_TextPrinter.set_ostream_to_use(stderr);
                _vrpnButtonRemote->register_change_handler(this, QVRVrpnButtonChangeHandler);
            }
        }
#endif
    case QVR_Device_Buttons_OSVR:
#ifdef HAVE_OSVR
        {
            QStringList args = config().buttonsParameters().split(' ', QString::SkipEmptyParts);
            _buttons.resize(args.length());
            if (QVRManager::processIndex() == config().processIndex()) {
                Q_ASSERT(QVROsvrClientContext);
                for (int i = 0; i < _buttons.length(); i++) {
                    const QString& osvrPath = args[i];
                    OSVR_ClientInterface osvrInterface;
                    osvrClientGetInterface(QVROsvrClientContext, qPrintable(osvrPath), &osvrInterface);
                    if (!osvrInterface) {
                        QVR_WARNING("device %s: OSVR interface path %s does not exist",
                                qPrintable(id()), qPrintable(osvrPath));
                    }
                    _osvrButtonsInterfaces.append(osvrInterface);
                }
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
            for (int i = 0; i < _analogs.length(); i++)
                _analogs[i] = args[i].toFloat();
        }
        break;
    case QVR_Device_Analogs_VRPN:
#ifdef HAVE_VRPN
        {
            QStringList args = config().analogsParameters().split(' ', QString::SkipEmptyParts);
            QString name = (args.length() >= 1 ? args[0] : config().analogsParameters());
            if (args.length() > 1) {
                _analogs.resize(args.length() - 1);
                _vrpnAnalogs.resize(_analogs.length());
                for (int i = 0; i < _analogs.length(); i++)
                    _vrpnAnalogs[i] = args[i + 1].toInt();
            } else {
                _analogs.resize(8);
                _vrpnAnalogs.resize(_analogs.length());
                for (int i = 0; i < _analogs.length(); i++)
                    _vrpnAnalogs[i] = i;
            }
            for (int i = 0; i < _analogs.length(); i++)
                _analogs[i] = 0.0f;
            if (QVRManager::processIndex() == config().processIndex()) {
                _vrpnAnalogRemote = new vrpn_Analog_Remote(qPrintable(name));
                vrpn_System_TextPrinter.set_ostream_to_use(stderr);
                _vrpnAnalogRemote->register_change_handler(this, QVRVrpnAnalogChangeHandler);
            }
        }
#endif
    case QVR_Device_Analogs_OSVR:
#ifdef HAVE_OSVR
        {
            QStringList args = config().analogsParameters().split(' ', QString::SkipEmptyParts);
            _analogs.resize(args.length());
            if (QVRManager::processIndex() == config().processIndex()) {
                Q_ASSERT(QVROsvrClientContext);
                for (int i = 0; i < _analogs.length(); i++) {
                    const QString& osvrPath = args[i];
                    OSVR_ClientInterface osvrInterface;
                    osvrClientGetInterface(QVROsvrClientContext, qPrintable(osvrPath), &osvrInterface);
                    if (!osvrInterface) {
                        QVR_WARNING("device %s: OSVR interface path %s does not exist",
                                qPrintable(id()), qPrintable(osvrPath));
                    }
                    _osvrAnalogsInterfaces.append(osvrInterface);
                }
            }
        }
#endif
        break;
    }
}

QVRDevice::~QVRDevice()
{
#ifdef HAVE_VRPN
    delete _vrpnTrackerRemote;
    delete _vrpnAnalogRemote;
    delete _vrpnButtonRemote;
#endif
#ifdef HAVE_OSVR
    // the OSVR interfaces do not need to be cleaned up,
    // this will happen when the OSVR context exits.
#endif
}

const QVRDevice& QVRDevice::operator=(const QVRDevice& d)
{
    _index = d._index;
    _position = d._position;
    _orientation = d._orientation;
    _buttons = d._buttons;
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

void QVRDevice::update()
{
    if (config().processIndex() == QVRManager::processIndex()) {
#ifdef HAVE_OCULUS
        if (_oculusTrackedEye >= 0) {
            const ovrPosef* p;
            if (_oculusTrackedEye == 1)
                p = &(QVROculusRenderPoses[0]);
            else if (_oculusTrackedEye == 2)
                p = &(QVROculusRenderPoses[1]);
            else
                p = &(QVROculusTrackingState.HeadPose.ThePose);
            _position = QVector3D(p->Position.x, p->Position.y, p->Position.z);
            _orientation = QQuaternion(p->Orientation.w, p->Orientation.x, p->Orientation.y, p->Orientation.z);
        }
#endif
#ifdef HAVE_VRPN
        if (_vrpnTrackerRemote)
            _vrpnTrackerRemote->mainloop();
        if (_vrpnButtonRemote)
            _vrpnButtonRemote->mainloop();
        if (_vrpnAnalogRemote)
            _vrpnAnalogRemote->mainloop();
#endif
#ifdef HAVE_OSVR
        if (_osvrTrackedEye != -1 || _osvrTrackingInterface) {
            OSVR_Pose3 pose;
            bool ok;
            if (_osvrTrackedEye == 0) { // center eye
                ok = (osvrClientGetViewerPose(QVROsvrDisplayConfig, 0, &pose) == OSVR_RETURN_SUCCESS);
            } else if (_osvrTrackedEye == 1) { // left eye
                ok = (osvrClientGetViewerEyePose(QVROsvrDisplayConfig, 0, 0, &pose) == OSVR_RETURN_SUCCESS);
            } else if (_osvrTrackedEye == 2) { // right eye
                OSVR_EyeCount eyes;
                osvrClientGetNumEyesForViewer(QVROsvrDisplayConfig, 0, &eyes);
                int e = (eyes == 2 ? 1 : 0);
                ok = (osvrClientGetViewerEyePose(QVROsvrDisplayConfig, 0, e, &pose) == OSVR_RETURN_SUCCESS);
            } else { // _osvrTrackingInterface
                struct OSVR_TimeValue timestamp;
                ok = (osvrGetPoseState(_osvrTrackingInterface, &timestamp, &pose) == OSVR_RETURN_SUCCESS);
            }
            if (ok) {
                _position = QVector3D(pose.translation.data[0], pose.translation.data[1],
                        pose.translation.data[2]);
                _orientation = QQuaternion(pose.rotation.data[0], pose.rotation.data[1],
                        pose.rotation.data[2], pose.rotation.data[3]);
            }
        }
        if (_osvrButtonsInterfaces.length() > 0) {
            OSVR_ButtonState state;
            struct OSVR_TimeValue timestamp;
            for (int i = 0; i < _buttons.length(); i++) {
                if (_osvrButtonsInterfaces[i]
                        && osvrGetButtonState(_osvrButtonsInterfaces[i],
                            &timestamp, &state) == OSVR_RETURN_SUCCESS) {
                    _buttons[i] = state;
                }
            }
        }
        if (_osvrAnalogsInterfaces.length() > 0) {
            OSVR_AnalogState state;
            struct OSVR_TimeValue timestamp;
            for (int i = 0; i < _analogs.length(); i++) {
                if (_osvrAnalogsInterfaces[i]
                        && osvrGetAnalogState(_osvrAnalogsInterfaces[i],
                            &timestamp, &state) == OSVR_RETURN_SUCCESS) {
                    _analogs[i] = state;
                }
            }
        }
#endif
    }
}

QDataStream &operator<<(QDataStream& ds, const QVRDevice& d)
{
    ds << d._index << d._position << d._orientation << d._buttons << d._analogs;
    return ds;
}

QDataStream &operator>>(QDataStream& ds, QVRDevice& d)
{
    ds >> d._index >> d._position >> d._orientation >> d._buttons >> d._analogs;
    return ds;
}
