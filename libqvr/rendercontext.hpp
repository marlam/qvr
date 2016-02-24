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

#ifndef QVR_RENDERCONTEXT_HPP
#define QVR_RENDERCONTEXT_HPP

#include <QRect>
#include <QVector3D>
#include <QMatrix4x4>

#include "config.hpp"
#include "frustum.hpp"

class QDataStream;

class QVRRenderContext
{
private:
    int _processIndex;
    int _windowIndex;
    QRect _windowRect;
    QRect _screenRect;
    QVector3D _screenWall[3];
    QVROutputMode _outputMode;
    int _viewPasses;
    QVREye _eye[2];
    QMatrix4x4 _eyeMatrix[2];
    QVRFrustum _frustum[2];
    QMatrix4x4 _viewMatrix[2];

public:
    QVRRenderContext();

    int processIndex() const { return _processIndex; }
    int windowIndex() const { return _windowIndex; }
    QRect windowRect() const { return _windowRect; }
    QRect screenRect() const { return _screenRect; }
    QVector3D screenWallBottomLeft() const { return _screenWall[0]; }
    QVector3D screenWallBottomRight() const { return _screenWall[1]; }
    QVector3D screenWallTopLeft() const { return _screenWall[2]; }
    QVROutputMode outputMode() const { return _outputMode; }
    int viewPasses() const { return _viewPasses; }
    QVREye eye(int viewPass) const { return _eye[viewPass]; }
    const QMatrix4x4& eyeMatrix(int viewPass) const { return _eyeMatrix[viewPass]; }
    const QVRFrustum& frustum(int viewPass) const { return _frustum[viewPass]; }
    const QMatrix4x4& viewMatrix(int viewPass) const { return _viewMatrix[viewPass]; }

    void setProcessIndex(int pi) { _processIndex = pi; }
    void setWindowIndex(int wi) { _windowIndex = wi; }
    void setWindowRect(const QRect& r) { _windowRect = r; }
    void setScreenRect(const QRect& r) { _screenRect = r; }
    void setScreenWall(const QVector3D& bl, const QVector3D& br, const QVector3D& tl)
    { _screenWall[0] = bl; _screenWall[1]= br; _screenWall[2] = tl; }
    void setOutputConf(QVROutputMode om);
    void setEyeMatrix(int vp, const QMatrix4x4& em) { _eyeMatrix[vp] = em; }
    void setFrustum(int vp, const QVRFrustum f) { _frustum[vp] = f; }
    void setViewMatrix(int vp, const QMatrix4x4& vm) { _viewMatrix[vp] = vm; }

    void serialize(QDataStream& ds) const;
    void deserialize(QDataStream& ds);
};

#endif
