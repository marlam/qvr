/*
 * Copyright (C) 2016, 2017, 2018 Computer Graphics Group, University of Siegen
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
#include <QFlags>


/*!
 * \brief Device tracking method.
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
    /*! \brief A device with position and orientation tracked via Google VR (Cardboard, Daydream). */
    QVR_Device_Tracking_GoogleVR,
} QVRDeviceTrackingType;

/*!
 * \brief Device buttons query method.
 */
typedef enum {
    /*! \brief A device without digital buttons. */
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
    /*! \brief A device with digital buttons queried via Google VR. */
    QVR_Device_Buttons_GoogleVR
} QVRDeviceButtonsType;

/*!
 * \brief Device analogs query method.
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
    /*! \brief A device with analog joystick elements queried via Google VR. */
    QVR_Device_Analogs_GoogleVR
} QVRDeviceAnalogsType;

/*!
 * \brief Device buttons.
 */
typedef enum {
    /*! \brief L1 button, typically found on gamepads. */
    QVR_Button_L1,
    /*! \brief L2 button, typically found on gamepads. */
    QVR_Button_L2,
    /*! \brief L3 button, typically found on some gamepads. */
    QVR_Button_L3,
    /*! \brief R1 button, typically found on gamepads. */
    QVR_Button_R1,
    /*! \brief R2 button, typically found on gamepads. */
    QVR_Button_R2,
    /*! \brief R3 button, typically found on some gamepads. */
    QVR_Button_R3,
    /*! \brief A button, typically found on gamepads. */
    QVR_Button_A,
    /*! \brief B button, typically found on gamepads. */
    QVR_Button_B,
    /*! \brief X button, typically found on gamepads. */
    QVR_Button_X,
    /*! \brief Y button, typically found on gamepads. */
    QVR_Button_Y,
    /*! \brief Up button, typically used together with other direction buttons. */
    QVR_Button_Up,
    /*! \brief Down button, typically used together with other direction buttons. */
    QVR_Button_Down,
    /*! \brief Left button, typically used together with other direction buttons. */
    QVR_Button_Left,
    /*! \brief Right button, typically used together with other direction buttons. */
    QVR_Button_Right,
    /*! \brief Center button, typically found on some gamepads. */
    QVR_Button_Center,
    /*! \brief Select button, typically found on some gamepads. */
    QVR_Button_Select,
    /*! \brief Start button, typically found on some gamepads. */
    QVR_Button_Start,
    /*! \brief Menu button. */
    QVR_Button_Menu,
    /*! \brief Back button. */
    QVR_Button_Back,
    /*! \brief Trigger button. */
    QVR_Button_Trigger,
    /*! \brief Unknown or unidentified button. */
    QVR_Button_Unknown // must be the last entry!
} QVRButton;

/*!
 * \brief Device analog elements.
 */
typedef enum {
    /*! \brief Trigger element, with values in [0,1]. */
    QVR_Analog_Trigger,
    /*! \brief Left hand trigger element, with values in [0,1]. */
    QVR_Analog_Left_Trigger = QVR_Analog_Trigger,
    /*! \brief Right hand trigger element, with values in [0,1]. */
    QVR_Analog_Right_Trigger,
    /*! \brief Grip element, with values in [0,1]. */
    QVR_Analog_Grip,
    /*! \brief Left hand grip element, with values in [0,1]. */
    QVR_Analog_Left_Grip = QVR_Analog_Grip,
    /*! \brief Right hand grip element, with values in [0,1]. */
    QVR_Analog_Right_Grip,
    /*! \brief Horizontal axis with values in [-1,1]. */
    QVR_Analog_Axis_X,
    /*! \brief Vertical axis with values in [-1,1]. */
    QVR_Analog_Axis_Y,
    /*! \brief Left hand horizontal axis with values in [-1,1]. */
    QVR_Analog_Left_Axis_X = QVR_Analog_Axis_X,
    /*! \brief Left hand vertical axis with values in [-1,1]. */
    QVR_Analog_Left_Axis_Y = QVR_Analog_Axis_Y,
    /*! \brief Right hand horizontal axis with values in [-1,1]. */
    QVR_Analog_Right_Axis_X,
    /*! \brief Right hand vertical axis with values in [-1,1]. */
    QVR_Analog_Right_Axis_Y,
    /*! \brief Unknown or unidentified analog element. */
    QVR_Analog_Unknown // must be the last entry!
} QVRAnalog;

/*!
 * \brief Observer navigation type.
 */
typedef enum {
    /*! \brief An observer that never navigates anywhere. */
    QVR_Navigation_Stationary,
    /*! \brief An observer that navigates via a controller device, e.g. wand, flystick, or gamepad.
     *
     * The following controller types should work:
     * - Four analog joystick elements (\a QVR_Analog_Left_Axis_X, \a QVR_Analog_Left_Axis_Y,
     *   \a QVR_Analog_Right_Axis_X, \a QVR_Analog_Right_Axis_Y).
     * - Two analog joystick elements (\a QVR_Analog_Axis_X, \a QVR_Analog_Axis_Y) and four digital buttons
     *   (\a QVR_Button_Up, \a QVR_Button_Down, \a QVR_Button_Left, \a QVR_Button_Down).
     * - Two analog joystick elements (\a QVR_Analog_Axis_X, \a QVR_Analog_Axis_Y) and less than
     *   four buttons.
     * - A single digital button and nothing else (special case for Google Cardboard).
     *
     * If the device is tracked, the movement directions are based on the device orientation. Otherwise,
     * they are based on the observer orientation.
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
    /*! \brief Output a stereoscopic view via OpenGL-supported quad-buffer stereo. */
    QVR_Output_Stereo = 3,
    /*! \brief Output a stereoscopic view for red-cyan anaglyph glasses. */
    QVR_Output_Red_Cyan = 4,
    /*! \brief Output a stereoscopic view for green-magenta anaglyph glasses. */
    QVR_Output_Green_Magenta = 5,
    /*! \brief Output a stereoscopic view for amber-blue anaglyph glasses. */
    QVR_Output_Amber_Blue = 6,
    /*! \brief Output a stereoscopic view for the Oculus Rift head-mounted display. */
    QVR_Output_Oculus = 7,
    /*! \brief Output a stereoscopic view for the HTC Vive head-mounted display. */
    QVR_Output_OpenVR = 8,
    /*! \brief Output a stereoscopic view for Google VR devices (Cardboard, Daydream). */
    QVR_Output_GoogleVR = 9
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
     * For \a QVR_Device_Tracking_GoogleVR, the parameter string must be one of "head",
     * "eye-left", "eye-right", "daydream".
     */
    const QString& trackingParameters() const { return _trackingParameters; }

    /*! \brief Returns the buttons type. */
    QVRDeviceButtonsType buttonsType() const { return _buttonsType; }

    /*! \brief Returns the buttons parameters.
     *
     * These parameters sometimes define button names. The following button names
     * have a special meaning that translates to a \a QVRButton value: l1, l2, l3,
     * r1, r2, r3, a, b, x, y, up, down, left, right, center, select, start, menu,
     * back.
     *
     * For \a QVR_Device_Buttons_None, parameters are ignored.
     *
     * For \a QVR_Device_Buttons_Static, the parameter string is a list of name / value pairs,
     * e.g. `up 0 down 1 left 0 right 0`. The value must be either 0 (button not pressed) or 1
     * (button pressed).
     *
     * For \a QVR_Device_Buttons_Gamepad, the parameter string is the identifier
     * of the gamepad. The default is 0 which identifies the first gamepad found
     * by Qt; higher numbers identify additional controllers.
     * There will be the following 18 gamepad buttons: up, down, left, right,
     * l1, r1, l2, r2, l3, r3, a, b, x, y, center, menu, select, start.
     *
     * For \a QVR_Device_Buttons_VRPN, the parameter string is of the form
     * `<tracker> [<name0> [<name1> [...]]]` where `<tracker>` is the VRPN tracker name,
     * e.g. Tracker0\@localhost
     * and the optional button name list specifies how many buttons the device has and
     * what meaning each button has. Example: `Tracker0\@localhost up down left right l1 l2`
     * defines 6 buttons (VRPN button indices 0, 1, 2, 3, 4, 5) and gives them a meaning
     * according to their name.
     *
     * For \a QVR_Device_Buttons_Oculus, the parameter string must be "xbox", "controller-left",
     * or "controller-right". The left controller has 8 buttons: up, down, left, right, x, y,
     * l1, menu. The right controller has 8 buttons: up, down, left, right, a, b, r1, menu.
     * Note that the up, down, left, right buttons on controllers are only simulated:
     * they will report to be pressed when the analog value of their direction exceeds 0.5.
     * The xbox controller has 12 buttons: up, down, left, right, a, b, x, y, l1, r1, menu, back.
     *
     * For \a QVR_Device_Buttons_OpenVR, the parameter string must be either "controller-0"
     * or "controller-1". There will be 6 buttons: up, down, left, right, menu, grip.
     * Note that the four direction buttons are only simulated: they will report to be pressed
     * when the analog value of their direction exceeds 0.5.
     *
     * For \a QVR_Device_Buttons_GoogleVR, the parameter string is either "touch" or "daydream".
     */
    const QString& buttonsParameters() const { return _buttonsParameters; }

    /*! \brief Returns the analogs type. */
    QVRDeviceAnalogsType analogsType() const { return _analogsType; }

    /*! \brief Returns the analogs parameters.
     *
     * These parameters sometimes define analog elemt names. The following names
     * have a special meaning that translates to a \a QVRAnalog value: trigger,
     * left-trigger, right-trigger, grip, left-grip, right-grip,
     * axis-x, axis-y, left-axis-x, left-axis-y, right-axis-x, right-axis-y.
     * Triggers and grips have values in [0,1], while axes have values in [-1,1].
     *
     * For \a QVR_Device_Analogs_None, parameters are ignored.
     *
     * For \a QVR_Device_Analogs_Static, the parameter string is a list of name / value pairs,
     * e.g. `trigger 0 axis-x 0.5`. The value must be in [0,1] or [-1,1].
     *
     * For \a QVR_Device_Analogs_Gamepad, the parameter string is the identifier
     * of the gamepad. The default is 0 which identifies the first gamepad found
     * by Qt; higher numbers identify additional controllers.
     * There will be 6 gamepad analogs: right-axis-y, right-axis-x, left-axis-y, left-axis-x,
     * right-trigger, left-trigger.
     *
     * For \a QVR_Device_Analogs_VRPN, the parameter string is of the form
     * `<tracker> [<name0> [<name1> [...]]]` where `<tracker>` is the VRPN tracker name,
     * e.g. Tracker0\@localhost
     * and the optional analog name list specifies how many analog elements the device has and
     * what meaning each of them has. Example: `Tracker0\@localhost trigger axis-x axis-y`
     * defines 3 analog elements (VRPN analog element indices 0, 1, 2) and gives them a meaning
     * according to their name.
     *
     * For \a QVR_Device_Analogs_Oculus, the parameter string must be "xbox", "controller-left",
     * or "controller-right". The left and right controllers each have 4 analog elements:
     * axis-y, axis-x, trigger, grip. The xbox controller has 8 analog elements:
     * left-axis-y, left-axis-x, right-axis-y, right-axis-x, left-trigger, right-trigger,
     * left-grip, right-grip.
     *
     * For \a QVR_Device_Analogs_OpenVR, the parameter string must be either "controller-0"
     * or "controller-1". There will be 3 analogs: axis-y, axis-x, trigger.
     *
     * For \a QVR_Device_Analogs_GoogleVR, the parameter string must currently be "daydream".
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
    // Whether windows on this process sync to vblank
    bool _syncToVBlank;
    // Whether the rendering of this slave process is decoupled from the master process
    bool _decoupledRendering;
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
    /*! \brief Returns whether windows of this process are synchronized with the vertical refresh of the display. */
    bool syncToVBlank() const { return _syncToVBlank; }
    /*! \brief Returns whether the rendering of this slave process is decoupled from the master process. */
    bool decoupledRendering() const { return _decoupledRendering; }
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

    /*! \brief Flags to enable autodetection of certain types of VR hardware.
     * Note that these flags may be ignored, e.g. if QVR was built without support
     * for a specific kind of hardware. */
    enum AutodetectFlag {
        /*! \brief Autodetect Oculus Rift HMD and controllers */
        AutodetectOculus   = (1 << 0),
        /*! \brief Autodetect OpenVR-supported HMD and controllers (e.g. HTC Vive). */
        AutodetectOpenVR   = (1 << 1),
        /*! \brief Autodetect GoogleVR-supported HMDs and controllers (Cardboard, Daydream). */
        AutodetectGoogleVR = (1 << 2),
        /*! \brief Autodetect Gamepads via the QtGamepad module. */
        AutodetectGamepads = (1 << 3),
        /*! \brief Autodetect all hardware. */
        AutodetectAll = 0xffffff
    };
    Q_DECLARE_FLAGS(Autodetect, AutodetectFlag)

    /*!
     * \brief Create a default configuration.
     * \param preferCustomNavigation   Use custom navigation for the default observer if feasible.
     * \param autodetect        Specify which types of VR hardware to detect automatically.
     *
     * This function will detect a head-mounted display and create a suitable
     * configuration for it.
     * If no special equipment is detected, the function will generate a single
     * observer viewing a single window on the master process.
     *
     * The observer created will a suitable default navigation type. If \a preferCustomNavigation
     * is set, then the navigation type \a QVR_Navigation_Custom will be used if it makes sense.
     *
     * Using the \a autodetect parameter only makes sense for developing / debugging; its default
     * value should be fine.
     */
    void createDefault(bool preferCustomNavigation, Autodetect autodetect = AutodetectAll);

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
