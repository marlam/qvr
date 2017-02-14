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

#ifndef QVR_CONFIG_HPP
#define QVR_CONFIG_HPP

#include <QString>
#include <QQuaternion>
#include <QPoint>
#include <QSize>
#include <QRect>
#include <QList>


/*!
 * \brief Device tracking type.
 */
typedef enum {
    /*! \brief An untracked device without position and orientation. */
    QVR_Device_Tracking_None,
    /*! \brief A untracked device with a static position and orientation. */
    QVR_Device_Tracking_Static,
    /*! \brief A device with position and orientation tracked via <a href="https://github.com/vrpn/vrpn/wiki">VRPN</a>. */
    QVR_Device_Tracking_VRPN,
    /*! \brief A device with position and orientation tracked via Oculus Rift. */
    QVR_Device_Tracking_Oculus,
    /*! \brief A device with position and orientation tracked via OpenVR (HTC Vive). */
    QVR_Device_Tracking_OpenVR,
    /*! \brief A device with position and orientation tracked via <a href="http://www.osvr.org/">OSVR</a>. */
    QVR_Device_Tracking_OSVR,
} QVRDeviceTrackingType;

/*!
 * \brief Device buttons type.
 */
typedef enum {
    /*! \brief An device without digital buttons. */
    QVR_Device_Buttons_None,
    /*! \brief A device with digital buttons that are static (never changed). */
    QVR_Device_Buttons_Static,
    /*! \brief A gamepad with digital buttons. */
    QVR_Device_Buttons_Gamepad,
    /*! \brief A device with digital buttons queried via <a href="https://github.com/vrpn/vrpn/wiki">VRPN</a>. */
    QVR_Device_Buttons_VRPN,
    /*! \brief A device with digital buttons queried via Oculus SDK. */
    QVR_Device_Buttons_Oculus,
    /*! \brief A device with digital buttons queried via OpenVR (HTC Vive). */
    QVR_Device_Buttons_OpenVR,
    /*! \brief A device with digital buttons queried via <a href="http://www.osvr.org/">OSVR</a>. */
    QVR_Device_Buttons_OSVR
} QVRDeviceButtonsType;

/*!
 * \brief Device analogs type.
 */
typedef enum {
    /*! \brief An device without analog joystick elements. */
    QVR_Device_Analogs_None,
    /*! \brief A device with analog joystick elements that are static (never changed). */
    QVR_Device_Analogs_Static,
    /*! \brief A gamepad with analog joystick elements. */
    QVR_Device_Analogs_Gamepad,
    /*! \brief A device with analog joystick elements queried via <a href="https://github.com/vrpn/vrpn/wiki">VRPN</a>. */
    QVR_Device_Analogs_VRPN,
    /*! \brief A device with analog joystick elements queried via Oculus SDK. */
    QVR_Device_Analogs_Oculus,
    /*! \brief A device with analog joystick elements queried via OpenVR (HTC Vive). */
    QVR_Device_Analogs_OpenVR,
    /*! \brief A device with analog joystick elements queried via <a href="http://www.osvr.org/">OSVR</a>. */
    QVR_Device_Analogs_OSVR
} QVRDeviceAnalogsType;

/*!
 * \brief Observer navigation type.
 */
typedef enum {
    /*! \brief An observer that never navigates anywhere. */
    QVR_Navigation_Stationary,
    /*! \brief An observer that navigates via a wand or flystick device.
     *
     * The wand must be a tracked device (position and orientation), and it must have two analog joystick elements for
     * movements in the x/z plane, and four digital buttons for movements based on the y axis (up, down, rotate left,
     * rotate right).
     */
    QVR_Navigation_Device,
    /*! \brief An observer with keyboard and mouse navigation (WASD+QE and lookaround).
     *
     * WASDQE observers navigate via a common keyboard and mouse mapping: the keys
     * WASD are use for forward, sideways, and backward movement, and the additional
     * keys QE move up and down. Mouse movements for looking left/right/up/down are
     * activated by clicking in a window and deactivated by pressing ESC.
     */
    QVR_Navigation_WASDQE,
    /*! \brief An observer with navigation implemented by QVRApp::update(). */
    QVR_Navigation_Custom
} QVRNavigationType;

/*!
 * \brief Observer tracking type.
 */
typedef enum {
    /*! \brief An observer that never moves. */
    QVR_Tracking_Stationary,
    /*! \brief An observer that is tracked via a tracked device. */
    QVR_Tracking_Device,
    /*! \brief An observer with tracking implemented by QVRApp::update(). */
    QVR_Tracking_Custom
} QVRTrackingType;

/*!
 * \brief Eye of an observer.
 */
typedef enum {
    /*! \brief The center between left and right eye. */
    QVR_Eye_Center = 0,
    /*! \brief The left eye. */
    QVR_Eye_Left = 1,
    /*! \brief The right eye. */
    QVR_Eye_Right = 2
} QVREye;

/*!
 * \brief Output mode of a \a QVRWindow.
 */
typedef enum {
    // these values are re-used in a GLSL shader; keep them in sync
    /*! \brief Output a monoscopic view for \a QVR_Eye_Center. */
    QVR_Output_Center = 0,
    /*! \brief Output a monoscopic view for \a QVR_Eye_Left. */
    QVR_Output_Left = 1,
    /*! \brief Output a monoscopic view for \a QVR_Eye_Right. */
    QVR_Output_Right = 2,
    /*! \brief Output a view for OSVR (monoscopic or stereoscopic). */
    QVR_Output_OSVR = 9,
    /*! \brief Output a stereoscopic view via OpenGL-supported quad-buffer stereo. */
    QVR_Output_Stereo_GL = 3,
    /*! \brief Output a stereoscopic view for red-cyan anaglyph glasses. */
    QVR_Output_Stereo_Red_Cyan = 4,
    /*! \brief Output a stereoscopic view for green-magenta anaglyph glasses. */
    QVR_Output_Stereo_Green_Magenta = 5,
    /*! \brief Output a stereoscopic view for amber-blue anaglyph glasses. */
    QVR_Output_Stereo_Amber_Blue = 6,
    /*! \brief Output a stereoscopic view for the Oculus Rift head-mounted display. */
    QVR_Output_Stereo_Oculus = 7,
    /*! \brief Output a stereoscopic view for the HTC Vive head-mounted display. */
    QVR_Output_Stereo_OpenVR = 10,
    /*! \brief Output a stereoscopic view via an output plugin; see \a QVROutputPlugin(). */
    QVR_Output_Stereo_Custom = 8
} QVROutputMode;

/*!
 * \brief Types of inter-process communication that can be used if multiple processes
 * are configured.
 */
typedef enum {
    /*! \brief TCP sockets. Processes may run on different hosts. */
    QVR_IPC_TcpSocket,
    /*! \brief Local sockets (UNIX domain sockets). All processes must run on the same host. */
    QVR_IPC_LocalSocket,
    /*! \brief Shared Memory. All processes must run on the same host. */
    QVR_IPC_SharedMemory,
    /*! \brief Automatic. QVR will choose \a QVR_IPC_TcpSocket if at least one process has
     * a launch command configured, and \a QVR_IPC_SharedMemory otherwise. */
    QVR_IPC_Automatic
} QVRIpcType;

/*!
 * \brief Configuration of a \a QVRDevice.
 */
class QVRDeviceConfig
{
private:
    // Unique identification string
    QString _id;
    // Index of owning process
    int _processIndex;
    // Types and parameters for tracking, buttons, and analogs
    QVRDeviceTrackingType _trackingType;
    QString _trackingParameters;
    QVRDeviceButtonsType _buttonsType;
    QString _buttonsParameters;
    QVRDeviceAnalogsType _analogsType;
    QString _analogsParameters;

    friend class QVRConfig;

public:
    /*! \brief Constructor. */
    QVRDeviceConfig();

    /*! \brief Returns the unique id. */
    const QString& id() const { return _id; }

    /*! \brief Returns the index of the owning process.
     *
     * This is 0 (the master process) by default, but in special
     * cases (such as remote processes) a different process can own a device.
     *
     * The corresponding configuration file entry for the device is
     * `process <id>` (note that the id and not the index is used).
     */
    int processIndex() const { return _processIndex; }

    /*! \brief Returns the tracking type. */
    QVRDeviceTrackingType trackingType() const { return _trackingType; }

    /*! \brief Returns the tracking parameters.
     *
     * For \a QVR_Device_Tracking_None, parameters are ignored.
     *
     * For \a QVR_Device_Tracking_Static, the parameter string is of the form
     * `[<pos-x> <pos-y> <pos-z>] [<pitch> <yaw> <roll>]` where the first
     * three numbers give the static position and the last three numbers give
     * the static orientation as euler angles.
     *
     * For \a QVR_Device_Tracking_VRPN, the parameter string is of the form
     * `<name> [<sensor>]` where `<name>` is the VRPN tracker name, e.g. Tracker0\@localhost,
     * and `<sensor>` is the number of the sensor to be used (can be omitted to use all).
     *
     * For \a QVR_Device_Tracking_Oculus, the parameter string must be one of "head",
     * "eye-left", "eye-right", "controller-left", and "controller-right".
     *
     * For \a QVR_Device_Tracking_OpenVR, the parameter string must be one of "head",
     * "eye-left", "eye-right", "controller-0", and "controller-1".
     *
     * For \a QVR_Device_Tracking_OSVR, the parameter string is the path
     * name of an OSVR tracker interface. The special strings "eye-center", "eye-left",
     * and "eye-right" identify the center, left, and right eyes of the viewer (these are
     * unfortunately not available via path names in OSVR).
     */
    const QString& trackingParameters() const { return _trackingParameters; }

    /*! \brief Returns the buttons type. */
    QVRDeviceButtonsType buttonsType() const { return _buttonsType; }

    /*! \brief Returns the buttons parameters.
     *
     * For \a QVR_Device_Buttons_None, parameters are ignored.
     *
     * For \a QVR_Device_Buttons_Static, the parameter string is a list of values.
     * Each value represent the static state of one button and must be 0 (button
     * not pressed) or 1 (button pressed).
     *
     * For \a QVR_Device_Buttons_Gamepad, the parameter string is the identifier
     * of the gamepad. The default is 0 which identifies the first gamepad found
     * by Qt; higher numbers identify additional controllers.
     * There will be the following 18 gamepad buttons: up, down, left, right,
     * l1, r1, l2, r2, l3, r3, a, b, x, y, center, guide, select, start.
     *
     * For \a QVR_Device_Buttons_VRPN, the parameter string is of the form
     * `<name> [<button0> [<button1> [...]]]` where `<name>` is the VRPN tracker name,
     * e.g. Tracker0\@localhost
     * and the optional button list specifies the number and order of VRPN buttons to use.
     *
     * For \a QVR_Device_Buttons_Oculus, the parameter string must be "xbox", "controller-left",
     * or "controller-right". The left controller has 8 buttons: Up, Down, Left, Right, X, Y,
     * Shoulder, Menu. The right controller has 8 buttons: Up, Down, Left, Right, A, B, Shoulder,
     * Menu. Note that the Up, Down, Left, Right buttons on controllers are only simulated:
     * they will report to be pressed when the analog value of their direction exceeds 0.5.
     * The xbox controller has 12 buttons: up, down, left, right, A, B, X, Y, left shoulder,
     * right shoulder, Menu, Back.
     *
     * For \a QVR_Device_Buttons_OpenVR, the parameter string must be either "controller-0"
     * or "controller-1". There will be 6 buttons: pad up (0), pad down (1), pad left (2),
     * pad right (3), menu (4), grip (5). Note that the four pad buttons are only simulated:
     * they will report to be pressed when the analog value of their direction exceeds 0.5.
     *
     * For \a QVR_Device_Buttons_OSVR, the parameter string is a space-separated list
     * of path names of OSVR button interfaces (one path name for each button managed by this device).
     */
    const QString& buttonsParameters() const { return _buttonsParameters; }

    /*! \brief Returns the analogs type. */
    QVRDeviceAnalogsType analogsType() const { return _analogsType; }

    /*! \brief Returns the analogs parameters.
     *
     * For \a QVR_Device_Analogs_None, parameters are ignored.
     *
     * For \a QVR_Device_Analogs_Static, the parameter string is a list of values.
     * Each value represent the static state of one button and must be in [-1,+1].
     *
     * For \a QVR_Device_Analogs_Gamepad, the parameter string is the identifier
     * of the gamepad. The default is 0 which identifies the first gamepad found
     * by Qt; higher numbers identify additional controllers.
     * There will be 4 gamepad analogs: right up/down, right left/right, left up/down,
     * left left/right.
     *
     * For \a QVR_Device_Analogs_VRPN, the parameter string is of the form
     * `<name> [<analog0> [<analog1> [...]]]` where `<name>` is the VRPN tracker name,
     * e.g. Tracker0\@localhost
     * and the optional analogs list specifies the number and order of VRPN analog joystick elements to use.
     *
     * For \a QVR_Device_Analogs_Oculus, the parameter string must be "xbox", "controller-left",
     * or "controller-right". The left and right controllers each have 4 analog elements: thumbstick up/down,
     * thumbstick left/right, trigger, grip. The xbox controller has 8 analog elements: thumbstick left up/down,
     * thumbstick left left/right, thumbstick right up/down, thumbstick right left/right, trigger left, trigger
     * right, grip left, grip right.
     *
     * For \a QVR_Device_Analogs_OpenVR, the parameter string must be either "controller-0"
     * or "controller-1". There will be 3 analogs: pad left-right axis (0), pad up-down axis (1),
     * trigger (2). Note that the pad axes will go from -1 to 1 while the trigger will go from 0 to 1.
     *
     * For \a QVR_Device_Analogs_OSVR, the parameter string is a space-separated list
     * of path names of OSVR analog interfaces (one path name for each analog managed by this device).
     */
    const QString& analogsParameters() const { return _analogsParameters; }
};

/*!
 * \brief Configuration of a \a QVRObserver.
 */
class QVRObserverConfig
{
private:
    // Unique identification string
    QString _id;
    // Types and parameters for navigation and tracking
    QVRNavigationType _navigationType;
    QString _navigationParameters;
    QVRTrackingType _trackingType;
    QString _trackingParameters;
    // Initial navigation position and orientation
    QVector3D _initialNavigationPosition;
    QVector3D _initialNavigationForwardDirection;
    QVector3D _initialNavigationUpDirection;
    // Initial distance between left and right eye
    float _initialEyeDistance;
    // Initial position and orientation for the center eye
    QVector3D _initialTrackingPosition;
    QVector3D _initialTrackingForwardDirection;
    QVector3D _initialTrackingUpDirection;

    friend class QVRConfig;

public:
    /*! \brief Default eye height: average human height minus average human offset to eye. */
    static constexpr float defaultEyeHeight = 1.76f - 0.15f;
    /*! \brief Default eye distance: average value for humans. */
    static constexpr float defaultEyeDistance = 0.064f;

    /*! \brief Constructor. */
    QVRObserverConfig();

    /*! \brief Returns the unique id. */
    const QString& id() const { return _id; }

    /*! \brief Returns the navigation type. */
    QVRNavigationType navigationType() const { return _navigationType; }

    /*! \brief Returns navigation parameters.
     *
     * For \a QVR_Navigation_Stationary, parameters are ignored.
     *
     * For \a QVR_Navigation_Device, the parameter is the id of the device to use.
     *
     * For \a QVR_Navigation_WASDQE, parameters are currently ignored.
     *
     * For \a QVR_Navigation_Custom, parameters are currently ignored by QVR, but may be used by applications.
     */
    const QString& navigationParameters() const { return _navigationParameters; }

    /*! \brief Returns the tracking type. */
    QVRTrackingType trackingType() const { return _trackingType; }

    /*! \brief Returns tracking parameters.
     *
     * For \a QVR_Tracking_Stationary, parameters are ignored.
     *
     * For \a QVR_Tracking_Device, the parameter is the id of the device to use. The pose of this device is
     * interpreted as the pose of the center eye; left and right eye poses are computed from this and depend on
     * the observer's eye distance. If two device ids are given, their poses are interpreted as the poses
     * of the left and right eye and used directly.
     *
     * For \a QVR_Tracking_Oculus, parameters are currently ignored.
     *
     * For \a QVR_Tracking_Custom, parameters are currently ignored by QVR, but may be used by applications.
     */
    const QString& trackingParameters() const { return _trackingParameters; }

    /*! \brief Returns the initial navigation position. */
    const QVector3D& initialNavigationPosition() const { return _initialNavigationPosition; }

    /*! \brief Returns the initial navigation forward direction. */
    const QVector3D& initialNavigationForwardDirection() const { return _initialNavigationForwardDirection; }

    /*! \brief Returns the initial navigation upward direction. */
    const QVector3D& initialNavigationUpDirection() const { return _initialNavigationUpDirection; }

    /*! \brief Returns the initial navigation orientation, computed from forward and up direction. */
    QQuaternion initialNavigationOrientation() const { return QQuaternion::fromDirection(-initialNavigationForwardDirection(), initialNavigationUpDirection()); }

    /*! \brief Returns the eye distance (interpupillary distance). */
    float initialEyeDistance() const { return _initialEyeDistance; }

    /*! \brief Returns the initial tracking position. */
    const QVector3D& initialTrackingPosition() const { return _initialTrackingPosition; }

    /*! \brief Returns the initial tracking forward direction. */
    const QVector3D& initialTrackingForwardDirection() const { return _initialTrackingForwardDirection; }

    /*! \brief Returns the initial tracking upward direction. */
    const QVector3D& initialTrackingUpDirection() const { return _initialTrackingUpDirection; }

    /*! \brief Returns the initial tracking orientation, computed from forward and up direction. */
    QQuaternion initialTrackingOrientation() const { return QQuaternion::fromDirection(-initialTrackingForwardDirection(), initialTrackingUpDirection()); }
};

/*!
 * \brief Configuration of a \a QVRWindow.
 *
 * A window is displayed on a display screen (in Qt terminology)
 * and has a position and size on that display screen.
 *
 * For the purpose of Virtual Reality rendering, a window represents a
 * screen wall given in virtual world coordinates. The function
 * screenIsGivenByCenter() determines whether these coordinates
 * are determined from virtual world coordinates of the screen
 * wall center, or from virtual world coordinates of the screen's
 * bottom left, bottom right, and top left corners.
 *
 * The Qt display screen and the virtual world screen wall should not be mixed up.
 */
class QVRWindowConfig
{
private:
    // Unique identification string
    QString _id;
    // The observer that gets to view this window.
    int _observerIndex;
    // Output mode.
    QVROutputMode _outputMode;
    // Output postprocessing.
    QString _outputPlugin;
    // Initial window configuration. This may change at runtime.
    int _initialDisplayScreen;
    bool _initialFullscreen;
    QPoint _initialPosition;
    QSize _initialSize;
    // Virtual world geometry of the screen area covered by this window.
    // This changes at runtime if the observer navigates.
    bool _screenIsFixedToObserver;
    QVector3D _screenCornerBottomLeft;
    QVector3D _screenCornerBottomRight;
    QVector3D _screenCornerTopLeft;
    // This is an alternative to the above,
    // the actual dimensions will be taken from monitor specs
    bool _screenIsGivenByCenter;
    QVector3D _screenCenter;
    // Factor for optional lower-resolution rendering
    float _renderResolutionFactor;

    friend class QVRConfig;

public:
    /*! \brief Constructor */
    QVRWindowConfig();

    /*! \brief Returns the unique id. */
    const QString& id() const { return _id; }
    /*! \brief Returns the index of the observer that views this window. */
    int observerIndex() const { return _observerIndex; }
    /*! \brief Returns the output mode of this window. */
    QVROutputMode outputMode() const { return _outputMode; }
    /*! \brief Returns the output plugin specification of this window (may be empty). */
    const QString& outputPlugin() const { return _outputPlugin; }
    /*! \brief Returns the initial display screen on which the window will appear (-1 for default). */
    int initialDisplayScreen() const { return _initialDisplayScreen; }
    /*! \brief Returns whether this window is initially in fullscreen mode. */
    bool initialFullscreen() const { return _initialFullscreen; }
    /*! \brief Returns the initial position of this window on the screen ((-1,-1) for default). */
    QPoint initialPosition() const { return _initialPosition; }
    /*! \brief Returns the initial size of this window on the screen. */
    QSize initialSize() const { return _initialSize; }
    /*! \brief Returns the initial geometry of this window on the screen, composed from initialPosition() and initialSize(). */
    QRect initialGeometry() const { return QRect(initialPosition(), initialSize()); }
    /*! \brief Returns whether the screen wall represented by this window is fixed to its observer. */
    bool screenIsFixedToObserver() const { return _screenIsFixedToObserver; }
    /*! \brief Returns whether the screen wall coordinates determined from its center or from three of its corners.
     *
     * When determining screen wall coordinates from center coordinates, the Qt display screen
     * dimensions are queried.
     */
    bool screenIsGivenByCenter() const { return _screenIsGivenByCenter; }
    /*! \brief Returns the virtual world coordinates of the bottom left corner of the screen wall. */
    const QVector3D& screenCornerBottomLeft() const { return _screenCornerBottomLeft; }
    /*! \brief Returns the virtual world coordinates of the bottom right corner of the screen wall. */
    const QVector3D& screenCornerBottomRight() const { return _screenCornerBottomRight; }
    /*! \brief Returns the virtual world coordinates of the top left corner of the screen wall. */
    const QVector3D& screenCornerTopLeft() const { return _screenCornerTopLeft; }
    /*! \brief Returns the virtual world coordinates of the center of the screen wall. */
    const QVector3D& screenCenter() const { return _screenCenter; }
    /*! \brief Returns the render resolution factor.
     *
     * A window can show scenes that were rendered at a different
     * resolution than the actual window size. This is useful for example if a
     * control window does not need to look good and can render at lower
     * resolution to save time. The window size will be multiplied by this factor
     * to get the resolution of the rendering textures. Example: a 800x600 window
     * with a render resolution factor 0.5 will cause the application to render
     * into a 400x300 texture which is then upscaled to 800x600 for display.
     */
    float renderResolutionFactor() const { return _renderResolutionFactor; }
};

/*!
 * \brief Configuration of a \a QVRProcess.
 *
 * Each process works with only one display. The master process talks to the 
 * display that Qt initially uses by default; if a different display is configured,
 * QVR will relaunch the master process automatically so that Qt talks to the
 * configured display insted. Slave processes are usually configured to use different displays.
 *
 * The idea is that one display (with potentially multiple screens) runs on one GPU.
 */
class QVRProcessConfig
{
private:
    // Unique identification string
    QString _id;
    // The communication system to use for inter-process communication.
    // Only relevant for the master process.
    QVRIpcType _ipc;
    // The IP address to bind the QVR server to. Only relevant with IPC type QVR_IPC_TcpScoket,
    // and only for the master process.
    QString _address;
    // The launcher command, e.g. ssh
    QString _launcher;
    // The display that this process works on. Only relevant for X11 at this time.
    QString _display;
    // The windows driven by this process.
    QList<QVRWindowConfig> _windowConfigs;

    friend class QVRConfig;

public:
    /*! \brief Constructor */
    QVRProcessConfig();

    /*! \brief Returns the unique id of this process. */
    const QString& id() const { return _id; }
    /*! \brief Returns the type of inter-process communication to use. */
    QVRIpcType ipc() const { return _ipc; }
    /*! \brief Returns the IP address that the QVR server will listen on.
     *
     * A QVR server is only started on the master process (which is the application
     * process that is started first), and only if multiple processes are configured.
     * A TCP QVR server that listens on a network address is only used if configured manually
     * or if at least one of the processes is run on a remote host (which is assumed to be the
     * case when a launcher is configured; see launcher()).
     *
     * By default, a TCP QVR server listens on all IP addresses of the host.
     */
    const QString& address() const { return _address; }
    /*! \brief Returns the launcher command used to launch this process.
     *
     * This is only required when processes need to run on remote computers,
     * but can also be useful for starting processes under e.g. <a href="http://valgrind.org/">valgrind</a>,
     * or with specific operating system settings, e.g. using `taskset`.
     *
     * Example for ssh (passwordless login to remotehost must be set up):
     * `launcher ssh remotehost`
     *
     * Example for ssh if you use libraries in non-standard locations:
     * `launcher ssh remotehost env LD_LIBRARY_PATH=/path/to/libs`
     *
     * When the special string `manual` is used as launcher command, QVR will
     * not start the process itself, and instead expects the user to start it
     * manually. A list of options that the process needs to be started with
     * will be printed to the terminal.
     */
    const QString& launcher() const { return _launcher; }
    /*! \brief Returns the display that this process works with. */
    const QString& display() const { return _display; }
    /*! \brief Returns the configurations of the windows on this process. */
    const QList<QVRWindowConfig>& windowConfigs() const { return _windowConfigs; }
};

/*!
 * \brief QVR configuration of a Virtual Reality display setup.
 *
 * The configuration consists of a list of observer configurations
 * and a list of process configurations. The process configurations
 * define the windows that each process drives. Each window provides
 * a view for exactly one observer.
 */
class QVRConfig
{
private:
    // Devices for interaction and tracking.
    QList<QVRDeviceConfig> _deviceConfigs;
    // The observers that get a view on the virtual world.
    QList<QVRObserverConfig> _observerConfigs;
    // The processes associated with this configuration.
    // The first process is always the one that is started first.
    // Each process config has a list of associated window configs.
    QList<QVRProcessConfig> _processConfigs;

public:
    /*! \brief Constructor. */
    QVRConfig();

    /*!
     * \brief Create a default configuration.
     * \param preferCustomNavigation   Use custom navigation for the default observer if feasible.
     *
     * This function will detect a head-mounted display and create a suitable
     * configuration for it.
     * If no special equipment is detected, the function will generate a single
     * observer viewing a single window on the master process.
     *
     * The observer created will a suitable default navigation type. If \a preferCustomNavigation
     * is set, then the navigation type \a QVR_Navigation_Custom will be used if it makes sense.
     */
    void createDefault(bool preferCustomNavigation);

    /*!
     * \brief Read a configuration file.
     * \param filename  The name of the configuration file.
     * \return False on failure.
     */
    bool readFromFile(const QString& filename);

    /*! \brief Returns the list of device configurations. */
    const QList<QVRDeviceConfig>& deviceConfigs() const { return _deviceConfigs; }
    /*! \brief Returns the list of observer configurations. */
    const QList<QVRObserverConfig>& observerConfigs() const { return _observerConfigs; }
    /*! \brief Returns the list of process configurations. */
    const QList<QVRProcessConfig>& processConfigs() const { return _processConfigs; }
};

#endif
