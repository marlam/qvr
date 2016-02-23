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

#ifndef QVR_EVENT_HPP
#define QVR_EVENT_HPP

#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QMatrix4x4>

#include "rendercontext.hpp"

class QDataStream;

typedef enum {
    QVR_Event_KeyPress,
    QVR_Event_KeyRelease,
    QVR_Event_MouseMove,
    QVR_Event_MousePress,
    QVR_Event_MouseRelease,
    QVR_Event_MouseDoubleClick,
    QVR_Event_Wheel
} QVREventType;

class QVREvent
{
public:
    QVREventType type;
    QVRRenderContext context;
    QKeyEvent keyEvent;
    QMouseEvent mouseEvent;
    QWheelEvent wheelEvent;

    void serialize(QDataStream& ds) const;
    void deserialize(QDataStream& ds);

    QVREvent();
    QVREvent(QVREventType t, const QVRRenderContext& c, QKeyEvent e);
    QVREvent(QVREventType t, const QVRRenderContext& c, QMouseEvent e);
    QVREvent(QVREventType t, const QVRRenderContext& c, QWheelEvent e);
};

#endif
