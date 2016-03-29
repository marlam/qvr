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

#include <QDataStream>

#include "frustum.hpp"


QVRFrustum::QVRFrustum() : _lrbtnf { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }
{
}

QVRFrustum::QVRFrustum(float l, float r, float b, float t, float n, float f) :
    _lrbtnf { l, r, b, t, n, f }
{
}

QVRFrustum::QVRFrustum(const float* lrbtnf)
{
    for (int i = 0; i < 6; i++)
        _lrbtnf[i] = lrbtnf[i];
}

QMatrix4x4 QVRFrustum::toMatrix4x4() const
{
    QMatrix4x4 m;
    m.frustum(leftPlane(), rightPlane(), bottomPlane(), topPlane(), nearPlane(), farPlane());
    return m;
}

void QVRFrustum::adjustNearPlane(float n)
{
    float q = n / nearPlane();
    setLeftPlane(leftPlane() * q);
    setRightPlane(rightPlane() * q);
    setBottomPlane(bottomPlane() * q);
    setTopPlane(topPlane() * q);
    setNearPlane(n);
}

QDataStream &operator<<(QDataStream& ds, const QVRFrustum& f)
{
    ds << f._lrbtnf[0] << f._lrbtnf[1] << f._lrbtnf[2] << f._lrbtnf[3] << f._lrbtnf[4] << f._lrbtnf[5];
    return ds;
}

QDataStream &operator>>(QDataStream& ds, QVRFrustum& f)
{
    ds >> f._lrbtnf[0] >> f._lrbtnf[1] >> f._lrbtnf[2] >> f._lrbtnf[3] >> f._lrbtnf[4] >> f._lrbtnf[5];
    return ds;
}
