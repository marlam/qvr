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

#include <cstring>

#include <QThread>
#include <QMutex>
#include <QOpenGLShaderProgram>
#include <QOpenGLContext>
#include <QCoreApplication>
#include <QApplication>
#include <QDesktopWidget>
#include <QScreen>
#include <QKeyEvent>
#include <QLibrary>

#ifdef HAVE_OCULUS
# include <OVR.h>
# include <OVR_CAPI_GL.h>
#endif

#include "window.hpp"
#include "manager.hpp"
#include "logging.hpp"
#include "observer.hpp"


#ifdef HAVE_OCULUS
// This is required because otherwise the Oculus SDK messes up standard output of QVR processes.
// Unfortunately the Oculus SDK seems to ignore this from time to time...
static void oculusLogCallback(int level, const char* message)
{
    if (level == ovrLogLevel_Debug) {
        QVR_DEBUG(message);
    } else if (level == ovrLogLevel_Info) {
        QVR_INFO(message);
    } else {
        QVR_WARNING(message);
    }
}
#endif

class QVRWindowThread : public QThread
{
private:
    QVRWindow* _window;

public:
    bool exitWanted;
    QMutex renderingMutex;
    bool renderingFinished;
    QMutex swapbuffersMutex;
    bool swapbuffersFinished;

#ifdef HAVE_OCULUS
    ovrPosef oculusRenderPoses[2];
    ovrGLTexture oculusEyeTextures[2];
#endif

    QVRWindowThread(QVRWindow* window);

protected:
    void run() override;
};

QVRWindowThread::QVRWindowThread(QVRWindow* window) :
    _window(window), exitWanted(false)
{
}

void QVRWindowThread::run()
{
    bool oculus = false;
#ifdef HAVE_OCULUS
    oculus = _window->config().outputMode() == QVR_Output_Stereo_Oculus;
    ovrHmd oculusHmd = reinterpret_cast<ovrHmd>(_window->_hmdHandle);
#endif
    for (;;) {
        _window->winContext()->makeCurrent(_window);
        if (oculus) {
#ifdef HAVE_OCULUS
            ovrHmd_BeginFrame(oculusHmd, 0);
            ovrVector3f hmdToEyeViewOffset[2] = {
                { _window->_hmdToEyeViewOffset[0][0],
                  _window->_hmdToEyeViewOffset[1][0],
                  _window->_hmdToEyeViewOffset[2][0] },
                { _window->_hmdToEyeViewOffset[0][1],
                  _window->_hmdToEyeViewOffset[1][1],
                  _window->_hmdToEyeViewOffset[2][1] },
            };
            ovrHmd_GetEyePoses(oculusHmd, 0, hmdToEyeViewOffset, oculusRenderPoses, NULL);
#endif
        }
        // Start rendering
        renderingMutex.lock();
        if (!exitWanted) {
            if (!oculus) {
                _window->renderOutput();
            }
        }
        renderingMutex.unlock();
        renderingFinished = true;
        if (exitWanted)
            break;
        // Swap buffers
        swapbuffersMutex.lock();
        if (!exitWanted) {
            if (oculus) {
#ifdef HAVE_OCULUS
                ovrHmd_EndFrame(oculusHmd, oculusRenderPoses,
                        reinterpret_cast<ovrTexture*>(oculusEyeTextures));
#endif
            } else {
                _window->winContext()->swapBuffers(_window);
            }
        }
        swapbuffersMutex.unlock();
        swapbuffersFinished = true;
        if (exitWanted)
            break;
    }
    _window->winContext()->doneCurrent();
    _window->winContext()->moveToThread(QCoreApplication::instance()->thread());
}

QVRWindow::QVRWindow(QOpenGLContext* masterContext,
        QVRObserver* observer,
        int processIndex, int windowIndex) :
    QWindow(),
    QOpenGLFunctions_3_3_Core(),
    _isValid(true),
    _thread(NULL),
    _observer(observer),
    _processIndex(processIndex),
    _windowIndex(windowIndex),
    _textures { 0, 0 },
    _outputQuadVao(0),
    _outputPrg(NULL),
    _renderContext(),
    _hmdHandle(NULL)
{
    setSurfaceType(OpenGLSurface);
    create();
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    if (isMaster()) {
        // Create master window, single buffered (does not render to screen)
        format.setSwapBehavior(QSurfaceFormat::SingleBuffer);
        _winContext = masterContext;
    } else {
        format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
        _winContext = new QOpenGLContext;
        _winContext->setShareContext(masterContext);
    }
    setFormat(format);
    _winContext->setFormat(format);
    _winContext->create();
    if (!_winContext->isValid()) {
        QVR_FATAL("Cannot get a valid OpenGL context");
        _isValid = false;
        return;
    }
    if (!isMaster() && !QOpenGLContext::areSharing(_winContext, masterContext)) {
        QVR_FATAL("Cannot get a sharing OpenGL context");
        _isValid = false;
        return;
    }
    if (!initGL()) {
        _isValid = false;
        return;
    }

    // Disable the close button, since we cannot really properly handle it.
    setFlags(flags() | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    // Set an icon
    setIcon(QIcon(":/libqvr/cg-logo.png"));

    if (!isMaster()) {
        QVR_DEBUG("    creating window %s...", qPrintable(config().id()));
        if (QVRManager::config().processConfigs().size() > 0) {
            setTitle(processConfig().id() + " - " + config().id());
        } else {
            setTitle(config().id());
        }
        setMinimumSize(QSize(64, 64));
        if (config().outputMode() == QVR_Output_Stereo_Oculus) {
#ifdef HAVE_OCULUS
            bool ok;
            QVR_DEBUG("    initializing Oculus ...");
            ovrInitParams oculusInitParams;
            std::memset(&oculusInitParams, 0, sizeof(oculusInitParams));
            oculusInitParams.LogCallback = oculusLogCallback;
            int hmds;
            if (!ovr_Initialize(&oculusInitParams) || (hmds = ovrHmd_Detect() <= 0)) {
                QVR_DEBUG("    ... failed");
                QVR_FATAL("Oculus HMD not available");
                _isValid = false;
                return;
            }
            QVR_DEBUG("    ... SDK version is %s", ovr_GetVersionString());
            QVR_DEBUG("    ... number of HMDs: %d", hmds);
            QVR_DEBUG("    ... using HMD 0");
            ovrHmd oculusHmd = ovrHmd_Create(0);
            if (!oculusHmd) {
                QVR_DEBUG("    ... failed");
                QVR_FATAL("Oculus HMD not usable");
                _isValid = false;
                return;
            }
            QVR_DEBUG("    ... product name: %s", oculusHmd->ProductName);
            QVR_DEBUG("    ... resolution: %dx%d", oculusHmd->Resolution.w, oculusHmd->Resolution.h);
            QVR_DEBUG("    ... window pos: %d %d", oculusHmd->WindowsPos.x, oculusHmd->WindowsPos.y);
            QVR_DEBUG("    ... display: %s", oculusHmd->DisplayDeviceName);
            ok = ovrHmd_ConfigureTracking(oculusHmd,
                    ovrTrackingCap_Orientation
                    | ovrTrackingCap_MagYawCorrection
                    | ovrTrackingCap_Position,
                    ovrTrackingCap_Orientation
                    | ovrTrackingCap_Position);
            QVR_DEBUG("    ... tracking initialization: %s", ok ? "success" : "failure");
            ovrSizei texSizeL = ovrHmd_GetFovTextureSize(oculusHmd, ovrEye_Left, oculusHmd->DefaultEyeFov[0], 1.0f);
            ovrSizei texSizeR = ovrHmd_GetFovTextureSize(oculusHmd, ovrEye_Right, oculusHmd->DefaultEyeFov[1], 1.0f);
            QVR_DEBUG("    ... texture size left: %dx%d", texSizeL.w, texSizeL.h);
            QVR_DEBUG("    ... texture size right: %dx%d", texSizeR.w, texSizeR.h);
            QVR_DEBUG("    ... Cap_ExtendDesktop: %d", (oculusHmd->HmdCaps & ovrHmdCap_ExtendDesktop ? 1 : 0));
            QVR_DEBUG("    ... setting up Oculus rendering ...");
            unsigned int distortionCaps =
                ovrDistortionCap_TimeWarp | ovrDistortionCap_Overdrive
                | ovrDistortionCap_HqDistortion | ovrDistortionCap_LinuxDevFullscreen;
            ovrGLConfig glconf;
            std::memset(&glconf, 0, sizeof(glconf));
            glconf.OGL.Header.API = ovrRenderAPI_OpenGL;
            glconf.OGL.Header.BackBufferSize.w = oculusHmd->Resolution.w;
            glconf.OGL.Header.BackBufferSize.h = oculusHmd->Resolution.h;
            glconf.OGL.Header.Multisample = 0;
            _winContext->makeCurrent(this);
            ovrEyeRenderDesc oculusEyeRenderDesc[2];
            ok = ovrHmd_ConfigureRendering(oculusHmd,
                    reinterpret_cast<ovrRenderAPIConfig*>(&glconf),
                    distortionCaps,
                    oculusHmd->DefaultEyeFov,
                    oculusEyeRenderDesc);
            for (int i = 0; i < 2; i++) {
                const ovrFovPort& fov = oculusEyeRenderDesc[i].Fov;
                _hmdLRBTTan[0][i] = fov.LeftTan;
                _hmdLRBTTan[1][i] = fov.RightTan;
                _hmdLRBTTan[2][i] = fov.DownTan;
                _hmdLRBTTan[3][i] = fov.UpTan;
                _hmdToEyeViewOffset[0][i] = oculusEyeRenderDesc[i].HmdToEyeViewOffset.x;
                _hmdToEyeViewOffset[1][i] = oculusEyeRenderDesc[i].HmdToEyeViewOffset.y;
                _hmdToEyeViewOffset[2][i] = oculusEyeRenderDesc[i].HmdToEyeViewOffset.z;
            }
            ovrHmd_DismissHSWDisplay(oculusHmd);
            _winContext->doneCurrent();
            if (!ok) {
                QVR_DEBUG("    ... failed to initialize.");
                QVR_FATAL("Oculus rendering initialization failed");
                _isValid = false;
                return;
            }
            int oculusScreen = -1;
            for (int i = QApplication::desktop()->screenCount() - 1; i >= 0; i--) {
                if (QApplication::desktop()->screenGeometry(i).width() == 1080
                        && QApplication::desktop()->screenGeometry(i).height() == 1920) {
                    oculusScreen = i;
                    break;
                }
            }
            if (oculusScreen >= 0)
                setScreen(QApplication::screens().at(oculusScreen));
            setGeometry(QApplication::desktop()->screenGeometry(oculusScreen));
            setCursor(Qt::BlankCursor);
            show(); // Apparently this must be called before showFullScreen()
            showFullScreen();
            QVR_DEBUG("    ... done");
            _hmdHandle = const_cast<void*>(reinterpret_cast<const void*>(oculusHmd));
            _hmdInitialObserverMatrix.lookAt(
                    _observer->config().initialPosition(),
                    _observer->config().initialPosition() + _observer->config().initialForwardDirection(),
                    _observer->config().initialUpDirection());
            _hmdInitialObserverMatrix = _hmdInitialObserverMatrix.inverted();
#else
            QVR_FATAL("Oculus HMD support not available in this version of QVR");
            _isValid = false;
            return;
#endif
        } else {
            if (config().initialDisplayScreen() >= 0) {
                QVR_DEBUG("      screen: %d", config().initialDisplayScreen());
                setScreen(QApplication::screens().at(config().initialDisplayScreen()));
            }
            if (config().initialFullscreen()) {
                QVR_DEBUG("      fullscreen: %s", config().initialFullscreen() ? "yes" : "no");
                QRect screenGeom = QApplication::desktop()->screenGeometry(config().initialDisplayScreen());
                setGeometry(screenGeom);
                setCursor(Qt::BlankCursor);
                show(); // Apparently this must be called before showFullScreen()
                showFullScreen();
            } else {
                if (config().initialPosition().x() >= 0 && config().initialPosition().y() >= 0) {
                    QVR_DEBUG("      position %d,%d size %dx%d",
                            config().initialPosition().x(), config().initialPosition().y(),
                            config().initialSize().width(), config().initialSize().height());
                    QRect screenGeom = QApplication::desktop()->screenGeometry(config().initialDisplayScreen());
                    setGeometry(
                            config().initialPosition().x() + screenGeom.x(),
                            config().initialPosition().y() + screenGeom.y(),
                            config().initialSize().width(), config().initialSize().height());
                } else {
                    QVR_DEBUG("      size %dx%d", config().initialSize().width(), config().initialSize().height());
                    resize(config().initialSize());
                }
                show();
            }
        }
        raise();

        _thread = new QVRWindowThread(this);
        _winContext->moveToThread(_thread);
        _thread->renderingMutex.lock();
        _thread->swapbuffersMutex.lock();
        _thread->start();

        _renderContext.setProcessIndex(this->processIndex());
        _renderContext.setWindowIndex(index());

        QVR_DEBUG("    ... done");
    }
}

QVRWindow::~QVRWindow()
{
    if (_thread) {
        exitGL();
        winContext()->deleteLater();
     }
#ifdef HAVE_OCULUS
    if (_hmdHandle) {
        ovrHmd_Destroy(reinterpret_cast<ovrHmd>(_hmdHandle));
        ovr_Shutdown();
    }
#endif
}

void QVRWindow::renderToScreen()
{
    Q_ASSERT(!isMaster());
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
    Q_ASSERT(QOpenGLContext::currentContext() != _winContext);

    _thread->renderingFinished = false;
    _thread->renderingMutex.unlock();
    while (!_thread->renderingFinished)
        QThread::yieldCurrentThread();
    _thread->renderingMutex.lock();
}

void QVRWindow::asyncSwapBuffers()
{
    Q_ASSERT(!isMaster());
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
    Q_ASSERT(QOpenGLContext::currentContext() != _winContext);

    _thread->swapbuffersFinished = false;
    _thread->swapbuffersMutex.unlock();
}

void QVRWindow::waitForSwapBuffers()
{
    Q_ASSERT(!isMaster());
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
    Q_ASSERT(QOpenGLContext::currentContext() != _winContext);

    while (!_thread->swapbuffersFinished)
        QThread::usleep(1);
    _thread->swapbuffersMutex.lock();
}

#ifdef HAVE_OCULUS
static QMatrix4x4 poseToMatrix(const ovrPosef& pose)
{
    QQuaternion rot(pose.Orientation.w, pose.Orientation.x, pose.Orientation.y, pose.Orientation.z);
    QVector3D trans(pose.Position.x, pose.Position.y, pose.Position.z);
    QMatrix4x4 mat(rot.toRotationMatrix());
    mat.translate(trans);
    return mat;
}
#endif

void QVRWindow::updateObserver()
{
    if (config().outputMode() == QVR_Output_Stereo_Oculus) {
#ifdef HAVE_OCULUS
        _observer->setEyeMatrices(
                _hmdInitialObserverMatrix * poseToMatrix(_thread->oculusRenderPoses[0]),
                _hmdInitialObserverMatrix * poseToMatrix(_thread->oculusRenderPoses[1]));
#endif
    }
}

bool QVRWindow::isMaster() const
{
    return !_observer;
}

int QVRWindow::index() const
{
    return _windowIndex;
}

const QString& QVRWindow::id() const
{
    return config().id();
}

const QVRWindowConfig& QVRWindow::config() const
{
    return processConfig().windowConfigs().at(index());
}

int QVRWindow::processIndex() const
{
    return _processIndex;
}

const QString& QVRWindow::processId() const
{
    return processConfig().id();
}

const QVRProcessConfig& QVRWindow::processConfig() const
{
    return QVRManager::config().processConfigs().at(processIndex());
}

int QVRWindow::observerIndex() const
{
    return _observer->index();
}

const QString& QVRWindow::observerId() const
{
    return _observer->id();
}

const QVRObserverConfig& QVRWindow::observerConfig() const
{
    return _observer->config();
}

void QVRWindow::screenWall(QVector3D& cornerBottomLeft, QVector3D& cornerBottomRight, QVector3D& cornerTopLeft)
{
    Q_ASSERT(!isMaster());
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
    Q_ASSERT(QOpenGLContext::currentContext() != _winContext);
    Q_ASSERT(config().outputMode() != QVR_Output_Stereo_Oculus);

    if (config().screenIsGivenByCenter()) {
        // Get geometry (in meter) of the screen
        QDesktopWidget* desktop = QApplication::desktop();
        QRect monitorGeom = desktop->screenGeometry(config().initialDisplayScreen());
        float monitorWidth = monitorGeom.width() / desktop->logicalDpiX() * 0.0254f;
        float monitorHeight = monitorGeom.height() / desktop->logicalDpiY() * 0.0254f;
        cornerBottomLeft.setX(-monitorWidth / 2.0f);
        cornerBottomLeft.setY(-monitorHeight / 2.0f);
        cornerBottomLeft.setZ(0.0f);
        cornerBottomRight.setX(+monitorWidth / 2.0f);
        cornerBottomRight.setY(-monitorHeight / 2.0f);
        cornerBottomRight.setZ(0.0f);
        cornerTopLeft.setX(-monitorWidth / 2.0f);
        cornerTopLeft.setY(+monitorHeight / 2.0f);
        cornerTopLeft.setZ(0.0f);
        QVector3D cornerTopRight = cornerBottomRight + (cornerTopLeft - cornerBottomLeft);
        // Apply the window geometry (subset of screen)
        QRect windowGeom = geometry();
        float windowX = static_cast<float>(windowGeom.x() - monitorGeom.x()) / monitorGeom.width();
        float windowY = 1.0f - static_cast<float>(windowGeom.y() + windowGeom.height() - monitorGeom.y()) / monitorGeom.height();
        float windowW = static_cast<float>(windowGeom.width()) / monitorGeom.width();
        float windowH = static_cast<float>(windowGeom.height()) / monitorGeom.height();
        QVector3D l0 = (1.0f - windowX) * cornerBottomLeft + windowX * cornerBottomRight;
        QVector3D l1 = (1.0f - windowX) * cornerTopLeft  + windowX * cornerTopRight;
        QVector3D bl = (1.0f - windowY) * l0 + windowY * l1;
        QVector3D tl = (1.0f - windowY - windowH) * l0 + (windowY + windowH) * l1;
        QVector3D r0 = (1.0f - windowX - windowW) * cornerBottomLeft + (windowX + windowW) * cornerBottomRight;
        QVector3D r1 = (1.0f - windowX - windowW) * cornerTopLeft + (windowX + windowW) * cornerTopRight;
        QVector3D br = (1.0f - windowY) * r0 + windowY * r1;
        cornerBottomLeft = bl;
        cornerBottomRight = br;
        cornerTopLeft = tl;
        // Translate according to screen center position
        cornerBottomLeft += config().screenCenter();
        cornerBottomRight += config().screenCenter();
        cornerTopLeft += config().screenCenter();
    } else {
        cornerBottomLeft = config().screenCornerBottomLeft();
        cornerBottomRight = config().screenCornerBottomRight();
        cornerTopLeft = config().screenCornerTopLeft();
    }
    if (config().screenIsFixedToObserver()) {
        QMatrix4x4 o = _observer->eyeMatrix(QVR_Eye_Center);
        cornerBottomLeft = o * cornerBottomLeft;
        cornerBottomRight = o * cornerBottomRight;
        cornerTopLeft = o * cornerTopLeft;
    }
}

bool QVRWindow::initGL()
{
    if (isMaster())
        return true;

    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());

    _winContext->makeCurrent(this);
    if (!QOpenGLFunctions_3_3_Core::initializeOpenGLFunctions()) {
        QVR_FATAL("Cannot initialize OpenGL functions");
        return false;
    }

    if (config().outputPlugin().isEmpty()) {
        // Initialize our own output code
        static QVector3D quadPositions[] = {
            QVector3D(-1.0f, -1.0f, 0.0f), QVector3D(+1.0f, -1.0f, 0.0f),
            QVector3D(+1.0f, +1.0f, 0.0f), QVector3D(-1.0f, +1.0f, 0.0f)
        };
        static QVector2D quadTexcoords[] = {
            QVector2D(0.0f, 0.0f), QVector2D(1.0f, 0.0f),
            QVector2D(1.0f, 1.0f), QVector2D(0.0f, 1.0f)
        };
        static GLuint quadIndices[] = {
            0, 1, 3, 1, 2, 3
        };
        glGenVertexArrays(1, &_outputQuadVao);
        glBindVertexArray(_outputQuadVao);
        GLuint positionBuffer;
        glGenBuffers(1, &positionBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
        glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(QVector3D), quadPositions, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);
        GLuint texcoordBuffer;
        glGenBuffers(1, &texcoordBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, texcoordBuffer);
        glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(QVector2D), quadTexcoords, GL_STATIC_DRAW);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(1);
        GLuint indexBuffer;
        glGenBuffers(1, &indexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), quadIndices, GL_STATIC_DRAW);
        glBindVertexArray(0);
        GLenum e = glGetError();
        if (e != GL_NO_ERROR) {
            QVR_FATAL("OpenGL error 0x%04X\n", e);
            return false;
        }

        _outputPrg = new QOpenGLShaderProgram(this);
        if (!_outputPrg->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/libqvr/output-vs.glsl")) {
            QVR_FATAL("Cannot add output vertex shader");
            return false;
        }
        if (!_outputPrg->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/libqvr/output-fs.glsl")) {
            QVR_FATAL("Cannot add output fragment shader");
            return false;
        }
        if (!_outputPrg->link()) {
            QVR_FATAL("Cannot link output program");
            return false;
        }
    } else {
        // Initialize output plugin
        QStringList pluginSpec = config().outputPlugin().split(' ', QString::SkipEmptyParts);
        QString pluginPath = pluginSpec.at(0);
        QStringList pluginArgs = pluginSpec.mid(1);
        QLibrary plugin(pluginPath);
        if (!plugin.load()) {
            QVR_FATAL("Cannot load output plugin %s", qPrintable(pluginPath));
            return false;
        }
        _outputPluginInitFunc = reinterpret_cast<bool (*)(QVRWindow*, const QStringList&)>
            (plugin.resolve("QVROutputPluginInit"));
        _outputPluginExitFunc = reinterpret_cast<void (*)(QVRWindow*)>
            (plugin.resolve("QVROutputPluginExit"));
        _outputPluginFunc = reinterpret_cast<void (*)(QVRWindow*, const QVRRenderContext&,
                unsigned int, unsigned int)>
            (plugin.resolve("QVROutputPlugin"));
        if (!_outputPluginInitFunc || !_outputPluginExitFunc || !_outputPluginFunc) {
            QVR_FATAL("Cannot resolve output plugin functions from plugin %s", qPrintable(pluginPath));
            return false;
        }
        if (!_outputPluginInitFunc(this, pluginArgs)) {
            QVR_FATAL("Cannot initialize output plugin %s", qPrintable(pluginPath));
            return false;
        }
    }

    _winContext->doneCurrent();
    return true;
}

void QVRWindow::exitGL()
{
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
    Q_ASSERT(QOpenGLContext::currentContext() != _winContext);

    if (!isMaster() && _thread) {
        if (_thread->isRunning()) {
            _thread->exitWanted = 1;
            _thread->renderingMutex.unlock();
            _thread->swapbuffersMutex.unlock();
            _thread->wait();
        }
        delete _thread;
        _thread = NULL;
        _winContext->makeCurrent(this);
        if (config().outputPlugin().isEmpty()) {
            glDeleteTextures(2, _textures);
            glDeleteVertexArrays(1, &_outputQuadVao);
            delete _outputPrg;
        } else {
            _outputPluginExitFunc(this);
        }
        _winContext->doneCurrent();
    }
}

void QVRWindow::getTextures(unsigned int textures[2])
{
    Q_ASSERT(!isMaster());
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
    Q_ASSERT(QOpenGLContext::currentContext() != _winContext);

    GLint textureBinding2dBak;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &textureBinding2dBak);

    bool wantTwoTextures = false;
    if (config().outputMode() != QVR_Output_Center
            && config().outputMode() != QVR_Output_Left
            && config().outputMode() != QVR_Output_Right) {
        wantTwoTextures = true;
    }
    for (int i = 0; i < (wantTwoTextures ? 2 : 1); i++) {
        int tw = -1, th = -1;
        if (_textures[i] == 0) {
            glGenTextures(1, &(_textures[i]));
            glBindTexture(GL_TEXTURE_2D, _textures[i]);
            if (std::abs(config().renderResolutionFactor() - 1.0f) <= 0.0f) {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            } else {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            }
        } else {
            glBindTexture(GL_TEXTURE_2D, _textures[i]);
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &tw);
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &th);
        }
        int w = 0, h = 0;
        if (config().outputMode() == QVR_Output_Stereo_Oculus) {
#ifdef HAVE_OCULUS
            ovrHmd oculusHmd = reinterpret_cast<ovrHmd>(_hmdHandle);
            ovrSizei tex_size = ovrHmd_GetFovTextureSize(oculusHmd,
                    i == 0 ? ovrEye_Left : ovrEye_Right,
                    oculusHmd->DefaultEyeFov[i], 1.0f);
            w = tex_size.w * config().renderResolutionFactor();
            h = tex_size.h * config().renderResolutionFactor();
            _thread->oculusEyeTextures[i].OGL.Header.API = ovrRenderAPI_OpenGL;
            _thread->oculusEyeTextures[i].OGL.Header.TextureSize.w = w;
            _thread->oculusEyeTextures[i].OGL.Header.TextureSize.h = h;
            _thread->oculusEyeTextures[i].OGL.Header.RenderViewport.Pos.x = 0;
            _thread->oculusEyeTextures[i].OGL.Header.RenderViewport.Pos.y = 0;
            _thread->oculusEyeTextures[i].OGL.Header.RenderViewport.Size.w = w;
            _thread->oculusEyeTextures[i].OGL.Header.RenderViewport.Size.h = h;
            _thread->oculusEyeTextures[i].OGL.TexId = _textures[i];
#endif
        } else {
            w = width() * config().renderResolutionFactor();
            h = height() * config().renderResolutionFactor();
        }
        if (tw != w || th != h) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
                    GL_BGRA, GL_UNSIGNED_BYTE, NULL);
        }
    }
    if (!wantTwoTextures && _textures[1] != 0) {
        glDeleteTextures(1, &(_textures[1]));
        _textures[1] = 0;
    }
    textures[0] = _textures[0];
    textures[1] = _textures[1];

    glBindTexture(GL_TEXTURE_2D, textureBinding2dBak);
}

QMatrix4x4 QVRWindow::getFrustumAndViewMatrix(int viewPass, float near, float far, float frustum[6])
{
    Q_ASSERT(!isMaster());
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
    Q_ASSERT(QOpenGLContext::currentContext() != _winContext);

    if (viewPass == 0) {
        // these render context properties do not change between view passes
        _renderContext.setWindowRect(geometry());
        _renderContext.setScreenRect(screen()->geometry());
        _renderContext.setOutputConf(config().outputMode());
        QVector3D bl, br, tl;
        screenWall(bl, br, tl);
        _renderContext.setScreenWall(bl, br, tl);
    }

    QVREye eye = _renderContext.eye(viewPass);
    _renderContext.setEyeMatrix(viewPass, _observer->eyeMatrix(eye));

    /* Compute frustum and view matrix for this eye */
    if (config().outputMode() == QVR_Output_Stereo_Oculus) {
        _renderContext.setFrustum(viewPass,
                -_hmdLRBTTan[0][viewPass] * near,
                 _hmdLRBTTan[1][viewPass] * near,
                -_hmdLRBTTan[2][viewPass] * near,
                 _hmdLRBTTan[3][viewPass] * near,
                near, far);
        _renderContext.setViewMatrix(viewPass,
                _renderContext.eyeMatrix(viewPass).inverted());
    } else {
        // Determine the eye position
        QVector3D eyePosition = _observer->eyePosition(eye);
        // Get the geometry of the screen area relative to the eye
        QVector3D bl = _renderContext.screenWallBottomLeft();
        QVector3D br = _renderContext.screenWallBottomRight();
        QVector3D tl = _renderContext.screenWallTopLeft();
        bl -= eyePosition;
        br -= eyePosition;
        tl -= eyePosition;
        // Compute the HNF of the screen plane
        QVector3D planeRight = (br - bl).normalized();
        QVector3D planeUp = (tl - bl).normalized();
        QVector3D planeNormal = QVector3D::crossProduct(planeUp, planeRight);
        float planeDistance = QVector3D::dotProduct(planeNormal, bl);
        // Compute the frustum
        float width = (br - bl).length();
        float height = (tl - bl).length();
        float l = -QVector3D::dotProduct(-bl, planeRight);
        float r = width + l;
        float b = -QVector3D::dotProduct(-bl, planeUp);
        float t = height + b;
        float q = near / planeDistance;
        _renderContext.setFrustum(viewPass, l * q, r * q, b * q, t * q, near, far);
        // Compute the view matrix
        QVector3D eyeProjection = -QVector3D::dotProduct(-bl, planeNormal) * planeNormal;
        QMatrix4x4 viewMatrix;
        viewMatrix.lookAt(eyePosition, eyePosition + eyeProjection, planeUp);
        _renderContext.setViewMatrix(viewPass, viewMatrix);
    }

    for (int i = 0; i < 6; i++)
        frustum[i] = _renderContext.frustumLrbtnf(viewPass)[i];
    return _renderContext.viewMatrix(viewPass);
}

void QVRWindow::renderOutput()
{
    Q_ASSERT(!isMaster());
    Q_ASSERT(QThread::currentThread() == _thread);
    Q_ASSERT(QOpenGLContext::currentContext() == _winContext);

    if (config().outputPlugin().isEmpty()) {
        glViewport(0, 0, width(), height());
        glDisable(GL_DEPTH_TEST);
        glUseProgram(_outputPrg->programId());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _textures[0]);
        glUniform1i(glGetUniformLocation(_outputPrg->programId(), "tex_l"), 0);
        glUniform1i(glGetUniformLocation(_outputPrg->programId(), "tex_r"), 0);
        if (_textures[1] != 0) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, _textures[1]);
            glUniform1i(glGetUniformLocation(_outputPrg->programId(), "tex_r"), 1);
        }
        glUniform1i(glGetUniformLocation(_outputPrg->programId(), "output_mode"), config().outputMode());
        glBindVertexArray(_outputQuadVao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    } else {
        _outputPluginFunc(this, _renderContext, _textures[0], _textures[1]);
    }
}

void QVRWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->matches(QKeySequence::FullScreen)
            || (event->key() == Qt::Key_F
                && (event->modifiers() & Qt::ShiftModifier)
                && (event->modifiers() & Qt::ControlModifier))
            || (event->key() == Qt::Key_F11)) {
        if (windowState() == Qt::WindowFullScreen)
            showNormal();
        else
            showFullScreen();
    } else {
        QVRManager::enqueueKeyPressEvent(_renderContext, event);
    }
}

void QVRWindow::keyReleaseEvent(QKeyEvent* event)
{
    QVRManager::enqueueKeyReleaseEvent(_renderContext, event);
}

void QVRWindow::mouseMoveEvent(QMouseEvent* event)
{
    QVRManager::enqueueMouseMoveEvent(_renderContext, event);
}

void QVRWindow::mousePressEvent(QMouseEvent* event)
{
    QVRManager::enqueueMousePressEvent(_renderContext, event);
}

void QVRWindow::mouseReleaseEvent(QMouseEvent* event)
{
    QVRManager::enqueueMouseReleaseEvent(_renderContext, event);
}

void QVRWindow::mouseDoubleClickEvent(QMouseEvent* event)
{
    QVRManager::enqueueMouseDoubleClickEvent(_renderContext, event);
}

void QVRWindow::wheelEvent(QWheelEvent* event)
{
    QVRManager::enqueueWheelEvent(_renderContext, event);
}
