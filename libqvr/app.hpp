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
class QVRDeviceEvent;
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
 * - To support your own navigation scheme, implement it in update(), and call
 *   QVRManager::init() with the appropriate flag to signal that your applications
 *   prefers its own navigation method.
 * - To support multi-process configurations (for multi-GPU or networked rendering),
 *   implement serializeDynamicData() and deserializeDynamicData(), and in special cases
 *   also serializeStaticData() and deserializeStaticData().
 * - To handle keyboard and mouse events, implement keyPressEvent(), keyReleaseEvent(),
 *   mouseMoveEvent(), mousePressEvent(), mouseReleaseEvent(), mouseDoubleClickEvent(),
 *   or wheelEvent() like you would in any Qt application.
 *
 * A key concept of QVR is that an application uses only one OpenGL context.
 * This context is available to almost all functions in the interface.
 */
class QVRApp {
public:
    /*!
     * \brief Render the current frame.
     * \param w         The window
     * \param context   The render context with information on how to render
     * \param textures  The textures to render into
     *
     * This function is called for each window to render the current frame into textures.
     * Monoscopic windows require one view per frame (for a single eye), and stereoscopic
     * windows require two views (one for each eye). The \a context provides information
     * about the required views and how to render them.
     *
     * A typical implementation does the following:
     * \code{.cpp}
     * for (int view = 0; view < context.viewCount(); view++) {
     *     setup_framebuffer_object(textures[view], context.textureSize(view));
     *     QMatrix4x4 projMatrix = context.frustum(view).toMatrix4x4();
     *     QMatrix4x4 viewMatrix = context.viewMatrix(view);
     *     set_and_clear_viewport(...);
     *     render_my_scene(...);
     * }
     * \endcode
     *
     * This example renders one or two views sequentially. As an alternative, some
     * platform-dependent OpenGL extensions allow to render two views at once to reduce
     * rendering costs.
     */
    virtual void render(QVRWindow* w, const QVRRenderContext& context, const unsigned int* textures) = 0;

    /*!
     * \brief Update scene state.
     * \param observers         A list of observers that the application may modify.
     *
     * Update scene state, e.g. for animations and interaction.
     *
     * You can query the state of interaction devices via \a QVRManager::deviceCount() and \a QVRManager::device().
     * This is useful for implementing interaction, tracking, and navigation functionality (all optional).
     *
     * The list of \a observers contains all observers in the active configuration.
     * You can update these observers to implement your own tracking and/or navigation functionality (optional).
     * You should only update an observer's navigation information via \a QVRObserver::setNavigation()
     * if its navigation type is \a QVR_Navigation_Custom.
     * You should only update an observer's tracking information via \a QVRObserver::setTracking()
     * if its tracking type is \a QVR_Tracking_Custom.
     *
     * Called once before each frame on the master process.
     */
    virtual void update(const QList<QVRObserver*>& observers) { Q_UNUSED(observers); }

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
     * \brief Handle a key press event.
     * \param context   The context that this event originated from
     * \param event     The event
     *
     * All events from all windows on all processes are gathered by the \a QVRManager.
     *
     * This function is called once before each frame on the master process, before
     * update().
     */
    virtual void keyPressEvent(const QVRRenderContext& context, QKeyEvent* event) { Q_UNUSED(context); Q_UNUSED(event); }

    /*!
     * \brief Handle a key release event.
     * \param context   The context that this event originated from
     * \param event     The event
     *
     * All events from all windows on all processes are gathered by the \a QVRManager.
     *
     * This function is called once for each event before each frame on the
     * master process, before update().
     */
    virtual void keyReleaseEvent(const QVRRenderContext& context, QKeyEvent* event) { Q_UNUSED(context); Q_UNUSED(event); }

    /*!
     * \brief Handle a mouse move release event.
     * \param context   The context that this event originated from
     * \param event     The event
     *
     * All events from all windows on all processes are gathered by the \a QVRManager.
     *
     * This function is called once for each event before each frame on the
     * master process, before update().
     */
    virtual void mouseMoveEvent(const QVRRenderContext& context, QMouseEvent* event) { Q_UNUSED(context); Q_UNUSED(event); }

    /*!
     * \brief Handle a mouse press event.
     * \param context   The context that this event originated from
     * \param event     The event
     *
     * All events from all windows on all processes are gathered by the \a QVRManager.
     *
     * This function is called once for each event before each frame on the
     * master process, before update().
     */
    virtual void mousePressEvent(const QVRRenderContext& context, QMouseEvent* event) { Q_UNUSED(context); Q_UNUSED(event); }

    /*!
     * \brief Handle a mouse release event.
     * \param context   The context that this event originated from
     * \param event     The event
     *
     * All events from all windows on all processes are gathered by the \a QVRManager.
     *
     * This function is called once for each event before each frame on the
     * master process, before update().
     */
    virtual void mouseReleaseEvent(const QVRRenderContext& context, QMouseEvent* event) { Q_UNUSED(context); Q_UNUSED(event); }

    /*!
     * \brief Handle a mouse double click event.
     * \param context   The context that this event originated from
     * \param event     The event
     *
     * All events from all windows on all processes are gathered by the \a QVRManager.
     *
     * This function is called once for each event before each frame on the
     * master process, before update().
     */
    virtual void mouseDoubleClickEvent(const QVRRenderContext& context, QMouseEvent* event) { Q_UNUSED(context); Q_UNUSED(event); }

    /*!
     * \brief Handle a wheel event.
     * \param context   The context that this event originated from
     * \param event     The event
     *
     * All events from all windows on all processes are gathered by the \a QVRManager.
     *
     * This function is called once for each event before each frame on the
     * master process, before update().
     */
    virtual void wheelEvent(const QVRRenderContext& context, QWheelEvent* event) { Q_UNUSED(context); Q_UNUSED(event); }

    /*!
     * \brief Handle a device button press event.
     * \param event     The event
     *
     * All events from all devices are gathered by the \a QVRManager.
     *
     * This function is called once for each event before each frame on the
     * master process, before update().
     */
    virtual void deviceButtonPressEvent(QVRDeviceEvent* event) { Q_UNUSED(event); }

    /*!
     * \brief Handle a device button release event.
     * \param event     The event
     *
     * All events from all devices are gathered by the \a QVRManager.
     *
     * This function is called once for each event before each frame on the
     * master process, before update().
     */
    virtual void deviceButtonReleaseEvent(QVRDeviceEvent* event) { Q_UNUSED(event); }

    /*!
     * \brief Handle a device analog element change event.
     * \param event     The event
     *
     * All events from all devices are gathered by the \a QVRManager.
     *
     * This function is called once for each event before each frame on the
     * master process, before update().
     */
    virtual void deviceAnalogChangeEvent(QVRDeviceEvent* event) { Q_UNUSED(event); }
};

#endif
