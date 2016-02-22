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

#ifndef QVR_OBSERVER_HPP
#define QVR_OBSERVER_HPP

#include <QMatrix4x4>

#ifdef HAVE_VRPN
# include <vrpn_Tracker.h>
#endif

#include "config.hpp"

class QDataStream;

class QVRObserver
{
private:
    int _index;
    QMatrix4x4 _matrix[3];
#ifdef HAVE_VRPN
    vrpn_Tracker_Remote* _vrpnTracker;
#endif

public:
    QVRObserver();
    QVRObserver(int index);
    ~QVRObserver();

    int index() const;
    const QString& id() const;
    const QVRObserverConfig& config() const;

    void serialize(QDataStream& ds) const;
    void deserialize(QDataStream& ds);

    const QMatrix4x4& eyeMatrix(QVREye eye) const
    {
        return _matrix[eye];
    }

    QVector3D eyePosition(QVREye eye) const
    {
        return eyeMatrix(eye).column(3).toVector3D();
    }

    void setEyeMatrices(const QMatrix4x4& centerMatrix,
            float eyeDistance = QVRObserverConfig::defaultEyeDistance);

    void setEyeMatrices(const QMatrix4x4& leftMatrix, const QMatrix4x4& rightMatrix);

    void update();
};

#endif
