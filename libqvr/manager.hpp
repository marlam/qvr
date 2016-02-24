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

#include <QObject>
#include <QVector3D>

template <typename T> class QList;
class QTimer;
class QOpenGLContext;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;

class QVRApp;
class QVRObserverConfig;
class QVRProcessConfig;
class QVRWindowConfig;
class QVRConfig;
class QVRObserver;
class QVRWindow;
class QVRProcess;
class QVRRenderContext;

/*!
 * \brief The level of logging of the QVR framework
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
 * \brief This class manages the QVR application's control flow and its processes and windows.
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
    unsigned int _fpsMsecs;
    unsigned int _fpsCounter;
    QString _configFilename;
    QStringList _appArgs;
    // Data initialized by init():
    QVRApp* _app;
    QVRConfig* _config;
    QList<QVRObserver*> _observers;
    QList<QVRObserver*> _customObservers; // subset of _observers
    QVRWindow* _masterWindow;
    QOpenGLContext* _masterGLContext;
    QList<QVRWindow*> _windows;
    QVRProcess* _thisProcess;
    QList<QVRProcess*> _slaveProcesses;
    float _near, _far;
    bool _wantExit;
    bool _haveWasdqeObservers;    // WASDQE observers: do we have at least one?
    bool _wasdqeIsPressed[6];     // WASDQE observers: keys
    int _wasdqeMouseProcessIndex; // WASDQE observers: process with mouse grab
    int _wasdqeMouseWindowIndex;  // WASDQE observers: window with mouse grab
    bool _wasdqeMouseInitialized; // WASDQE observers: was mouse grab initialized
    QVector3D _wasdqePos;         // WASDQE observers: position
    float _wasdqeHorzAngle;       // WASDQE observers: angle around the y axis
    float _wasdqeVertAngle;       // WASDQE observers: angle around the x axis
    bool _haveVrpnObservers;

    void processEventQueue();

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
     * - --qvr-config=\<config.qvr\>
     *   Specify a QVR configuration file.
     * - --qvr-log-level=\<level\>
     *   See \a QVRLogLevel.
     * - --qvr-fps=\<n\>
     *   Make QVR report frames per second measurements every n milliseconds.
     */
    QVRManager(int& argc, char* argv[]);

    /*! \brief Destructor. */
    ~QVRManager();

    /*!
     * \brief Initialize the QVR application.
     * \param app       The QVR application.
     * \return  False on failure.
     *
     * This function will create all slave processes and all windows, depending
     * on the QVR configuration, and it will call the initialization functions
     * of \a app.
     */
    bool init(QVRApp* app);

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
     * See \a config().
     */
    static int processIndex();

    /*!
     * \brief Return the active configuration.
     */
    static const QVRConfig& config();

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
