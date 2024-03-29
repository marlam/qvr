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

#ifndef QVR_EVENT_HPP
#define QVR_EVENT_HPP

#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

#include "device.hpp"
#include "rendercontext.hpp"

class QDataStream;

typedef enum {
    QVR_Event_KeyPress,
    QVR_Event_KeyRelease,
    QVR_Event_MouseMove,
    QVR_Event_MousePress,
    QVR_Event_MouseRelease,
    QVR_Event_MouseDoubleClick,
    QVR_Event_Wheel,
    QVR_Event_DeviceButtonPress,
    QVR_Event_DeviceButtonRelease,
    QVR_Event_DeviceAnalogChange
} QVREventType;

class QVREvent
{
public:
    QVREventType type;
    QVRRenderContext context;
    QVRDeviceEvent deviceEvent;
    /* for QKeyEvent: */
    QEvent::Type keyEventType;
    int keyEventKey;
    Qt::KeyboardModifiers keyEventModifiers;
    quint32 keyEventNativeScanCode;
    quint32 keyEventNativeVirtualKey;
    quint32 keyEventNativeModifiers;
    QString keyEventText;
    bool keyEventAutorepeat;
    quint16 keyEventCount;
    /* for QMouseEvent: */
    QEvent::Type mouseEventType;
    QPointF mouseEventPosition;
    QPointF mouseEventScenePosition;
    QPointF mouseEventGlobalPosition;
    Qt::MouseButton mouseEventButton;
    Qt::MouseButtons mouseEventButtons;
    Qt::KeyboardModifiers mouseEventModifiers;
    /* for QWheelEvent: */
    QPointF wheelEventPosition;
    QPointF wheelEventGlobalPosition;
    QPoint wheelEventPixelDelta;
    QPoint wheelEventAngleDelta;
    Qt::MouseButtons wheelEventButtons;
    Qt::KeyboardModifiers wheelEventModifiers;
    Qt::ScrollPhase wheelEventPhase;
    bool wheelEventInverted;

    QVREvent();
    QVREvent(QVREventType t, const QVRDeviceEvent& e);
    QVREvent(QVREventType t, const QVRRenderContext& c, const QKeyEvent& e);
    QVREvent(QVREventType t, const QVRRenderContext& c, const QMouseEvent& e);
    QVREvent(QVREventType t, const QVRRenderContext& c, const QWheelEvent& e);

    QKeyEvent* createKeyEvent() const;
    QMouseEvent* createMouseEvent() const;
    QWheelEvent* createWheelEvent() const;
};

QDataStream &operator<<(QDataStream& ds, const QVREvent& e);
QDataStream &operator>>(QDataStream& ds, QVREvent& e);

#endif
