/*
 * Copyright (C) 2016, 2017 Computer Graphics Group, University of Siegen
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


QVREvent::QVREvent() :
    type(QVR_Event_KeyPress),
    context(),
    keyEvent(QEvent::None, 0, Qt::NoModifier),
    mouseEvent(QEvent::None, QPointF(), Qt::NoButton, Qt::NoButton, Qt::NoModifier),
    wheelEvent(QPointF(), QPointF(), QPoint(), QPoint(), 0, Qt::Horizontal, Qt::NoButton, Qt::NoModifier),
    deviceEvent(QVRDevice(), -1, -1)
{}

QVREvent::QVREvent(QVREventType t, const QVRRenderContext& c, const QKeyEvent& e) :
    type(t),
    context(c),
    keyEvent(e),
    mouseEvent(QEvent::None, QPointF(), Qt::NoButton, Qt::NoButton, Qt::NoModifier),
    wheelEvent(QPointF(), QPointF(), QPoint(), QPoint(), 0, Qt::Horizontal, Qt::NoButton, Qt::NoModifier),
    deviceEvent(QVRDevice(), -1, -1)
{}

QVREvent::QVREvent(QVREventType t, const QVRRenderContext& c, const QMouseEvent& e) :
    type(t),
    context(c),
    keyEvent(QEvent::None, 0, Qt::NoModifier),
    mouseEvent(e),
    wheelEvent(QPointF(), QPointF(), QPoint(), QPoint(), 0, Qt::Horizontal, Qt::NoButton, Qt::NoModifier),
    deviceEvent(QVRDevice(), -1, -1)
{}

QVREvent::QVREvent(QVREventType t, const QVRRenderContext& c, const QWheelEvent& e) :
    type(t),
    context(c),
    keyEvent(QEvent::None, 0, Qt::NoModifier),
    mouseEvent(QEvent::None, QPointF(), Qt::NoButton, Qt::NoButton, Qt::NoModifier),
    wheelEvent(e),
    deviceEvent(QVRDevice(), -1, -1)
{}

QVREvent::QVREvent(QVREventType t, const QVRDeviceEvent& e) :
    type(t),
    context(),
    keyEvent(QEvent::None, 0, Qt::NoModifier),
    mouseEvent(QEvent::None, QPointF(), Qt::NoButton, Qt::NoButton, Qt::NoModifier),
    wheelEvent(QPointF(), QPointF(), QPoint(), QPoint(), 0, Qt::Horizontal, Qt::NoButton, Qt::NoModifier),
    deviceEvent(e)
{}

QDataStream &operator<<(QDataStream& ds, const QVREvent& e)
{
    ds << static_cast<int>(e.type);
    switch (e.type) {
    case QVR_Event_KeyPress:
    case QVR_Event_KeyRelease:
        ds << e.context
            << static_cast<int>(e.keyEvent.type())
            << e.keyEvent.key()
            << static_cast<int>(e.keyEvent.modifiers());
        break;
    case QVR_Event_MouseMove:
    case QVR_Event_MousePress:
    case QVR_Event_MouseRelease:
    case QVR_Event_MouseDoubleClick:
        ds << e.context
            << static_cast<int>(e.mouseEvent.type())
            << e.mouseEvent.localPos()
            << static_cast<int>(e.mouseEvent.button())
            << static_cast<int>(e.mouseEvent.buttons())
            << static_cast<int>(e.mouseEvent.modifiers());
        break;
    case QVR_Event_Wheel:
        ds << e.context
            << e.wheelEvent.posF()
            << e.wheelEvent.globalPosF()
            << e.wheelEvent.pixelDelta()
            << e.wheelEvent.angleDelta()
            << static_cast<int>(e.wheelEvent.buttons())
            << static_cast<int>(e.wheelEvent.modifiers());
        break;
    case QVR_Event_DeviceButtonPress:
    case QVR_Event_DeviceButtonRelease:
    case QVR_Event_DeviceAnalogChange:
        ds << e.deviceEvent.device()
            << e.deviceEvent.buttonIndex()
            << e.deviceEvent.analogIndex();
    }
    return ds;
}

QDataStream &operator>>(QDataStream& ds, QVREvent& e)
{
    int type;
    ds >> type;
    e.type = static_cast<QVREventType>(type);

    int ke[3];
    int me[4];
    QPointF mepf;
    QPointF wepf[2];
    QPoint wep[2];
    int we[2];
    QVRDevice d;
    int de[2];

    switch (e.type) {
    case QVR_Event_KeyPress:
    case QVR_Event_KeyRelease:
        ds >> e.context
            >> ke[0]
            >> ke[1]
            >> ke[2];
        e.keyEvent = QKeyEvent(static_cast<QEvent::Type>(ke[0]), ke[1], static_cast<Qt::KeyboardModifier>(ke[2]));
        break;
    case QVR_Event_MouseMove:
    case QVR_Event_MousePress:
    case QVR_Event_MouseRelease:
    case QVR_Event_MouseDoubleClick:
        ds >> e.context
            >> me[0]
            >> mepf
            >> me[1]
            >> me[2]
            >> me[3];
        e.mouseEvent = QMouseEvent(static_cast<QEvent::Type>(me[0]), mepf,
                static_cast<Qt::MouseButton>(me[1]), static_cast<Qt::MouseButtons>(me[2]),
                static_cast<Qt::KeyboardModifier>(me[3]));
        break;
    case QVR_Event_Wheel:
        ds >> e.context
            >> wepf[0]
            >> wepf[1]
            >> wep[0]
            >> wep[1]
            >> we[0]
            >> we[1];
        e.wheelEvent = QWheelEvent(wepf[0], wepf[1], wep[0], wep[1], 0, Qt::Horizontal,
                static_cast<Qt::MouseButtons>(we[0]), static_cast<Qt::KeyboardModifier>(we[1]));
        break;
    case QVR_Event_DeviceButtonPress:
    case QVR_Event_DeviceButtonRelease:
    case QVR_Event_DeviceAnalogChange:
        ds >> d
            >> de[0]
            >> de[1];
        e.deviceEvent = QVRDeviceEvent(d, de[0], de[1]);
        break;
    }
    return ds;
}
