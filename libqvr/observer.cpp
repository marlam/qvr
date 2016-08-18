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
#include "observer.hpp"


QVRObserver::QVRObserver() :
    _index(-1)
{
}

QVRObserver::QVRObserver(int observerIndex) :
    _index(observerIndex)
{
    setNavigation(config().initialNavigationPosition(), config().initialNavigationOrientation());
    setEyeDistance(config().initialEyeDistance());
    setTracking(config().initialTrackingPosition(), config().initialTrackingOrientation());
}

QVRObserver::~QVRObserver()
{
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
