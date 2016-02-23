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

#ifndef QVR_APP_HPP
#define QVR_APP_HPP

class QDataStream;
class QRect;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
class QMatrix4x4;
template <typename T> class QList;

class QVRObserver;
class QVRWindow;
class QVRProcess;

class QVRApp {
public:
    // The following functions need to be implemented only if your application
    // should support multi-process QVR configurations. Single-process configurations
    // will never need them.
    // Use the (de)serializeStaticData() functions to (de)serialize data that does
    // not change once your QVRApp is initialized.
    // Use the (de)serializeDynamicData() functions to (de)serialize data that does
    // change between frames, typically when the update() function is called.
    virtual void serializeStaticData(QDataStream& /* ds */) const {}
    virtual void deserializeStaticData(QDataStream& /* ds */) {}
    virtual void serializeDynamicData(QDataStream& /* ds */) const {}
    virtual void deserializeDynamicData(QDataStream& /* ds */) {}

    /* All following functions have a valid GL context, and it is always the same context. */

    // Set the near and far clipping plane values that your application needs.
    // Called once before each frame on all processes.
    virtual void getNearFar(float* n, float* f) { *n = 0.05f; *f = 100.0f; }

    // Update scene state (animations etc). Optionally update position and
    // orientation of the given custom observers (or simply ignore them).
    // Called once before each frame on the master process.
    // All changes made here need to be handled by serializeDynamicData() and
    // deserializeDynamicData() if you want to support multi-process
    // configurations.
    virtual void update(const QList<QVRObserver*>& /* customObservers */) {}

    // Called once per frame on the master process to see if the application
    // wants to exit.
    virtual bool wantExit() { return false; }

    // Called only once per process, from QVRManager::init(). If this returns
    // false, the QVRManager initialization will fail.
    virtual bool initProcess(QVRProcess* /* p */) { return true; }
    // Called only once per process, when QVRManager ends the application.
    virtual void exitProcess(QVRProcess* /* p */) {}

    // Called only once per window, from QVRManager::init(). If this returns
    // false, the QVRManager initialization will fail.
    virtual bool initWindow(QVRWindow* /* w */) { return true; }
    // Called only once per window, when QVRManager ends the application.
    virtual void exitWindow(QVRWindow* /* w */) {}

    // Called once per frame and process before rendering.
    virtual void preRenderProcess(QVRProcess* /* p */) {}
    // Called once per frame and window before rendering.
    virtual void preRenderWindow(QVRWindow* /* w */) {}
    // Called potentially multiple times for each window per frame.
    // Does the actual rendering.
    virtual void render(QVRWindow* w,
            unsigned int fboTex,
            const float* frustumLrbtnf,
            const QMatrix4x4& viewMatrix) = 0;
    // Called once per frame and window after rendering.
    virtual void postRenderWindow(QVRWindow* /* w */) {}
    // Called once per frame and process after rendering.
    virtual void postRenderProcess(QVRProcess* /* p */) {}

    /*
     * Qt event handling
     *
     * All events from all windows of all processes are sent to the master process
     * (index 0) by QVRManager, and only on this process will the following event
     * handlers be called. Each event still carries information about the process
     * and window it originated on, in case you need that information.
     *
     * All state changes you make in the event handlers need to be handled by
     * serializeDynamicData() and deserializeDynamicData() if you want to support
     * multi-process configurations.
     *
     * These functions do not have valid OpenGL contexts.
     */
    virtual void keyPressEvent(const QVRRenderContext& /* context */, QKeyEvent* /* event */) {}
    virtual void keyReleaseEvent(const QVRRenderContext& /* context */, QKeyEvent* /* event */) {}
    virtual void mouseMoveEvent(const QVRRenderContext& /* context */, QMouseEvent* /* event */) {}
    virtual void mousePressEvent(const QVRRenderContext& /* context */, QMouseEvent* /* event */) {}
    virtual void mouseReleaseEvent(const QVRRenderContext& /* context */, QMouseEvent* /* event */) {}
    virtual void mouseDoubleClickEvent(const QVRRenderContext& /* context */, QMouseEvent* /* event */) {}
    virtual void wheelEvent(const QVRRenderContext& /* context */, QWheelEvent* /* event */) {}
};

#endif
