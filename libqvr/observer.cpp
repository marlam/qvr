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

#include <QtMath>

#include "manager.hpp"
#include "observer.hpp"


#ifdef HAVE_VRPN
void QVRVrpnNavigationTrackerChangeHandler(void* userdata, const vrpn_TRACKERCB info)
{
    QVRObserver* o = reinterpret_cast<QVRObserver*>(userdata);
    o->_vrpnNavigationOrientation = QQuaternion(info.quat[3], info.quat[0], info.quat[1], info.quat[2]);
}
void QVRVrpnNavigationAnalogChangeHandler(void* userdata, const vrpn_ANALOGCB info)
{
    QVRObserver* o = reinterpret_cast<QVRObserver*>(userdata);
    if (o->_vrpnNavigationAnalog[0] >= 0 && o->_vrpnNavigationAnalog[0] < info.num_channel)
        o->_vrpnNavigationAnalogState[0] = info.channel[o->_vrpnNavigationAnalog[0]];
    if (o->_vrpnNavigationAnalog[1] >= 0 && o->_vrpnNavigationAnalog[1] < info.num_channel)
        o->_vrpnNavigationAnalogState[1] = info.channel[o->_vrpnNavigationAnalog[1]];
}
void QVRVrpnNavigationButtonChangeHandler(void* userdata, const vrpn_BUTTONCB info)
{
    QVRObserver* o = reinterpret_cast<QVRObserver*>(userdata);
    for (int i = 0; i < 4; i++) {
        if (info.button == o->_vrpnNavigationButton[i]) {
            o->_vrpnNavigationButtonState[i] = info.state;
        }
    }
}
void QVRVrpnTrackingTrackerChangeHandler(void* userdata, const vrpn_TRACKERCB info)
{
    QVRObserver* o = reinterpret_cast<QVRObserver*>(userdata);
    o->setTracking(QVector3D(info.pos[0], info.pos[1], info.pos[2]),
            QQuaternion(info.quat[3], info.quat[0], info.quat[1], info.quat[2]));
}
#endif


QVRObserver::QVRObserver() :
    _index(-1)
{
#ifdef HAVE_VRPN
    _vrpnNavigationTrackerRemote = NULL;
    _vrpnNavigationAnalogRemote = NULL;
    _vrpnNavigationButtonRemote = NULL;
    _vrpnTrackingTrackerRemote = NULL;
#endif
}

QVRObserver::QVRObserver(int observerIndex) :
    _index(observerIndex)
{
    setNavigation(config().initialNavigationPosition(), config().initialNavigationOrientation());
    setEyeDistance(config().initialEyeDistance());
    setTracking(config().initialTrackingPosition(), config().initialTrackingOrientation());
#ifdef HAVE_VRPN
    if (config().navigationType() == QVR_Navigation_VRPN && QVRManager::processIndex() == 0) {
        QStringList args = config().navigationParameters().split(' ', QString::SkipEmptyParts);
        QString name = (args.length() == 8 ? args[0] : config().navigationParameters());
        int sensor = (args.length() == 8 ? args[1].toInt() : vrpn_ALL_SENSORS);
        _vrpnNavigationAnalog[0] = (args.length() == 8 ? args[2].toInt() : 0);
        _vrpnNavigationAnalog[1] = (args.length() == 8 ? args[3].toInt() : 1);
        _vrpnNavigationButton[0] = (args.length() == 8 ? args[4].toInt() : 0);
        _vrpnNavigationButton[1] = (args.length() == 8 ? args[5].toInt() : 1);
        _vrpnNavigationButton[2] = (args.length() == 8 ? args[6].toInt() : 2);
        _vrpnNavigationButton[3] = (args.length() == 8 ? args[7].toInt() : 3);
        _vrpnNavigationTrackerRemote = new vrpn_Tracker_Remote(qPrintable(name));
        _vrpnNavigationAnalogRemote = new vrpn_Analog_Remote(qPrintable(name));
        _vrpnNavigationButtonRemote = new vrpn_Button_Remote(qPrintable(name));
        _vrpnNavigationAnalogState[0] = 0.0f;
        _vrpnNavigationAnalogState[1] = 0.0f;
        _vrpnNavigationButtonState[0] = false;
        _vrpnNavigationButtonState[1] = false;
        _vrpnNavigationButtonState[2] = false;
        _vrpnNavigationButtonState[3] = false;
        vrpn_System_TextPrinter.set_ostream_to_use(stderr);
        _vrpnNavigationTrackerRemote->register_change_handler(this, QVRVrpnNavigationTrackerChangeHandler, sensor);
        _vrpnNavigationAnalogRemote->register_change_handler(this, QVRVrpnNavigationAnalogChangeHandler);
        _vrpnNavigationButtonRemote->register_change_handler(this, QVRVrpnNavigationButtonChangeHandler);
        _vrpnNavigationPos = QVector3D(0.0f, 0.0f, 0.0f);
        _vrpnNavigationRotY = 0.0f;
    } else {
        _vrpnNavigationTrackerRemote = NULL;
        _vrpnNavigationAnalogRemote = NULL;
        _vrpnNavigationButtonRemote = NULL;
    }
    if (config().trackingType() == QVR_Tracking_VRPN && QVRManager::processIndex() == 0) {
        QStringList args = config().trackingParameters().split(' ', QString::SkipEmptyParts);
        QString name = (args.length() == 2 ? args[0] : config().trackingParameters());
        int sensor = (args.length() == 2 ? args[1].toInt() : vrpn_ALL_SENSORS);
        _vrpnTrackingTrackerRemote = new vrpn_Tracker_Remote(qPrintable(name));
        vrpn_System_TextPrinter.set_ostream_to_use(stderr);
        _vrpnTrackingTrackerRemote->register_change_handler(this, QVRVrpnTrackingTrackerChangeHandler, sensor);
    } else {
        _vrpnTrackingTrackerRemote = NULL;
    }
#endif
}

QVRObserver::~QVRObserver()
{
#ifdef HAVE_VRPN
    delete _vrpnNavigationTrackerRemote;
    delete _vrpnNavigationAnalogRemote;
    delete _vrpnNavigationButtonRemote;
    delete _vrpnTrackingTrackerRemote;
#endif
}

int QVRObserver::index() const
{
    return _index;
}

const QString& QVRObserver::id() const
{
    return config().id();
}

const QVRObserverConfig& QVRObserver::config() const
{
    return QVRManager::config().observerConfigs().at(index());
}

void QVRObserver::setTracking(const QVector3D& pos, const QQuaternion& rot)
{
    _trackingPosition[QVR_Eye_Center] = pos;
    _trackingPosition[QVR_Eye_Left] = pos + rot * QVector3D(-0.5f * eyeDistance(), 0.0f, 0.0f);
    _trackingPosition[QVR_Eye_Right] = pos + rot * QVector3D(+0.5f * eyeDistance(), 0.0f, 0.0f);
    _trackingOrientation[QVR_Eye_Center] = rot;
    _trackingOrientation[QVR_Eye_Left] = rot;
    _trackingOrientation[QVR_Eye_Right] = rot;
}

void QVRObserver::setTracking(const QVector3D& posLeft, const QQuaternion& rotLeft, const QVector3D& posRight, const QQuaternion& rotRight)
{
    _eyeDistance = (posLeft - posRight).length();
    _trackingPosition[QVR_Eye_Center] = 0.5f * (posLeft + posRight);
    _trackingPosition[QVR_Eye_Left] = posLeft;
    _trackingPosition[QVR_Eye_Right] = posRight;
    _trackingOrientation[QVR_Eye_Center] = QQuaternion::slerp(rotLeft, rotRight, 0.5f);
    _trackingOrientation[QVR_Eye_Left] = rotLeft;
    _trackingOrientation[QVR_Eye_Right] = rotRight;
}

void QVRObserver::update()
{
#ifdef HAVE_VRPN
    if (_vrpnNavigationTrackerRemote) {
        _vrpnNavigationTrackerRemote->mainloop();
        _vrpnNavigationAnalogRemote->mainloop();
        _vrpnNavigationButtonRemote->mainloop();
        const float speed = 0.04f; // TODO: make this configurable?
        if (std::abs(_vrpnNavigationAnalogState[0]) > 0.0f
                || std::abs(_vrpnNavigationAnalogState[1]) > 0.0f) {
            QQuaternion wandRot = _vrpnNavigationOrientation * QQuaternion::fromEulerAngles(0.0f, _vrpnNavigationRotY, 0.0f);
            QVector3D forwardDir = wandRot * QVector3D(0.0f, 0.0f, -1.0f);
            forwardDir.setY(0.0f);
            forwardDir.normalize();
            QVector3D rightDir = wandRot * QVector3D(1.0f, 0.0f, 0.0f);
            rightDir.setY(0.0f);
            rightDir.normalize();
            _vrpnNavigationPos += speed * (forwardDir * _vrpnNavigationAnalogState[0] + rightDir * _vrpnNavigationAnalogState[1]);
        }
        if (_vrpnNavigationButtonState[0]) {
            _vrpnNavigationPos += speed * QVector3D(0.0f, +1.0f, 0.0f);
        }
        if (_vrpnNavigationButtonState[1]) {
            _vrpnNavigationPos += speed * QVector3D(0.0f, -1.0f, 0.0f);
        }
        if (_vrpnNavigationButtonState[2]) {
            _vrpnNavigationRotY += 1.0f;
            if (_vrpnNavigationRotY >= 360.0f)
                _vrpnNavigationRotY -= 360.0f;
        }
        if (_vrpnNavigationButtonState[3]) {
            _vrpnNavigationRotY -= 1.0f;
            if (_vrpnNavigationRotY <= 0.0f)
                _vrpnNavigationRotY += 360.0f;
        }
        setNavigation(_vrpnNavigationPos + config().initialNavigationPosition(),
                QQuaternion::fromEulerAngles(0.0f, _vrpnNavigationRotY, 0.0f) * config().initialNavigationOrientation());
    }
    if (_vrpnTrackingTrackerRemote) {
        _vrpnTrackingTrackerRemote->mainloop();
    }
#endif
}

QDataStream &operator<<(QDataStream& ds, const QVRObserver& o)
{
    ds << o._index << o._navigationPosition << o._navigationOrientation
        << o._trackingPosition[0] << o._trackingPosition[1] << o._trackingPosition[2]
        << o._trackingOrientation[0] << o._trackingOrientation[1] << o._trackingOrientation[2];
    return ds;
}

QDataStream &operator>>(QDataStream& ds, QVRObserver& o)
{
    ds >> o._index >> o._navigationPosition >> o._navigationOrientation
        >> o._trackingPosition[0] >> o._trackingPosition[1] >> o._trackingPosition[2]
        >> o._trackingOrientation[0] >> o._trackingOrientation[1] >> o._trackingOrientation[2];
    return ds;
}
