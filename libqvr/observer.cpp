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

#include <QQuaternion>

#include "manager.hpp"
#include "observer.hpp"


#ifdef HAVE_VRPN
static void vrpnPosChangeHandler(void* userdata, const vrpn_TRACKERCB info)
{
    QVRObserver* o = reinterpret_cast<QVRObserver*>(userdata);
    QMatrix4x4 m;
    m.rotate(QQuaternion(info.quat[3], info.quat[0], info.quat[1], info.quat[2]));
    m.translate(QVector3D(info.pos[0], info.pos[1], info.pos[2]));
    o->setEyeMatrices(m);
}
#endif


QVRObserver::QVRObserver() :
    _index(-1)
{
#ifdef HAVE_VRPN
    _vrpnTracker = NULL;
#endif
}

QVRObserver::QVRObserver(int observerIndex) :
    _index(observerIndex)
{
    _matrix[QVR_Eye_Center] = config().initialEyeMatrix(QVR_Eye_Center);
    _matrix[QVR_Eye_Left] = config().initialEyeMatrix(QVR_Eye_Left);
    _matrix[QVR_Eye_Right] = config().initialEyeMatrix(QVR_Eye_Right);
#ifdef HAVE_VRPN
    if (config().type() == QVR_Observer_VRPN) {
        QStringList args = config().parameters().split(' ', QString::SkipEmptyParts);
        QString name = (args.length() >= 2 ? args[0] : config().parameters());
        int sensor = (args.length() >= 2 ? args[1].toInt() : vrpn_ALL_SENSORS);
        _vrpnTracker = new vrpn_Tracker_Remote(qPrintable(name));
        vrpn_System_TextPrinter.set_ostream_to_use(stderr);
        _vrpnTracker->register_change_handler(this, vrpnPosChangeHandler, sensor);
    } else {
        _vrpnTracker = NULL;
    }
#endif
}

QVRObserver::~QVRObserver()
{
#ifdef HAVE_VRPN
    delete _vrpnTracker;
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

void QVRObserver::serialize(QDataStream& ds) const
{
    ds << _index << _matrix[0] << _matrix[1] << _matrix[2];
}

void QVRObserver::deserialize(QDataStream& ds)
{
    ds >> _index >> _matrix[0] >> _matrix[1] >> _matrix[2];
}

void QVRObserver::setEyeMatrices(const QMatrix4x4& centerMatrix, float eyeDistance)
{
    _matrix[QVR_Eye_Center] = centerMatrix;
    _matrix[QVR_Eye_Left] = centerMatrix;
    _matrix[QVR_Eye_Left].translate(-0.5f * eyeDistance, 0.0f, 0.0f);
    _matrix[QVR_Eye_Right] = centerMatrix;
    _matrix[QVR_Eye_Right].translate(+0.5f * eyeDistance, 0.0f, 0.0f);
}

void QVRObserver::setEyeMatrices(const QMatrix4x4& leftMatrix, const QMatrix4x4& rightMatrix)
{
    // To compute the center matrix, we simply average left and right matrix.
    // This works if both eyes have the same orientation and only their
    // position differs. Let's hope that other cases are irrelevant...
    _matrix[QVR_Eye_Center] = 0.5f * (leftMatrix + rightMatrix);
    _matrix[QVR_Eye_Left] = leftMatrix;
    _matrix[QVR_Eye_Right] = rightMatrix;
}

void QVRObserver::update()
{
#ifdef HAVE_VRPN
    if (_vrpnTracker)
        _vrpnTracker->mainloop();
#endif
}
