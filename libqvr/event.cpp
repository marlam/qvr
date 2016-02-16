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

#include "event.hpp"


void QVREvent::serialize(QDataStream& ds) const
{
    ds << processIndex << windowIndex << windowGeometry << screenGeometry;
    ds << frustum[0] << frustum[1] << frustum[2] << frustum[3] << frustum[4] << frustum[5];
    ds << viewMatrix;
    ds << static_cast<int>(type);
    ds << static_cast<int>(keyEvent.type()) << keyEvent.key() << static_cast<int>(keyEvent.modifiers());
    ds << static_cast<int>(mouseEvent.type()) << mouseEvent.localPos()
        << static_cast<int>(mouseEvent.button()) << static_cast<int>(mouseEvent.buttons())
        << static_cast<int>(mouseEvent.modifiers());
    ds << wheelEvent.posF() << wheelEvent.globalPosF()
        << wheelEvent.pixelDelta() << wheelEvent.angleDelta()
        << static_cast<int>(wheelEvent.buttons()) << static_cast<int>(wheelEvent.modifiers());
}

void QVREvent::deserialize(QDataStream& ds)
{
    int x[4];
    QPoint p[2];
    QPointF pf[2];
    ds >> processIndex >> windowIndex >> windowGeometry >> screenGeometry;
    ds >> frustum[0] >> frustum[1] >> frustum[2] >> frustum[3] >> frustum[4] >> frustum[5];
    ds >> viewMatrix;
    ds >> x[0];
    type = static_cast<QVREventType>(x[0]);
    ds >> x[0] >> x[1] >> x[2];
    keyEvent = QKeyEvent(static_cast<QEvent::Type>(x[0]), x[1], static_cast<Qt::KeyboardModifier>(x[2]));
    ds >> x[0];
    ds >> pf[0];
    ds >> x[1] >> x[2] >> x[3];
    mouseEvent = QMouseEvent(static_cast<QEvent::Type>(x[0]), pf[0],
            static_cast<Qt::MouseButton>(x[1]), static_cast<Qt::MouseButtons>(x[2]),
            static_cast<Qt::KeyboardModifier>(x[3]));
    ds >> pf[0] >> pf[1] >> p[0] >> p[1] >> x[0] >> x[1];
    wheelEvent = QWheelEvent(pf[0], pf[1], p[0], p[1], 0, Qt::Horizontal,
            static_cast<Qt::MouseButtons>(x[0]), static_cast<Qt::KeyboardModifier>(x[1]));
}

QVREvent::QVREvent() :
    processIndex(-1), windowIndex(-1), windowGeometry(), screenGeometry(),
    frustum { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
    viewMatrix(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
    type(QVR_Event_KeyPress),
    keyEvent(QEvent::None, 0, Qt::NoModifier),
    mouseEvent(QEvent::None, QPointF(), Qt::NoButton, Qt::NoButton, Qt::NoModifier),
    wheelEvent(QPointF(), QPointF(), QPoint(), QPoint(), 0, Qt::Horizontal, Qt::NoButton, Qt::NoModifier)
{}

QVREvent::QVREvent(int pi, int wi, const QRect& wG, const QRect& sG, const float* f, const QMatrix4x4& vM, QVREventType t, QKeyEvent e) :
    processIndex(pi), windowIndex(wi), windowGeometry(wG), screenGeometry(sG),
    frustum { f[0], f[1], f[2], f[3], f[4], f[5] },
    viewMatrix(vM),
    type(t),
    keyEvent(e),
    mouseEvent(QEvent::None, QPointF(), Qt::NoButton, Qt::NoButton, Qt::NoModifier),
    wheelEvent(QPointF(), QPointF(), QPoint(), QPoint(), 0, Qt::Horizontal, Qt::NoButton, Qt::NoModifier)
{}

QVREvent::QVREvent(int pi, int wi, const QRect& wG, const QRect& sG, const float* f, const QMatrix4x4& vM, QVREventType t, QMouseEvent e) :
    processIndex(pi), windowIndex(wi), windowGeometry(wG), screenGeometry(sG),
    frustum { f[0], f[1], f[2], f[3], f[4], f[5] },
    viewMatrix(vM),
    type(t),
    keyEvent(QEvent::None, 0, Qt::NoModifier),
    mouseEvent(e),
    wheelEvent(QPointF(), QPointF(), QPoint(), QPoint(), 0, Qt::Horizontal, Qt::NoButton, Qt::NoModifier)
{}

QVREvent::QVREvent(int pi, int wi, const QRect& wG, const QRect& sG, const float* f, const QMatrix4x4& vM, QVREventType t, QWheelEvent e) :
    processIndex(pi), windowIndex(wi), windowGeometry(wG), screenGeometry(sG),
    frustum { f[0], f[1], f[2], f[3], f[4], f[5] },
    viewMatrix(vM),
    type(t),
    keyEvent(QEvent::None, 0, Qt::NoModifier),
    mouseEvent(QEvent::None, QPointF(), Qt::NoButton, Qt::NoButton, Qt::NoModifier),
    wheelEvent(e)
{}
