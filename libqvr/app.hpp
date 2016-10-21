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

class QVRDevice;
class QVRObserver;
class QVRWindow;
class QVRProcess;

#include "rendercontext.hpp"

/*!
 * \brief QVR application interface.
 *
 * This is the central interface that VR applications must implement.
 * Most functions provide a default implementation.
 * The only function that \a must be implemented is render().
 *
 * Overview:
 * - For rendering, implement render().
 * - To animate your scene, implement update().
 * - To override the default near and far plane, implement getNearFar().
 * - To signal when your application wants to quit, implement wantExit().
 * - For special process-specific actions, implement initProcess(), exitProcess(),
 *   preRenderProcess(), or postRenderProcess().
 * - For special window-specific actions, implement initWindow(), exitWindow(),
 *   preRenderWindow(), or postRenderWindow().
 * - To support your own tracking system that is unsupported by QVR, implement
 *   updateObservers().
 * - To support multi-process configurations (for multi-GPU or networked rendering),
 *   implement serializeDynamicData() and deserializeDynamicData(), and in special cases
 *   also serializeStaticData() and deserializeStaticData().
 * - To handle keyboard and mouse events, implement keyPressEvent(), keyReleaseEvent(),
 *   mouseMoveEvent(), mousePressEvent(), mouseReleaseEvent(), mouseDoubleClickEvent(),
 *   or wheelEvent() like you would in any Qt application.
 *
 * A key concept of QVR is that an application uses only one OpenGL context.
 * This context is available to all functions in the interface, with the exception
 * of the serialization functions.
 */
class QVRApp {
public:
    /*!
     * \brief Render the current frame.
     * \param w         The window
     * \param context   The render context with information about the current view
     * \param viewPass  The current viewPass for this render call
     * \param texture   The texture to render into
     *
     * This function is called for each window potentially multiple times, once
     * for each view pass, to render the current frame.
     *
     * The \a context holds all relevant information that the application needs
     * to render the view. Typically this is the frustum and the view matrix:
     * \code{.cpp}
     * QMatrix4x4 P = context.frustum(viewPass).toMatrix4x4();
     * QMatrix4x4 V = context.viewMatrix(viewPass);
     * \endcode
     *
     * The view must be rendered into the given \a texture, so the first thing
     * this function typically does is to attach the texture to a framebuffer
     * object that is then used for rendering.
     */
    virtual void render(QVRWindow* w,
            const QVRRenderContext& context,
            int viewPass, unsigned int texture) = 0;

    /*!
     * \brief Update scene state.
     * \param devices   A list of Virtual Reality interaction devices
     *
     * Update scene state, e.g. for animations.
     *
     * The \a devices list can be used to implement your own interaction schemes
     * based on VR interaction devices.
     *
     * Called once before each frame on the master process. Immediately afterwards,
     * \a updateObservers() is called.
     */
    virtual void update(const QList<const QVRDevice*>& devices) { Q_UNUSED(devices); }

    /*!
     * \brief Set the near and far clipping plane.
     * \param nearPlane Near clipping plane
     * \param farPlane  Far clipping plane
     *
     * Called once before each frame on the master process.
     */
    virtual void getNearFar(float& nearPlane, float& farPlane) { nearPlane = 0.05f; farPlane = 100.0f; }

    /*!
     * \brief Indicate if the application wants to exit.
     *
     * When this function returns true, the application will be terminated by the
     * \a QVRManager. Typically an application returns true when ESC is pressed.
     *
     * Called once before each frame on the master process.
     */
    virtual bool wantExit() { return false; }

    /*!
     * \brief Initialize an appplication process.
     * \param p         The process
     *
     * This function typically sets up process-specific data for rendering, e.g. OpenGL
     * textures and vertex array buffers.
     *
     * This function is called from QVRManager::init(). If it returns false, then
     * \a QVRManager initialization will fail.
     *
     * Called once per process at initialization time.
     */
    virtual bool initProcess(QVRProcess* p) { Q_UNUSED(p); return true; }

    /*!
     * \brief Cleanup an application process before exiting.
     * \param p         The process
     *
     * This function cleans up process-specific data.
     *
     * It is called by QVRManager when the application is terminated.
     *
     * Called once per process at termination time.
     */
    virtual void exitProcess(QVRProcess* p) { Q_UNUSED(p); }

    /*!
     * \brief Perform actions once before each frame on each process.
     * \param p         The process
     *
     * This function is called once for each process before each frame.
     */
    virtual void preRenderProcess(QVRProcess* p) { Q_UNUSED(p); }

    /*!
     * \brief Perform actions once after each frame on each process.
     * \param p         The process
     *
     * This function is called once for each process after each frame.
     */
    virtual void postRenderProcess(QVRProcess* p) { Q_UNUSED(p); }

    /*!
     * \brief Initialize a window.
     * \param w         The window
     *
     * This function may set up window-specific data. This is usually unnecessary.
     *
     * This function is called from QVRManager::init(). If it returns false, then
     * \a QVRManager initialization will fail.
     *
     * Called once per window at initialization time.
     */
    virtual bool initWindow(QVRWindow* w) { Q_UNUSED(w); return true; }

    /*!
     * \brief Cleanup a window before exiting.
     * \param w         The window
     *
     * This function cleans up window-specific data (this is usually unnecessary).
     *
     * It is called by QVRManager when the application is terminated.
     *
     * Called once per window at termination time.
     */
    virtual void exitWindow(QVRWindow* w) { Q_UNUSED(w); }

    /*!
     * \brief Perform actions once before each frame on each window.
     * \param w         The window
     *
     * This function is called once for each window before each frame.
     */
    virtual void preRenderWindow(QVRWindow* w) { Q_UNUSED(w); }

    /*!
     * \brief Perform actions once after each frame on each window.
     * \param w         The window
     *
     * This function is called once for each window after each frame.
     */
    virtual void postRenderWindow(QVRWindow* w) { Q_UNUSED(w); }

    /*!
     * \brief Serialize data that changes between frames.
     * \param ds        Stream to write the serialization data to.
     *
     * Implement this if you want to support multi-process configurations.
     * Only serialize data that is required by render() and may change;
     * this is typically what you manipulate in update().
     *
     * See also deserializeDynamicData().
     */
    virtual void serializeDynamicData(QDataStream& ds) const { Q_UNUSED(ds); }

    /*!
     * \brief Deserialize data that changes between frames.
     * \param ds        Stream to read the serialization data from.
     *
     * Implement this if you want to support multi-process configurations.
     * Only serialize data that is required by render() and may change;
     * this is typically what you manipulate in update().
     *
     * See also serializeDynamicData().
     */
    virtual void deserializeDynamicData(QDataStream& ds) { Q_UNUSED(ds); }

    /*!
     * \brief Serialize data that does not change after initialization.
     * \param ds        Stream to write the serialization data to.
     *
     * Only implement this if you want to support multi-process configurations
     * and your application benefits from transferring initialized data to slave
     * processes instead of initializing the data in each process.
     *
     * See also deserializeStaticData().
     */
    virtual void serializeStaticData(QDataStream& ds) const { Q_UNUSED(ds); }

    /*!
     * \brief Deserialize data that does not change after initialization.
     * \param ds        Stream to read the serialization data from.
     *
     * Only implement this if you want to support multi-process configurations
     * and your application benefits from transferring initialized data to slave
     * processes instead of initializing the data in each process.
     *
     * See also serializeStaticData().
     */
    virtual void deserializeStaticData(QDataStream& ds) { Q_UNUSED(ds); }

    /*!
     * \brief Update observers for custom tracking and navigation implementations.
     * \param devices           A list of Virtual Reality interaction devices.
     * \param customObservers   A list of observers that the application may modify.
     *
     * Update tracking and/or navigation information for observers of type
     * \a QVR_Observer_Custom. Use this to implement your own navigation methods,
     * and/or your own tracking system.
     *
     * Called once before each frame on the master process, after \a update().
     */
    virtual void updateObservers(const QList<const QVRDevice*>& devices, const QList<QVRObserver*>& customObservers) { Q_UNUSED(devices); Q_UNUSED(customObservers); }

    /*!
     * \brief Handle a key press event.
     * \param context   The context that this event originated from
     * \param event     The event
     *
     * All events from all windows on all processes are gathered by the \a QVRManager.
     *
     * This function is called once before each frame on the master process, before
     * update() and updateObservers().
     */
    virtual void keyPressEvent(const QVRRenderContext& context, QKeyEvent* event) { Q_UNUSED(context); Q_UNUSED(event); }

    /*!
     * \brief Handle a key release event.
     * \param context   The context that this event originated from
     * \param event     The event
     *
     * All events from all windows on all processes are gathered by the \a QVRManager.
     *
     * This function is called once before each frame on the master process, before
     * update() and updateObservers().
     */
    virtual void keyReleaseEvent(const QVRRenderContext& context, QKeyEvent* event) { Q_UNUSED(context); Q_UNUSED(event); }

    /*!
     * \brief Handle a mouse move release event.
     * \param context   The context that this event originated from
     * \param event     The event
     *
     * All events from all windows on all processes are gathered by the \a QVRManager.
     *
     * This function is called once before each frame on the master process, before
     * update() and updateObservers().
     */
    virtual void mouseMoveEvent(const QVRRenderContext& context, QMouseEvent* event) { Q_UNUSED(context); Q_UNUSED(event); }

    /*!
     * \brief Handle a mouse press event.
     * \param context   The context that this event originated from
     * \param event     The event
     *
     * All events from all windows on all processes are gathered by the \a QVRManager.
     *
     * This function is called once before each frame on the master process, before
     * update() and updateObservers().
     */
    virtual void mousePressEvent(const QVRRenderContext& context, QMouseEvent* event) { Q_UNUSED(context); Q_UNUSED(event); }

    /*!
     * \brief Handle a mouse release event.
     * \param context   The context that this event originated from
     * \param event     The event
     *
     * All events from all windows on all processes are gathered by the \a QVRManager.
     *
     * This function is called once before each frame on the master process, before
     * update() and updateObservers().
     */
    virtual void mouseReleaseEvent(const QVRRenderContext& context, QMouseEvent* event) { Q_UNUSED(context); Q_UNUSED(event); }

    /*!
     * \brief Handle a mouse double click event.
     * \param context   The context that this event originated from
     * \param event     The event
     *
     * All events from all windows on all processes are gathered by the \a QVRManager.
     *
     * This function is called once before each frame on the master process, before
     * update() and updateObservers().
     */
    virtual void mouseDoubleClickEvent(const QVRRenderContext& context, QMouseEvent* event) { Q_UNUSED(context); Q_UNUSED(event); }

    /*!
     * \brief Handle a wheel event.
     * \param context   The context that this event originated from
     * \param event     The event
     *
     * All events from all windows on all processes are gathered by the \a QVRManager.
     *
     * This function is called once before each frame on the master process, before
     * update() and updateObservers().
     */
    virtual void wheelEvent(const QVRRenderContext& context, QWheelEvent* event) { Q_UNUSED(context); Q_UNUSED(event); }
};

#endif
