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
#ifdef HAVE_VRPN
    _vrpnTrackerRemote = NULL;
    _vrpnAnalogRemote = NULL;
    _vrpnButtonRemote = NULL;
#endif
}

QVRDevice::QVRDevice(int deviceIndex) :
    _index(deviceIndex)
{
#ifdef HAVE_VRPN
    _vrpnTrackerRemote = NULL;
    _vrpnAnalogRemote = NULL;
    _vrpnButtonRemote = NULL;
#endif

    _isTracked = false;
    switch (config().trackingType()) {
    case QVR_Device_Tracking_None:
        break;
    case QVR_Device_Tracking_Static:
        _isTracked = true;
        {
            QStringList args = config().trackingParameters().split(' ', QString::SkipEmptyParts);
            if (args.length() == 3 || args.length() == 6)
                _position = QVector3D(args[0].toFloat(), args[1].toFloat(), args[2].toFloat());
            if (args.length() == 6)
                _orientation = QQuaternion::fromEulerAngles(args[3].toFloat(), args[4].toFloat(), args[5].toFloat());
        }
        break;
    case QVR_Device_Tracking_VRPN:
        _isTracked = true;
#ifdef HAVE_VRPN
        if (QVRManager::processIndex() == 0) {
            QStringList args = config().trackingParameters().split(' ', QString::SkipEmptyParts);
            QString name = (args.length() >= 1 ? args[0] : config().trackingParameters());
            int sensor = (args.length() >= 2 ? args[1].toInt() : vrpn_ALL_SENSORS);
            _vrpnTrackerRemote = new vrpn_Tracker_Remote(qPrintable(name));
            vrpn_System_TextPrinter.set_ostream_to_use(stderr);
            _vrpnTrackerRemote->register_change_handler(this, QVRVrpnTrackerChangeHandler, sensor);
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
        if (QVRManager::processIndex() == 0) {
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
            _vrpnButtonRemote = new vrpn_Button_Remote(qPrintable(name));
            vrpn_System_TextPrinter.set_ostream_to_use(stderr);
            _vrpnButtonRemote->register_change_handler(this, QVRVrpnButtonChangeHandler);
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
        if (QVRManager::processIndex() == 0) {
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
            _vrpnAnalogRemote = new vrpn_Analog_Remote(qPrintable(name));
            vrpn_System_TextPrinter.set_ostream_to_use(stderr);
            _vrpnAnalogRemote->register_change_handler(this, QVRVrpnAnalogChangeHandler);
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
#ifdef HAVE_VRPN
    if (_vrpnTrackerRemote)
        _vrpnTrackerRemote->mainloop();
    if (_vrpnButtonRemote)
        _vrpnButtonRemote->mainloop();
    if (_vrpnAnalogRemote)
        _vrpnAnalogRemote->mainloop();
#endif
}

QDataStream &operator<<(QDataStream& ds, const QVRDevice& d)
{
    ds << d._index << d._isTracked << d._position << d._orientation << d._buttons << d._analogs;
    return ds;
}

QDataStream &operator>>(QDataStream& ds, QVRDevice& d)
{
    ds >> d._index >> d._isTracked >> d._position >> d._orientation >> d._buttons >> d._analogs;
    return ds;
}
