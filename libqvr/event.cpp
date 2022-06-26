/*
 * Copyright (C) 2016, 2017, 2018, 2019, 2020, 2021, 2022
 * Computer Graphics Group, University of Siegen
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
    deviceEvent(QVRDevice(), -1, -1),
    keyEventType(QEvent::None),
    keyEventKey(0),
    keyEventModifiers(Qt::NoModifier),
    keyEventNativeScanCode(0),
    keyEventNativeVirtualKey(0),
    keyEventNativeModifiers(0),
    keyEventText(),
    keyEventAutorepeat(false),
    keyEventCount(0)
{}

QVREvent::QVREvent(QVREventType t, const QVRDeviceEvent& e) :
    type(t),
    context(),
    deviceEvent(e)
{}

QVREvent::QVREvent(QVREventType t, const QVRRenderContext& c, const QKeyEvent& e) :
    type(t),
    context(c),
    deviceEvent(QVRDevice(), -1, -1),
    keyEventType(e.type()),
    keyEventKey(e.key()),
    keyEventModifiers(e.modifiers()),
    keyEventNativeScanCode(e.nativeScanCode()),
    keyEventNativeVirtualKey(e.nativeVirtualKey()),
    keyEventNativeModifiers(e.nativeModifiers()),
    keyEventText(e.text()),
    keyEventAutorepeat(e.isAutoRepeat()),
    keyEventCount(e.count())
{}

QVREvent::QVREvent(QVREventType t, const QVRRenderContext& c, const QMouseEvent& e) :
    type(t),
    context(c),
    deviceEvent(QVRDevice(), -1, -1),
    mouseEventType(e.type()),
    mouseEventPosition(e.position()),
    mouseEventScenePosition(e.scenePosition()),
    mouseEventGlobalPosition(e.globalPosition()),
    mouseEventButton(e.button()),
    mouseEventButtons(e.buttons()),
    mouseEventModifiers(e.modifiers())
{}

QVREvent::QVREvent(QVREventType t, const QVRRenderContext& c, const QWheelEvent& e) :
    type(t),
    context(c),
    deviceEvent(QVRDevice(), -1, -1),
    wheelEventPosition(e.position()),
    wheelEventGlobalPosition(e.globalPosition()),
    wheelEventPixelDelta(e.pixelDelta()),
    wheelEventAngleDelta(e.angleDelta()),
    wheelEventButtons(e.buttons()),
    wheelEventModifiers(e.modifiers()),
    wheelEventPhase(e.phase()),
    wheelEventInverted(e.inverted())
{}

QKeyEvent* QVREvent::createKeyEvent() const
{
    return new QKeyEvent(keyEventType, keyEventKey, keyEventModifiers,
            keyEventNativeScanCode, keyEventNativeVirtualKey, keyEventNativeModifiers,
            keyEventText, keyEventAutorepeat, keyEventCount);
}

QMouseEvent* QVREvent::createMouseEvent() const
{
    return new QMouseEvent(mouseEventType, mouseEventPosition, mouseEventScenePosition,
            mouseEventGlobalPosition, mouseEventButton, mouseEventButtons, mouseEventModifiers);
}

QWheelEvent* QVREvent::createWheelEvent() const
{
    return new QWheelEvent(wheelEventPosition, wheelEventGlobalPosition, wheelEventPixelDelta,
            wheelEventAngleDelta, wheelEventButtons, wheelEventModifiers, wheelEventPhase,
            wheelEventInverted);
}

QDataStream &operator<<(QDataStream& ds, const QVREvent& e)
{
    ds << static_cast<int>(e.type) << e.context;
    switch (e.type) {
    case QVR_Event_KeyPress:
    case QVR_Event_KeyRelease:
        ds << static_cast<int>(e.keyEventType)
            << e.keyEventKey
            << static_cast<int>(e.keyEventModifiers)
            << e.keyEventNativeScanCode
            << e.keyEventNativeVirtualKey
            << e.keyEventNativeModifiers
            << e.keyEventText
            << e.keyEventAutorepeat
            << e.keyEventCount;
        break;
    case QVR_Event_MouseMove:
    case QVR_Event_MousePress:
    case QVR_Event_MouseRelease:
    case QVR_Event_MouseDoubleClick:
        ds << static_cast<int>(e.mouseEventType)
            << e.mouseEventPosition
            << e.mouseEventScenePosition
            << e.mouseEventGlobalPosition
            << static_cast<int>(e.mouseEventButton)
            << static_cast<int>(e.mouseEventButtons)
            << static_cast<int>(e.mouseEventModifiers);
        break;
    case QVR_Event_Wheel:
        ds << e.wheelEventPosition
            << e.wheelEventGlobalPosition
            << e.wheelEventPixelDelta
            << e.wheelEventAngleDelta
            << static_cast<int>(e.wheelEventButtons)
            << static_cast<int>(e.wheelEventModifiers)
            << static_cast<int>(e.wheelEventPhase)
            << e.wheelEventInverted;
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
    ds >> e.context;

    int intval;
    switch (e.type) {
    case QVR_Event_KeyPress:
    case QVR_Event_KeyRelease:
        ds >> intval;
        e.keyEventType = static_cast<QEvent::Type>(intval);
        ds >> e.keyEventKey;
        ds >> intval;
        e.keyEventModifiers = static_cast<Qt::KeyboardModifiers>(intval);
        ds >> e.keyEventNativeScanCode
            >> e.keyEventNativeVirtualKey
            >> e.keyEventNativeModifiers
            >> e.keyEventText
            >> e.keyEventAutorepeat
            >> e.keyEventCount;
        break;
    case QVR_Event_MouseMove:
    case QVR_Event_MousePress:
    case QVR_Event_MouseRelease:
    case QVR_Event_MouseDoubleClick:
        ds >> intval;
        e.mouseEventType = static_cast<QEvent::Type>(intval);
        ds >> e.mouseEventPosition
            >> e.mouseEventScenePosition
            >> e.mouseEventGlobalPosition
            >> intval;
        e.mouseEventButton = static_cast<Qt::MouseButton>(intval);
        ds >> intval;
        e.mouseEventButtons = static_cast<Qt::MouseButtons>(intval);
        ds >> intval;
        e.mouseEventModifiers = static_cast<Qt::KeyboardModifiers>(intval);
        break;
    case QVR_Event_Wheel:
        ds >> e.wheelEventPosition
            >> e.wheelEventGlobalPosition
            >> e.wheelEventPixelDelta
            >> e.wheelEventAngleDelta
            >> intval;
        e.wheelEventButtons = static_cast<Qt::MouseButtons>(intval);
        ds >> intval;
        e.wheelEventModifiers = static_cast<Qt::KeyboardModifiers>(intval);
        ds >> intval;
        e.wheelEventPhase = static_cast<Qt::ScrollPhase>(intval);
        ds >> e.wheelEventInverted;
        break;
    case QVR_Event_DeviceButtonPress:
    case QVR_Event_DeviceButtonRelease:
    case QVR_Event_DeviceAnalogChange:
        {
            QVRDevice d;
            int de[2];
            ds >> d >> de[0] >> de[1];
            e.deviceEvent = QVRDeviceEvent(d, de[0], de[1]);
        }
        break;
    }
    return ds;
}
