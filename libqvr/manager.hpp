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
 * uses by default; if a different display is configured, the master process will be
 * relaunched automatically to take this into account. Slave processes typcially connect
 * to different displays. Optionally, a launcher command can be specified, e.g. to run a
 * slave process on a different host using ssh.
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
 * Device definition (see \a QVRDevice and \a QVRDeviceConfig):
 * - `device <id>`<br>
 *   Start a new device definition with the given unique id.
 * - `tracking <none|static|oculus|openvr|vprn>`<br>
 *   Use the specified tracking method for this device.
 * - `buttons <none|static|gamepad|vprn|oculus|openvr>`<br>
 *   Use the specified method to query digital buttons for this device.
 * - `analogs <none|static|gamepad|vrpn|oculus|openvr>`<br>
 *   Use the specified method to query analog joystick elements for this device.
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
 * - `ipc <tcp-socket|local-socket|shared-memory|auto>`<br>
 *   Select the inter-process communication method.
 * - `address <ip-address>`<br>
 *   Set the IP address to bind the server to when using tcp-based inter-process communication.
 * - `launcher <prg-and-args>`<br>
 *   Launcher commando used to start this process.
 * - `display <name>`<br>
 *   Display that this process is connected to.
 * - `sync_to_vblank <true|false>`<br>
 *   Whether windows of this process are synchronized with the vertical refresh of the display.
 * - `decoupled_rendering <true|false>`<br>
 *   Whether the rendering of this slave process is decoupled from the master process.
 *
 * Window definition (see \a QVRWindow and \a QVRWindowConfig):
 * - `window <id>`<br>
 *   Start a new window definition with the given unique id, within the current process definition.
 * - `observer <id>`<br>
 *   Set the observer that this window provides a view for.
 * - `output <center|left|right|stereo|red_cyan|green_magenta|amber_blue|oculus|openvr|googlevr>`<br>
 *   Set the output mode. For center, left, right, and stereo, you can set an additional output plugin.
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
 */

#include <QObject>
#include <QVector3D>
#include <QByteArray>

template <typename T> class QList;
class QTimer;
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
    bool _syncToVBlankWasSet;
    bool _syncToVBlank;
    unsigned int _fpsMsecs;
    unsigned int _fpsCounter;
    QString _configFilename;
    QString _masterName;
    QVRConfig::Autodetect _autodetect;
    QStringList _appArgs;
    bool _isRelaunchedMaster;
    // Data initialized by init():
    QByteArray _serializationBuffer;
    QVRServer* _server; // only on the master process
    QVRClient* _client; // only on a client process
    QVRApp* _app;
    QVRConfig* _config;
    QList<QVRDevice*> _devices;
    QList<QVRDevice> _deviceLastStates;
    QList<QVRObserver*> _observers;
    QList<int> _observerNavigationDevices;
    QList<int> _observerTrackingDevices0;
    QList<int> _observerTrackingDevices1;
    QVRWindow* _masterWindow;
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
    bool _initialized;

    void buildProcessCommandLine(int processIndex, QString* prg, QStringList* args);

    void processEventQueue();

    void updateDevices();
    void render();
    void waitForBufferSwaps();
    void quit();

    friend void QVRMsg(QVRLogLevel level, const char* s);

private slots:
    void masterLoop();
    void slaveLoop();
    void printFps();

public:
    /**
     * \name Constructor, Destructor, Initialization
     */
    /*@{*/

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
     * - \-\-qvr-log-file=\<filename\><br>
     *   Write all log messages to the given file instead of the standard error stream.
     * - \-\-qvr-sync-to-vblank=<0|1><br>
     *   Disable (0) or enable (1) sync-to-vblank. This overrides the per-process setting in the configuration file.
     * - \-\-qvr-fps=\<n\><br>
     *   Make QVR report frames per second measurements every n milliseconds.
     * - \-\-qvr-autodetect=\<list\><br>
     *   Comma-separated list of VR hardware that QVR should attempt to detect automatically.
     *   Currently supported keywords are 'all' for all hardware, 'oculus' for Oculus Rift,
     *   'openvr' for OpenVR hardware, 'googlevr' for Google VR hardware,
     *   'gamepads' for gamepads. Each entry can be preceded with '~' to negate it. For example,
     *   use '\-\-qvr-autodetect=all,~oculus' to try autodetection for all hardware except Oculus Rift.
     *   This option only takes effect if no \-\-qvr-config option was given.
     */
    QVRManager(int& argc, char* argv[]);

    /*! \brief Destructor. */
    ~QVRManager();

    /*!
     * \brief Return the QVR manager instance.
     *
     * There can be only one instance of \a QVRManager. This function returns it.
     */
    static QVRManager* instance();

    /*!
     * \brief Initialize the QVR application.
     * \param app                       The QVR application.
     * \param preferCustomNavigation    Whether the application prefers its own navigation methods.
     * \return  False on failure.
     *
     * This function will create all slave processes and all windows, depending
     * on the QVR configuration, and it will call the initialization functions
     * of \a app.
     *
     * Applications that implement their own navigation methods in \a QVRApp::update() should
     * set the \a preferCustomNavigation flag.
     */
    bool init(QVRApp* app, bool preferCustomNavigation = false);

    /*!
     * \brief Returns whether the manager was successfully initialized.
     */
    static bool isInitialized();

    /*@}*/

    /**
     * \name Configuration access.
     *
     * This is only guaranteed to work after a successfull call to \a init(). See \a isInitialized().
     */
    /*@{*/

    /*!
     * \brief Return the log level.
     */
    static QVRLogLevel logLevel();

    /*!
     * \brief Return the active configuration.
     */
    static const QVRConfig& config();

    /*!
     * \brief Return the number of devices in the configuration.
     *
     * This is a convenience function, you can also get this information from \a config().
     */
    static int deviceCount()
    {
        return config().deviceConfigs().size();
    }

    /*!
     * \brief Return the configuration of the device with index \a deviceIndex.
     *
     * This is a convenience function, you can also get this information from \a config().
     */
    static const QVRDeviceConfig& deviceConfig(int deviceIndex)
    {
        return config().deviceConfigs().at(deviceIndex);
    }

    /*!
     * \brief Return the number of observers in the configuration.
     *
     * This is a convenience function, you can also get this information from \a config().
     */
    static int observerCount()
    {
        return config().observerConfigs().size();
    }

    /*!
     * \brief Return the configuration of the observer with index \a observerIndex.
     *
     * This is a convenience function, you can also get this information from \a config().
     */
    static const QVRObserverConfig& observerConfig(int observerIndex)
    {
        return config().observerConfigs().at(observerIndex);
    }

    /*!
     * \brief Return the number of processes in the configuration.
     *
     * This is a convenience function, you can also get this information from \a config().
     */
    static int processCount()
    {
        return config().processConfigs().size();
    }

    /*!
     * \brief Return the index of the running process. The process with index 0 is the master process.
     */
    static int processIndex();

    /*!
     * \brief Return the configuration of the process with the index \a pi.
     *
     * This is a convenience function, you can also get this information from \a config().
     */
    static const QVRProcessConfig& processConfig(int pi = processIndex())
    {
        return config().processConfigs().at(pi);
    }

    /*!
     * \brief Return the number of windows in the configuration of the process with index \a pi.
     *
     * This is a convenience function, you can also get this information from \a config().
     */
    static int windowCount(int pi = processIndex())
    {
        return config().processConfigs().at(pi).windowConfigs().size();
    }

    /*!
     * \brief Return the configuration of the window with the index \a windowIndex on
     * the process with index \a processIndex.
     *
     * This is a convenience function, you can also get this information from \a config().
     */
    static const QVRWindowConfig& windowConfig(int processIndex, int windowIndex)
    {
        return config().processConfigs().at(processIndex).windowConfigs().at(windowIndex);
    }

    /*@}*/

    /**
     * \name Object access
     */
    /*@{*/

    /*! \brief Return the device with index \a deviceIndex. See \a deviceCount(). */
    static const QVRDevice& device(int deviceIndex);

    /*! \brief Return the observer with index \a observerIndex. See \a observerCount(). */
    static const QVRObserver& observer(int observerIndex);

    /*! \brief Return the process. Only the running process is accessible in this way. See \a processIndex(). */
    static const QVRProcess& process();

    /*! \brief Return the window with the given index in the running process. */
    static const QVRWindow& window(int windowIndex);

    /*@}*/

    /**
     * \name Renderable device models
     *
     * See the documentation of \a QVRDevice for information on how to use this model data to render
     * representations of interaction devices into the virtual world.
     *
     * The data returned by these functions does not change, so you can upload it once to the GPU
     * and reuse it.
     */
    /*@{*/

    /*! \brief Return the number of vertex data blocks. */
    static int deviceModelVertexDataCount();
    /*! \brief Return the number of vertices in vertex data block \a vertexDataIndex. */
    static int deviceModelVertexCount(int vertexDataIndex);
    /*! \brief Return the vertex positions in vertex data block \a vertexDataIndex. Each position consists of three values (x, y, z). */
    static const float* deviceModelVertexPositions(int vertexDataIndex);
    /*! \brief Return the vertex normals in vertex data block \a vertexDataIndex. Each normal consists of three values (nx, ny, nz). */
    static const float* deviceModelVertexNormals(int vertexDataIndex);
    /*! \brief Return the vertex texture coordinates in vertex data block \a vertexDataIndex. Each texture coordinate consists of two values (u, v). */
    static const float* deviceModelVertexTexCoords(int vertexDataIndex);
    /*! \brief Return the number of vertex indices in vertex data block \a vertexDataIndex. */
    static int deviceModelVertexIndexCount(int vertexDataIndex);
    /*! \brief Return the vertex indices in vertex data block \a vertexDataIndex. */
    static const unsigned short* deviceModelVertexIndices(int vertexDataIndex);
    /*! \brief Return the number of textures. */
    static int deviceModelTextureCount();
    /*! \brief Return the texture \a textureIndex. */
    static const QImage& deviceModelTexture(int textureIndex);

    /*@}*/
};

#endif
