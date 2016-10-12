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

#ifndef QVR_MANAGER_HPP
#define QVR_MANAGER_HPP

/*!
 * \mainpage QVR: A library to build Virtual Reality applications
 *
 * \section Overview
 *
 * QVR provides the base class \a QVRApp for Virtual Reality applications.
 * An application implements the subset of \a QVRApp functions that it needs.
 * See \a QVRApp for function descriptions.
 *
 * To run a QVR application, the main() function creates and initializes a
 * \a QVRManager instances and then calls QApplication::exec() as usual. See
 * \a QVRManager for a minimal code example.
 *
 * The \a QVRManager manages three basic types of objects: observers, processes,
 * and windows:
 * - Observers view the virtual scene. Often there is only one observer.
 *   Observers are configured via \a QVRObserverConfig and implemented as \a QVRObserver.
 * - Processes all run the same application binary. The first process initially
 *   run by the user is the master process. Slave processes are automatically
 *   launched by QVRManager as needed. Each process is connected to one display
 *   which can have multiple screens attached. Processes can run on the same host
 *   or across a network. Often there is only the master process.
 *   Processes are configured via \a QVRProcessConfig and implemented as \a QVRProcess.
 * - Windows belong to processes and appear on the display that their process is
 *   connected to. They can have different positions and sizes on different screens
 *   attached to that display. Each window shows a view for exactly one observer
 *   (typically that observer views multiple windows).
 *   Windows are configured via \a QVRWindowConfig and implemented as \a QVRWindow.
 *
 * All QVR applications support a set of command line options. The most important
 * option lets the user choose a configuration file. See QVRManager::QVRManager().
 * The same application binary can run on different Virtual Reality display setups
 * by specifying different configuration files.
 *
 * To get started, see the main descriptions of \a QVRManager and \a QVRApp, and
 * then look at the source code of an example application to understand how it is
 * implemented.
 *
 * \section conffile Configuration files
 *
 * A configuration file defines the observers, processes, and windows that the
 * \a QVRManager will manage.
 *
 * Each observer, process, and window definition is mapped directly to the
 * \a QVRObserverConfig, \a QVRProcessConfig, or \a QVRWindowConfig classes.
 *
 * A configuration file starts with a list of observers and their properties.
 *
 * After that, the list of processes starts. There is always at least one process:
 * the master process. The master process is connected to the display that Qt initially
 * uses (this can be changed with the -display option or DISPLAY environment
 * variable). Slave processes can connect to different displays. Optionally, a
 * launcher command can be specified, e.g. to run a slave process on a different
 * host using ssh.
 *
 * Each process definition contains a list of windows for that process.
 * Since windows provide views into the virtual world, the geometry of the
 * screen wall represented by a window must be known. This geometry is either
 * given by screen wall center coordinates, or by coordinates for three of its
 * corners.
 *
 * Please see the configuration file examples distributed with QVR to understand
 * how typical configurations look like.
 *
 * Observer definition (see \a QVRObserver and \a QVRObserverConfig):
 * - `observer <id>`<br>
 *   Start a new observer definition with the given unique id.
 * - `navigation <stationary|vrpn|wasdqe|custom> [parameters...]`<br>
 *   Set the navigation type and parameters.
 * - `navigation_position <x> <y> <z>`<br>
 *   Set the initial navigation position.
 * - `navigation_forward <x> <y> <z>`<br>
 *   Set the initial navigation forward (or viewing) direction.
 * - `navigation_up <x> <y> <z>`<br>
 *   Set the initial navigation up direction.
 * - `tracking <stationary|vrpn|oculus|custom> [parameters...]`<br>
 *   Set the tracking type and parameters.
 * - `eye_distance <meters>`<br>
 *   Set the interpupillary distance.
 * - `tracking_position <x> <y> <z>`<br>
 *   Set the initial tracking position.
 * - `tracking_forward <x> <y> <z>`<br>
 *   Set the initial tracking forward (or viewing) direction.
 * - `tracking_up <x> <y> <z>`<br>
 *   Set the initial tracking up direction.
 *
 * Process definition (see \a QVRProcess and \a QVRProcessConfig):
 * - `process <id>`<br>
 *   Start a new process definition with the given unique id.
 * - `display <name>`<br>
 *   Display that this process is connected to.
 * - `launcher <prg-and-args>`<br>
 *   Launcher commando used to start this process.
 *
 * Window definition (see \a QVRWindow and \a QVRWindowConfig):
 * - `window <id>`<br>
 *   Start a new window definition with the given unique id, within the current process definition.
 * - `observer <id>`<br>
 *   Set the observer that this window provides a view for.
 * - `output <center|left|right|stereo> <gl|red_cyan|green_magenta|amber_blue|oculus|plugin>`<br>
 *   Set the output mode. Center, left, and right are monoscopic views, stereo is
 *   a stereoscopic view with one of the builtin types gl, red_cyan, green_magenta,
 *   amber_blue, oculus. Both monoscopic and stereoscopic views can also be handled
 *   by an output plugin. See \a QVROutputPlugin().
 * - `display_screen <screen>`<br>
 *   Select the Qt screen index on the Qt display that this process is connected to.<br>
 * - `fullscreen <true|false>`
 *   Whether to show the window in fullscreen mode.<br>
 * - `position <x> <y>`<br>
 *   Set the window position on the Qt screen in pixels.
 * - `size <w> <h>`<br>
 *   Set the window size on the Qt screen in pixels.
 * - `screen_is_fixed_to_observer <true|false>`<br>
 *   Whether the screen wall represented by this window is fixed to the observer,
 *   like a head-mounted display, or it is fixed in virtual world space.
 * - `screen_is_given_by_center <true|false>`<br>
 *   Whether the screen wall represented by this window is given by its center
 *   or by three of its corners.
 * - `screen_center <x> <y> <z>`<br>
 *   Set screen wall center coordinates.
 * - `screen_wall <blx> <bly> <blz> <brx> <bry> <brz> <tlx> <tly> <tlz>`<br>
 *   Set the screen wall geometry defined by three points: bottom left corner, bottom
 *   right corner, and top left corner.
 * - `render_resolution_factor <factor>`<br>
 *   Set the render resolution factor.
 *
 * \section Implementation
 *
 * The \a QVRManager creates an invisible main OpenGL context for each process. This
 * context is used for all calls to \a QVRApp application functions: the application
 * only has to deal with this one context. The rendering results are directed into a
 * set of textures that cover all the windows of the process.
 *
 * When all window textures have been rendered, they are put on screen by the
 * actual visible windows who all have access to the textures from the main
 * window. The windows themselves render in separate rendering threads. On buffer
 * swap, only these rendering threads block. The main thread fills the waiting
 * time with CPU work such as processing events and calling the \a QVRApp::update()
 * function of the application.
 *
 * The process initially started by the user is the master process. \a QVRManager
 * launch slave processes as required by the configuration file, and it will
 * handle all necessary synchronization and data exchange with these slave
 * processes.
 *
 * \section Notes
 *
 * - Oculus Rift: Currently the DK2 was tested on Linux with SDK 0.5. It should
 *   be configured as a separate X11 screen, and the orientation of that screen
 *   should be fixed with `xrandr -display :0.1 -o left`.
 */

#include <QObject>
#include <QVector3D>

template <typename T> class QList;
class QTimer;
class QOpenGLContext;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
class QElapsedTimer;

#include "config.hpp"

class QVRApp;
class QVRDevice;
class QVRObserver;
class QVRWindow;
class QVRProcess;
class QVRRenderContext;
class QVRServer;
class QVRClient;

#ifdef HAVE_OSVR
# include <osvr/ClientKit/DisplayC.h>
# include <osvr/ClientKit/ContextC.h>
extern OSVR_ClientContext QVROsvrClientContext;
extern OSVR_DisplayConfig QVROsvrDisplayConfig;
void QVRAttemptOSVRInitialization();
#endif

/*!
 * \brief Level of logging of the QVR framework
 *
 * You can use the command line or an environment variable to set
 * the log level:
 * \code{.unparsed}
 * <application> --qvr-log-level=fatal|warning|info|debug|firehose
 * export QVR_LOG_LEVEL=FATAL|WARNING|INFO|DEBUG|FIREHOSE
 * \endcode
 */
typedef enum {
    /*! Print only fatal errors */
    QVR_Log_Level_Fatal = 0,
    /*! Additionally print warnings (default) */
    QVR_Log_Level_Warning = 1,
    /*! Additionally print informational messages */
    QVR_Log_Level_Info = 2,
    /*! Additionally print debugging information */
    QVR_Log_Level_Debug = 3,
    /*! Additionally print verbose per-frame debugging information */
    QVR_Log_Level_Firehose = 4
} QVRLogLevel;

/*!
 * \brief Manager of the QVR application's control flow and its processes and windows.
 *
 * The QVRManager object is typically created right after the QApplication object.
 * Afterwards, the application typically sets its preferred OpenGL context properties,
 * and then initializes the QVRManager object:
 * \code{.cpp}
 * QApplication app(argc, argv);
 * QVRManager manager(argc, argv);
 * // Set OpenGL context properties:
 * QSurfaceFormat format;
 * format.setProfile(QSurfaceFormat::CoreProfile);
 * format.setVersion(3, 3);
 * QSurfaceFormat::setDefaultFormat(format);
 * // Start the QVR application
 * MyQVRApp myqvrapp;
 * if (!manager.init(&myqvrapp))
 *     return 1;
 * return app.exec();
 * \endcode
 */
class QVRManager : public QObject
{
Q_OBJECT

private:
    // Data initialized by the constructor:
    QTimer* _triggerTimer;
    QTimer* _fpsTimer;
    QVRLogLevel _logLevel;
    QString _workingDir;
    int _processIndex;
    bool _syncToVblank;
    unsigned int _fpsMsecs;
    unsigned int _fpsCounter;
    QString _configFilename;
    QString _masterName;
    QStringList _appArgs;
    // Data initialized by init():
    QVRServer* _server; // only on the master process
    QVRClient* _client; // only on a client process
    QVRApp* _app;
    QVRConfig* _config;
    QList<QVRDevice*> _devices;
    QList<QVRObserver*> _observers;
    QList<int> _observerNavigationDevices;
    QList<int> _observerTrackingDevices0;
    QList<int> _observerTrackingDevices1;
    QList<QVRObserver*> _customObservers; // subset of _observers
    QVRWindow* _masterWindow;
    QOpenGLContext* _masterGLContext;
    QList<QVRWindow*> _windows;
    QVRProcess* _thisProcess;
    QList<QVRProcess*> _slaveProcesses;
    float _near, _far;
    bool _wantExit;
    QElapsedTimer* _wandNavigationTimer;    // Wand-based observers: framerate-independent speed
    QVector3D _wandNavigationPos;           // Wand-based observers: position
    float _wandNavigationRotY;              // Wand-based observers: angle around the y axis
    bool _haveWasdqeObservers;    // WASDQE observers: do we have at least one?
    QElapsedTimer* _wasdqeTimer;  // WASDQE observers: framerate-independent speed
    bool _wasdqeIsPressed[6];     // WASDQE observers: keys
    int _wasdqeMouseProcessIndex; // WASDQE observers: process with mouse grab
    int _wasdqeMouseWindowIndex;  // WASDQE observers: window with mouse grab
    bool _wasdqeMouseInitialized; // WASDQE observers: was mouse grab initialized
    QVector3D _wasdqePos;         // WASDQE observers: position
    float _wasdqeHorzAngle;       // WASDQE observers: angle around the y axis
    float _wasdqeVertAngle;       // WASDQE observers: angle around the x axis
    bool _masterLoopFirstRun;

    void processEventQueue();

    void updateDevices();
    void render();
    void waitForBufferSwaps();
    void quit();

private slots:
    void masterLoop();
    void slaveLoop();
    void printFps();

public:
    /*!
     * \brief This constructor creates the manager object.
     *
     * The following command line options are intereted by the QVR manager
     * and removed from \a argc and \a argv:
     * - \-\-qvr-config=\<config.qvr\><br>
     *   Specify a QVR configuration file.
     * - \-\-qvr-timeout=\<msecs\><br>
     *   Set a timeout value in milliseconds for all interprocess communication.
     *   The default is -1, which means to never timeout.
     * - \-\-qvr-log-level=\<level\><br>
     *   See \a QVRLogLevel.
     * - \-\-qvr-sync-to-vblank=<0|1><br>
     *   Disable (0) or enable (1) sync-to-vblank. Default is enable.
     * - \-\-qvr-fps=\<n\><br>
     *   Make QVR report frames per second measurements every n milliseconds.
     */
    QVRManager(int& argc, char* argv[]);

    /*! \brief Destructor. */
    ~QVRManager();

    /*!
     * \brief Initialize the QVR application.
     * \param app       The QVR application.
     * \param preferredNavigationType   Preferred type of navigation for this app.
     * \return  False on failure.
     *
     * This function will create all slave processes and all windows, depending
     * on the QVR configuration, and it will call the initialization functions
     * of \a app.
     *
     * The parameter \a preferredNavigationType chooses an observer property
     * in case no configuration file was specified and QVR creates a default
     * configuration. Note that there is no guarantee that QVR fulfills the
     * request. The default is usually fine.
     */
    bool init(QVRApp* app,
            QVRNavigationType preferredNavigationType = QVR_Navigation_WASDQE);

    /*!
     * \brief Return the QVR manager instance.
     *
     * There can be only one instance of \a QVRManager. This function returns it.
     */
    static QVRManager* instance();

    /*!
     * \brief Return the log level.
     */
    static QVRLogLevel logLevel();

    /*!
     * \brief Return the index of this \a QVRProcess in the active configuration.
     *
     * The process with index 0 is the master process.
     *
     * See \a config().
     */
    static int processIndex();

    /*!
     * \brief Return the active configuration.
     */
    static const QVRConfig& config();

    /*!
     * \brief Return the configuration of the device with index \a deviceIndex.
     *
     * This is a convenience function, you can also get this information from \a config().
     */
    static const QVRDeviceConfig& deviceConfig(int deviceIndex);

    /*!
     * \brief Return the configuration of the observer with index \a observerIndex.
     *
     * This is a convenience function, you can also get this information from \a config().
     */
    static const QVRObserverConfig& observerConfig(int observerIndex);

    /*!
     * \brief Return the configuration of the process with the index \a processIndex.
     *
     * This is a convenience function, you can also get this information from \a config().
     */
    static const QVRProcessConfig& processConfig(int processIndex = processIndex());

    /*!
     * \brief Return the configuration of the window with the index \a windowIndex on
     * the process with index \a processIndex.
     *
     * This is a convenience function, you can also get this information from \a config().
     */
    static const QVRWindowConfig& windowConfig(int processIndex, int windowIndex);

    /*! \cond
     * These functions are only used internally. */
    static void enqueueKeyPressEvent(const QVRRenderContext& c, QKeyEvent* event);
    static void enqueueKeyReleaseEvent(const QVRRenderContext& c, QKeyEvent* event);
    static void enqueueMouseMoveEvent(const QVRRenderContext& c, QMouseEvent* event);
    static void enqueueMousePressEvent(const QVRRenderContext& c, QMouseEvent* event);
    static void enqueueMouseReleaseEvent(const QVRRenderContext& c, QMouseEvent* event);
    static void enqueueMouseDoubleClickEvent(const QVRRenderContext& c, QMouseEvent* event);
    static void enqueueWheelEvent(const QVRRenderContext& c, QWheelEvent* event);
    /*! \endcond */
};

#endif
