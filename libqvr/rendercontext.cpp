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


QVRRenderContext::QVRRenderContext() :
    _processIndex(-1),
    _windowIndex(-1),
    _windowRect(),
    _screenRect(),
    _screenWall { QVector3D(), QVector3D(), QVector3D() },
    _outputMode(QVR_Output_Center),
    _viewPasses(0),
    _eye { QVR_Eye_Center, QVR_Eye_Center },
    _eyeMatrix { QMatrix4x4(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
                 QMatrix4x4(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f) },
    _frustum { { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f } },
    _viewMatrix { QMatrix4x4(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
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
    case QVR_Output_Stereo_GL:
        _viewPasses = 2;
        _eye[0] = QVR_Eye_Left;
        _eye[1] = QVR_Eye_Right;
        break;
    case QVR_Output_Stereo_Red_Cyan:
        _viewPasses = 2;
        _eye[0] = QVR_Eye_Left;
        _eye[1] = QVR_Eye_Right;
        break;
    case QVR_Output_Stereo_Green_Magenta:
        _viewPasses = 2;
        _eye[0] = QVR_Eye_Left;
        _eye[1] = QVR_Eye_Right;
        break;
    case QVR_Output_Stereo_Amber_Blue:
        _viewPasses = 2;
        _eye[0] = QVR_Eye_Left;
        _eye[1] = QVR_Eye_Right;
        break;
    case QVR_Output_Stereo_Oculus:
        _viewPasses = 2;
        _eye[0] = QVR_Eye_Left;
        _eye[1] = QVR_Eye_Right;
        break;
    case QVR_Output_Stereo_Custom:
        _viewPasses = 2;
        _eye[0] = QVR_Eye_Left;
        _eye[1] = QVR_Eye_Right;
        break;
    }
}

void QVRRenderContext::serialize(QDataStream& ds) const
{
    ds << _processIndex << _windowIndex
        << _windowRect << _screenRect
        << _screenWall[0] << _screenWall[1] << _screenWall[2]
        << static_cast<int>(_outputMode)
        << _viewPasses
        << static_cast<int>(_eye[0]) << static_cast<int>(_eye[1])
        << _eyeMatrix[0] << _eyeMatrix[1]
        << _frustum[0] << _frustum[1]
        << _viewMatrix[0] << _viewMatrix[1];
}

void QVRRenderContext::deserialize(QDataStream& ds)
{
    int om, e0, e1;
    ds >> _processIndex >> _windowIndex
        >> _windowRect >> _screenRect
        >> _screenWall[0] >> _screenWall[1] >> _screenWall[2]
        >> om
        >> _viewPasses
        >> e0 >> e1
        >> _eyeMatrix[0] >> _eyeMatrix[1]
        >> _frustum[0] >> _frustum[1]
        >> _viewMatrix[0] >> _viewMatrix[1];
    _outputMode = static_cast<QVROutputMode>(om);
    _eye[0] = static_cast<QVREye>(e0);
    _eye[1] = static_cast<QVREye>(e1);
}
