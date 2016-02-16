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
    _matrix[QVR_Eye_Left] = config().initialEyeMatrix(QVR_Eye_Left);
    _matrix[QVR_Eye_Right] = config().initialEyeMatrix(QVR_Eye_Right);
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
    ds << _index << _matrix[0] << _matrix[1];
}

void QVRObserver::deserialize(QDataStream& ds)
{
    ds >> _index >> _matrix[0] >> _matrix[1];
}

void QVRObserver::setEyeMatrices(const QMatrix4x4& centerMatrix, float eyeDistance)
{
    _matrix[QVR_Eye_Left] = centerMatrix;
    _matrix[QVR_Eye_Left].translate(-0.5f * eyeDistance, 0.0f, 0.0f);
    _matrix[QVR_Eye_Right] = centerMatrix;
    _matrix[QVR_Eye_Right].translate(+0.5f * eyeDistance, 0.0f, 0.0f);
}
