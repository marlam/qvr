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

#ifndef QVR_CONFIG_HPP
#define QVR_CONFIG_HPP

#include <QString>
#include <QMatrix4x4>
#include <QPoint>
#include <QSize>
#include <QRect>
#include <QList>


/*!
 * \brief Observer type.
 */
typedef enum {
    /*! \brief An observer that never moves. */
    QVR_Observer_Stationary,
    /*! \brief An observer with keyboard and mouse navigation (WASD+QE and lookaround).
     *
     * WASDQE observers navigate via a common keyboard and mouse mapping: the keys
     * WASD are use for forward, sideways, and backward movement, and the additional
     * keys QE move up and down. Mouse movements for looking left/right/up/down are
     * activated by clicking in a window and deactivated by pressing ESC.
     */
    QVR_Observer_WASDQE,
    /*! \brief An observer that is attached to a <a href="https://github.com/vrpn/vrpn/wiki">VRPN</a> tracker.
     *
     * VRPN observers are bound to a VRPN tracker that will determine their position
     * and orientation. Use the \a parameters() property to configure the VRPN tracker.
     */
    QVR_Observer_VRPN,
    /*! \brief An observer that wears the Oculus Rift head mounted display. */
    QVR_Observer_Oculus,
    /*! \brief An observer with tracking implemented by QVRApp::updateObservers(). */
    QVR_Observer_Custom
} QVRObserverType;

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
    QVR_Output_Stereo_GL = 3,
    /*! \brief Output a stereoscopic view for red-cyan anaglyph glasses. */
    QVR_Output_Stereo_Red_Cyan = 4,
    /*! \brief Output a stereoscopic view for green-magenta anaglyph glasses. */
    QVR_Output_Stereo_Green_Magenta = 5,
    /*! \brief Output a stereoscopic view for amber-blue anaglyph glasses. */
    QVR_Output_Stereo_Amber_Blue = 6,
    /*! \brief Output a stereoscopic view for the Oculus Rift head-mounted display. */
    QVR_Output_Stereo_Oculus = 7,
    /*! \brief Output a stereoscopic view via an output plugin; see \a QVROutputPlugin(). */
    QVR_Output_Stereo_Custom = 8
} QVROutputMode;

/*!
 * \brief Configuration of a \a QVRObserver.
 */
class QVRObserverConfig
{
private:
    // Unique identification string
    QString _id;
    // Type of observer. If stationary, the transformation matrix will be
    // constant. Otherwise, it will be updated for each frame.
    QVRObserverType _type;
    // Parameters of observer, for observer types that need this.
    QString _parameters;
    // Distance between left and right eye
    float _eyeDistance;
    // Initial position and orientation for the center eye
    QVector3D _initialPosition;
    QVector3D _initialForwardDirection;
    QVector3D _initialUpDirection;

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

    /*! \brief Returns the type. */
    QVRObserverType type() const { return _type; }
    
    /*! \brief Returns parameters (may be empty).
     *
     * Specific observer types may require parameters.
     *
     * For VRPN observers, this is currently a string of the form <code>\<name\> [\<sensor\>]</code>,
     * where <code>\<name\></code> is the VRPN tracker name, e.g. Tracker0\@localhost,
     * and <code>\<sensor\></code> is the sensor number to be used. 
     * The sensor number can be omitted.
     */
    const QString& parameters() const { return _parameters; }

    /*! \brief Returns the eye distance (interpupillary distance). */
    float eyeDistance() const { return _eyeDistance; }

    /*! \brief Returns the initial position. */
    const QVector3D& initialPosition() const { return _initialPosition; }

    /*! \brief Returns the initial forward direction. */
    const QVector3D& initialForwardDirection() const { return _initialForwardDirection; }

    /*! \brief Returns the initial upward direction. */
    const QVector3D& initialUpDirection() const { return _initialUpDirection; }

    /*! \brief Returns the initial \a eye matrix. */
    QMatrix4x4 initialEyeMatrix(QVREye eye) const;
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
    // This may change at runtime if the screen is fixed to the observer.
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
 * Each process works with only one display. The master process (which is the
 * application process that is started first) always talks to the display that
 * Qt initially uses. On Linux/X11, this can be changed via the -display command
 * line option or DISPLAY environment variable. Slave processes are usually
 * configured to use a different display.
 *
 * The idea is that one display (with potentially multiple screens) runs on one GPU.
 */
class QVRProcessConfig
{
private:
    // Unique identification string
    QString _id;
    // The display that this process works on. Only relevant for X11 at this time.
    QString _display;
    // The launcher command, e.g. ssh
    QString _launcher;
    // The windows driven by this process.
    QList<QVRWindowConfig> _windowConfigs;

    friend class QVRConfig;

public:
    /*! \brief Constructor */
    QVRProcessConfig();

    /*! \brief Returns the unique id of this process. */
    const QString& id() const { return _id; }
    /*! \brief Returns the display that this process works with. */
    const QString& display() const { return _display; }
    /*! \brief Returns the launcher command used to launch this process.
     *
     * This is only required when processes need to run on remote computers,
     * but can also be useful for starting processes under e.g. <a href="http://valgrind.org/">valgrind</a>.
     *
     * Example for ssh (passwordless login to remotehost must be set up):
     * `launcher ssh remotehost`
     *
     * Example for ssh if you use libraries in non-standard locations:
     * `launcher ssh remotehost env LD_LIBRARY_PATH=/path/to/libs`
     */
    const QString& launcher() const { return _launcher; }
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
     *
     * This function will detect e.g. an Oculus Rift head mounted display and
     * create a suitable configuration for it.
     *
     * If no special equipment is detected, the function will generate a single
     * observer viewing a single window on the master process.
     */
    void createDefault();

    /*!
     * \brief Read a configuration file.
     * \param filename  The name of the configuration file.
     * \return False on failure.
     */
    bool readFromFile(const QString& filename);

    /*! \brief Returns the list of observer configurations. */
    const QList<QVRObserverConfig>& observerConfigs() const { return _observerConfigs; }
    /*! \brief Returns the list of process configurations. */
    const QList<QVRProcessConfig>& processConfigs() const { return _processConfigs; }
};

#endif
