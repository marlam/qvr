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
    _viewPasses(0),
    _eye { QVR_Eye_Center, QVR_Eye_Center },
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
        _viewPasses = 1;
        _eye[0] = QVR_Eye_Center;
        break;
    case QVR_Output_Left:
        _viewPasses = 1;
        _eye[0] = QVR_Eye_Left;
        break;
    case QVR_Output_Right:
        _viewPasses = 1;
        _eye[0] = QVR_Eye_Right;
        break;
    case QVR_Output_OSVR:
        _viewPasses = 2;
        _eye[0] = QVR_Eye_Left;
        _eye[1] = QVR_Eye_Right;
#ifdef HAVE_OSVR
        {
            Q_ASSERT(QVROsvrDisplayConfig);
            OSVR_EyeCount eyes;
            osvrClientGetNumEyesForViewer(QVROsvrDisplayConfig, 0, &eyes);
            if (eyes == 1) {
                _viewPasses = 1;
                _eye[0] = QVR_Eye_Center;
            }
        }
#endif
        break;
    case QVR_Output_Stereo_GL:
    case QVR_Output_Stereo_Red_Cyan:
    case QVR_Output_Stereo_Green_Magenta:
    case QVR_Output_Stereo_Amber_Blue:
    case QVR_Output_Stereo_Oculus:
    case QVR_Output_Stereo_OpenVR:
    case QVR_Output_Stereo_Custom:
        _viewPasses = 2;
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
        << rc._viewPasses
        << static_cast<int>(rc._eye[0]) << static_cast<int>(rc._eye[1])
        << rc._trackingPosition[0] << rc._trackingPosition[1]
        << rc._trackingOrientation[0] << rc._trackingOrientation[1]
        << rc._frustum[0] << rc._frustum[1]
        << rc._viewMatrix[0] << rc._viewMatrix[1]
        << rc._viewMatrixPure[0] << rc._viewMatrixPure[1];
    return ds;
}

QDataStream &operator>>(QDataStream& ds, QVRRenderContext& rc)
{
    int om, e0, e1;
    ds >> rc._processIndex >> rc._windowIndex
        >> rc._windowGeometry >> rc._screenGeometry
        >> rc._navigationPosition >> rc._navigationOrientation
        >> rc._screenWall[0] >> rc._screenWall[1] >> rc._screenWall[2]
        >> om
        >> rc._viewPasses
        >> e0 >> e1
        >> rc._trackingPosition[0] >> rc._trackingPosition[1]
        >> rc._trackingOrientation[0] >> rc._trackingOrientation[1]
        >> rc._frustum[0] >> rc._frustum[1]
        >> rc._viewMatrix[0] >> rc._viewMatrix[1]
        >> rc._viewMatrixPure[0] >> rc._viewMatrixPure[1];
    rc._outputMode = static_cast<QVROutputMode>(om);
    rc._eye[0] = static_cast<QVREye>(e0);
    rc._eye[1] = static_cast<QVREye>(e1);
    return ds;
}
