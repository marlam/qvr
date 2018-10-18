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

#include <QDataStream>

#include "rendercontext.hpp"
#include "internalglobals.hpp"


QVRRenderContext::QVRRenderContext() :
    _processIndex(-1),
    _windowIndex(-1),
    _windowGeometry(),
    _screenGeometry(),
    _navigationPosition(0.0f, 0.0f, 0.0f),
    _navigationOrientation(0.0f, 0.0f, 0.0f, 0.0f),
    _screenWall { QVector3D(), QVector3D(), QVector3D() },
    _outputMode(QVR_Output_Center),
    _viewCount(0),
    _eye { QVR_Eye_Center, QVR_Eye_Center },
    _textureSize { QSize(-1, -1), QSize(-1, -1) },
    _trackingPosition { QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0f, 0.0f, 0.0f) },
    _trackingOrientation { QQuaternion(0.0f, 0.0f, 0.0f, 0.0f), QQuaternion(0.0f, 0.0f, 0.0f, 0.0f) },
    _frustum { { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f } },
    _viewMatrix { QMatrix4x4(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
                  QMatrix4x4(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f) },
    _viewMatrixPure { QMatrix4x4(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
                  QMatrix4x4(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f) }
{
}

void QVRRenderContext::setOutputConf(QVROutputMode om)
{
    _outputMode = om;
    switch (om) {
    case QVR_Output_Center:
        _viewCount = 1;
        _eye[0] = QVR_Eye_Center;
        break;
    case QVR_Output_Left:
        _viewCount = 1;
        _eye[0] = QVR_Eye_Left;
        break;
    case QVR_Output_Right:
        _viewCount = 1;
        _eye[0] = QVR_Eye_Right;
        break;
    case QVR_Output_Stereo:
    case QVR_Output_Red_Cyan:
    case QVR_Output_Green_Magenta:
    case QVR_Output_Amber_Blue:
    case QVR_Output_Oculus:
    case QVR_Output_OpenVR:
    case QVR_Output_GoogleVR:
        _viewCount = 2;
        _eye[0] = QVR_Eye_Left;
        _eye[1] = QVR_Eye_Right;
        break;
    }
}

QDataStream &operator<<(QDataStream& ds, const QVRRenderContext& rc)
{
    ds << rc._processIndex << rc._windowIndex
        << rc._windowGeometry << rc._screenGeometry
        << rc._navigationPosition << rc._navigationOrientation
        << rc._screenWall[0] << rc._screenWall[1] << rc._screenWall[2]
        << static_cast<int>(rc._outputMode)
        << rc._viewCount;
    for (int i = 0; i < rc._viewCount; i++) {
        ds << static_cast<int>(rc._eye[i])
            << rc._textureSize[i]
            << rc._trackingPosition[i]
            << rc._trackingOrientation[i]
            << rc._frustum[i]
            << rc._viewMatrix[i]
            << rc._viewMatrixPure[i];
    }
    return ds;
}

QDataStream &operator>>(QDataStream& ds, QVRRenderContext& rc)
{
    int om;
    ds >> rc._processIndex >> rc._windowIndex
        >> rc._windowGeometry >> rc._screenGeometry
        >> rc._navigationPosition >> rc._navigationOrientation
        >> rc._screenWall[0] >> rc._screenWall[1] >> rc._screenWall[2]
        >> om
        >> rc._viewCount;
    rc._outputMode = static_cast<QVROutputMode>(om);
    for (int i = 0; i < rc._viewCount; i++) {
        int e;
        ds >> e
            >> rc._textureSize[i]
            >> rc._trackingPosition[i]
            >> rc._trackingOrientation[i]
            >> rc._frustum[i]
            >> rc._viewMatrix[i]
            >> rc._viewMatrixPure[i];
        rc._eye[i] = static_cast<QVREye>(e);
    }
    return ds;
}
