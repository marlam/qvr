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

#include <cstring>
#include <cmath>

#include <QThread>
#include <QMutex>
#include <QOpenGLShaderProgram>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QQuaternion>
#include <QGuiApplication>
#include <QScreen>
#include <QKeyEvent>
#include <QLibrary>
#include <QFile>
#include <QTextStream>

#include "window.hpp"
#include "manager.hpp"
#include "logging.hpp"
#include "observer.hpp"
#include "internalglobals.hpp"

#ifdef HAVE_OCULUS
# include <OVR_CAPI_GL.h>
#endif

#ifdef ANDROID
# include <QtAndroid>
# include <QAndroidJniObject>
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

#if defined(HAVE_OCULUS) && (OVR_PRODUCT_VERSION < 1)
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
    for (;;) {
        _window->winContext()->makeCurrent(_window);
        // Start rendering
        renderingMutex.lock();
        if (!exitWanted) {
            _window->renderOutput();
        }
        renderingMutex.unlock();
        renderingFinished = true;
        if (exitWanted)
            break;
        // Swap buffers
        swapbuffersMutex.lock();
        if (!exitWanted) {
            if (_window->config().outputMode() == QVR_Output_Oculus) {
#ifdef HAVE_OCULUS
# if (OVR_PRODUCT_VERSION >= 1)
                ovr_CommitTextureSwapChain(QVROculus, QVROculusTextureSwapChainL);
                ovr_CommitTextureSwapChain(QVROculus, QVROculusTextureSwapChainR);
                QVROculusLayer.RenderPose[0] = QVROculusRenderPoses[0];
                QVROculusLayer.RenderPose[1] = QVROculusRenderPoses[1];
                ovrLayerHeader* layers = &QVROculusLayer.Header;
                ovr_SubmitFrame(QVROculus, QVROculusFrameIndex, NULL, &layers, 1);
# else
                ovrHmd_EndFrame(QVROculus, QVROculusRenderPoses,
                        reinterpret_cast<ovrTexture*>(oculusEyeTextures));
# endif
#endif
            } else if (_window->config().outputMode() == QVR_Output_OpenVR) {
#ifdef HAVE_OPENVR
                QVRUpdateOpenVR();
#endif
            } else if (_window->config().outputMode() == QVR_Output_GoogleVR) {
                // no buffer swap wanted (?)
            } else {
                // We check if the window is exposed here because swapBuffers()
                // behaviour on an unexposed window is undefined. There seems to
                // be no way to know for sure when a window is exposed after
                // being shown...
                if (_window->isExposed())
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

// Helper function: read a complete file into a QString (without error checking)
static QString readFile(const char* fileName)
{
    QFile f(fileName);
    f.open(QIODevice::ReadOnly);
    QTextStream in(&f);
    return in.readAll();
}

QVRWindow::QVRWindow(QVRWindow* masterWindow, QVRObserver* observer, int windowIndex) :
    QWindow(),
    _isValid(true),
    _screen(-1),
    _thread(NULL),
    _observer(observer),
    _windowIndex(windowIndex),
    _textures { 0, 0 },
    _textureWidths { -1, -1 },
    _textureHeights { -1, -1 },
    _outputQuadVao(0),
    _outputPrg(NULL),
    _renderContext()
{
    setSurfaceType(OpenGLSurface);
    create();
    _winContext = new QOpenGLContext;
    if (!isMaster()) {
        _winContext->setShareContext(masterWindow->winContext());
    }
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    bool wantDoubleBuffer = true;
    // We do not want double buffering in the following cases:
    // - master context: never renders to screen
    // - Oculus or OpenVR control / mirror window: double-buffering this
    //   would cause libqvr to sync to the window's swap rate instead of
    //   the faster HMD swap rate
    // Note that OpenGL ES does not seem to support single buffering.
    if (QOpenGLContext::openGLModuleType() != QOpenGLContext::LibGLES
            && (isMaster()
                || config().outputMode() == QVR_Output_Oculus
                || config().outputMode() == QVR_Output_OpenVR)) {
        wantDoubleBuffer = false;
    }
    format.setSwapBehavior(wantDoubleBuffer ? QSurfaceFormat::DoubleBuffer : QSurfaceFormat::SingleBuffer);
    bool wantStereo = (!isMaster() && config().outputMode() == QVR_Output_Stereo);
    format.setStereo(wantStereo);
    setFormat(format);
    _winContext->setFormat(format);
    _winContext->create();
    if (!_winContext->isValid()) {
        QVR_FATAL("Cannot get a valid OpenGL context");
        _isValid = false;
        return;
    }
    if (_winContext->isOpenGLES()) {
        QVR_DEBUG("    context is OpenGL ES %d.%d",
                _winContext->format().majorVersion(),
                _winContext->format().minorVersion());
    } else {
        QVR_DEBUG("    context is OpenGL %d.%d %s profile",
                _winContext->format().majorVersion(),
                _winContext->format().minorVersion(),
                _winContext->format().profile() == QSurfaceFormat::CompatibilityProfile ? "compatibility" : "core");
    }
    if (!isMaster() && !QOpenGLContext::areSharing(_winContext, masterWindow->winContext())) {
        QVR_FATAL("Cannot get a sharing OpenGL context");
        _isValid = false;
        return;
    }
    if (wantStereo && !_winContext->format().stereo()) {
        QVR_FATAL("Cannot get a stereo OpenGL context");
        _isValid = false;
        return;
    }
    _winContext->makeCurrent(this);
    _gl = new QOpenGLExtraFunctions(_winContext);
    if (!initGL()) {
        _isValid = false;
        _winContext->doneCurrent();
        return;
    }

    // Disable the close button, since we cannot really properly handle it.
    setFlags(flags() | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    // Set an icon
    setIcon(QIcon(":/libqvr/cg-logo.png"));

    if (!isMaster()) {
        QVR_DEBUG("    creating window %s...", qPrintable(config().id()));
        if (QVRManager::config().processConfigs().size() > 1) {
            setTitle(processConfig().id() + " - " + config().id());
        } else {
            setTitle(config().id());
        }
        setMinimumSize(QSize(64, 64));
        _screen = config().initialDisplayScreen();
        if (_screen < 0)
            _screen = QVRPrimaryScreen;
        QVR_DEBUG("      screen: %d", _screen);
        if (false) {
#if defined(HAVE_OCULUS) && (OVR_PRODUCT_VERSION < 1)
        } else if (config().outputMode() == QVR_Output_Oculus) {
            unsigned int distortionCaps =
                ovrDistortionCap_TimeWarp
                | ovrDistortionCap_Vignette
                | ovrDistortionCap_NoRestore
                | ovrDistortionCap_Overdrive
                | ovrDistortionCap_HqDistortion
                | ovrDistortionCap_LinuxDevFullscreen;
            ovrGLConfig glconf;
            std::memset(&glconf, 0, sizeof(glconf));
            glconf.OGL.Header.API = ovrRenderAPI_OpenGL;
            glconf.OGL.Header.BackBufferSize.w = QVROculus->Resolution.w;
            glconf.OGL.Header.BackBufferSize.h = QVROculus->Resolution.h;
            glconf.OGL.Header.Multisample = 1;
            ovrHmd_ConfigureRendering(QVROculus,
                    reinterpret_cast<ovrRenderAPIConfig*>(&glconf),
                    distortionCaps, QVROculus->DefaultEyeFov,
                    QVROculusEyeRenderDesc);
            ovrHmd_DismissHSWDisplay(QVROculus);
            int oculusScreen = -1;
            for (int i = QVRScreenCount - 1; i >= 0; i--) {
                if (QVRScreenGeometries[i].width() == QVROculus->Resolution.w
                        && QVRScreenGeometries[i].height() == QVROculus->Resolution.h) {
                    oculusScreen = i;
                    break;
                }
            }
            if (oculusScreen < 0)
                oculusScreen = QVRPrimaryScreen;
            QVR_DEBUG("      screen: %d", oculusScreen);
            setScreen(QGuiApplication::screens().at(oculusScreen));
            QRect geom = QVRScreenGeometries[oculusScreen];
            QVR_DEBUG("      geometry: %d %d %dx%d", geom.x(), geom.y(), geom.width(), geom.height());
            setGeometry(geom);
            setCursor(Qt::BlankCursor);
            show(); // Apparently this must be called before showFullScreen()
            showFullScreen();
#endif
        } else {
            setScreen(QGuiApplication::screens().at(_screen));
            QRect screenGeom = QVRScreenGeometries[_screen];
            if (config().initialFullscreen()) {
                QVR_DEBUG("      fullscreen geometry: %d %d %dx%d", screenGeom.x(), screenGeom.y(), screenGeom.width(), screenGeom.height());
                setGeometry(screenGeom);
                setCursor(Qt::BlankCursor);
                show(); // Apparently this must be called before showFullScreen()
                showFullScreen();
            } else {
                if (config().initialPosition().x() >= 0 && config().initialPosition().y() >= 0) {
                    QVR_DEBUG("      position %d,%d size %dx%d",
                            config().initialPosition().x(), config().initialPosition().y(),
                            config().initialSize().width(), config().initialSize().height());
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
        if (config().outputMode() == QVR_Output_GoogleVR) {
#ifdef ANDROID
            QVRGoogleVRResolutionFactor = config().renderResolutionFactor();
            QAndroidJniObject activity = QtAndroid::androidActivity();
            _winContext->doneCurrent();
            masterWindow->winContext()->makeCurrent(this);
            activity.callMethod<void>("setMasterContext");
            masterWindow->winContext()->doneCurrent();
            QVR_DEBUG("    Google VR: set master context");
            QtAndroid::runOnAndroidThreadSync([&activity](){
                    activity = QtAndroid::androidActivity();
                    activity.callMethod<void>("initializeVR");});
            QVR_DEBUG("    Google VR: initialized VR");
#endif
        } else {
            _thread = new QVRWindowThread(this);
            _winContext->doneCurrent();
            _winContext->moveToThread(_thread);
            _thread->renderingMutex.lock();
            _thread->swapbuffersMutex.lock();
            _thread->start();
        }

        _renderContext.setProcessIndex(QVRManager::processIndex());
        _renderContext.setWindowIndex(index());

        QVR_DEBUG("    ... done");
    }
}

QVRWindow::~QVRWindow()
{
    if (_thread) {
        exitGL();
        delete _gl;
        winContext()->deleteLater();
    }
}

void QVRWindow::renderToScreen()
{
    Q_ASSERT(!isMaster());
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
    Q_ASSERT(QOpenGLContext::currentContext() != _winContext);
    Q_ASSERT(config().outputMode() == QVR_Output_GoogleVR || _thread);

    if (config().outputMode() == QVR_Output_GoogleVR) {
#ifdef ANDROID
        QVRGoogleVRTextures[0] = _textures[0];
        QVRGoogleVRTextures[1] = _textures[1];
        while (!QVRGoogleVRSync.testAndSetRelaxed(0, 1))
            QThread::yieldCurrentThread();
#endif
    } else {
        _thread->renderingFinished = false;
        _thread->renderingMutex.unlock();
        while (!_thread->renderingFinished)
            QThread::yieldCurrentThread();
        _thread->renderingMutex.lock();
    }
}

void QVRWindow::asyncSwapBuffers()
{
    Q_ASSERT(!isMaster());
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
    Q_ASSERT(QOpenGLContext::currentContext() != _winContext);
    Q_ASSERT(config().outputMode() == QVR_Output_GoogleVR || _thread);

    if (config().outputMode() == QVR_Output_GoogleVR) {
        // do nothing
    } else {
        _thread->swapbuffersFinished = false;
        _thread->swapbuffersMutex.unlock();
    }
}

void QVRWindow::waitForSwapBuffers()
{
    Q_ASSERT(!isMaster());
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
    Q_ASSERT(QOpenGLContext::currentContext() != _winContext);
    Q_ASSERT(config().outputMode() == QVR_Output_GoogleVR || _thread);

    if (config().outputMode() == QVR_Output_GoogleVR) {
#ifdef ANDROID
        while (!QVRGoogleVRSync.testAndSetRelaxed(2, 0))
            QThread::usleep(1);
#endif
    } else {
        while (!_thread->swapbuffersFinished)
            QThread::usleep(1);
        _thread->swapbuffersMutex.lock();
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
    return QVRManager::processIndex();
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

bool QVRWindow::initGL()
{
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());

    _winContext->makeCurrent(this);
    _gl->initializeOpenGLFunctions();

    if (!isMaster()) {
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
            _gl->glGenVertexArrays(1, &_outputQuadVao);
            _gl->glBindVertexArray(_outputQuadVao);
            GLuint positionBuffer;
            _gl->glGenBuffers(1, &positionBuffer);
            _gl->glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
            _gl->glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(QVector3D), quadPositions, GL_STATIC_DRAW);
            _gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
            _gl->glEnableVertexAttribArray(0);
            GLuint texcoordBuffer;
            _gl->glGenBuffers(1, &texcoordBuffer);
            _gl->glBindBuffer(GL_ARRAY_BUFFER, texcoordBuffer);
            _gl->glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(QVector2D), quadTexcoords, GL_STATIC_DRAW);
            _gl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
            _gl->glEnableVertexAttribArray(1);
            GLuint indexBuffer;
            _gl->glGenBuffers(1, &indexBuffer);
            _gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
            _gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), quadIndices, GL_STATIC_DRAW);
            _gl->glBindVertexArray(0);
            GLenum e = _gl->glGetError();
            if (e != GL_NO_ERROR) {
                QVR_FATAL("OpenGL error 0x%04X\n", e);
                return false;
            }

            _outputPrg = new QOpenGLShaderProgram(this);
            QString vertexShaderSource = readFile(":/libqvr/output-vs.glsl");
            QString fragmentShaderSource = readFile(":/libqvr/output-fs.glsl");
            if (QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGLES) {
                vertexShaderSource.prepend("#version 300 es\n");
                fragmentShaderSource.prepend("#version 300 es\n");
            } else {
                vertexShaderSource.prepend("#version 330\n");
                fragmentShaderSource.prepend("#version 330\n");
            }
            if (!_outputPrg->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
                QVR_FATAL("Cannot add output vertex shader");
                return false;
            }
            if (!_outputPrg->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)) {
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
            _outputPluginFunc = reinterpret_cast<void (*)(QVRWindow*, const QVRRenderContext&, const unsigned int*)>
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
            _gl->glDeleteTextures(2, _textures);
            _gl->glDeleteVertexArrays(1, &_outputQuadVao);
            delete _outputPrg;
        } else {
            _outputPluginExitFunc(this);
        }
        _winContext->doneCurrent();
    }
}

void QVRWindow::screenWall(QVector3D& cornerBottomLeft, QVector3D& cornerBottomRight, QVector3D& cornerTopLeft)
{
    Q_ASSERT(!isMaster());
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
    Q_ASSERT(QOpenGLContext::currentContext() != _winContext);
    Q_ASSERT(config().outputMode() != QVR_Output_Oculus);
    Q_ASSERT(_screen >= 0);

    if (config().screenIsGivenByCenter()) {
        // Get geometry (in meter) of the screen
        QRect displayGeom = QVRScreenGeometries[_screen];
        float displayWidth = QVRScreenSizes[_screen].width();
        float displayHeight = QVRScreenSizes[_screen].height();
        cornerBottomLeft.setX(-displayWidth / 2.0f);
        cornerBottomLeft.setY(-displayHeight / 2.0f);
        cornerBottomLeft.setZ(0.0f);
        cornerBottomRight.setX(+displayWidth / 2.0f);
        cornerBottomRight.setY(-displayHeight / 2.0f);
        cornerBottomRight.setZ(0.0f);
        cornerTopLeft.setX(-displayWidth / 2.0f);
        cornerTopLeft.setY(+displayHeight / 2.0f);
        cornerTopLeft.setZ(0.0f);
        QVector3D cornerTopRight = cornerBottomRight + (cornerTopLeft - cornerBottomLeft);
        // Apply the window geometry (subset of screen)
        QRect windowGeom = geometry();
        float windowX = static_cast<float>(windowGeom.x() - displayGeom.x()) / displayGeom.width();
        float windowY = 1.0f - static_cast<float>(windowGeom.y() + windowGeom.height() - displayGeom.y()) / displayGeom.height();
        float windowW = static_cast<float>(windowGeom.width()) / displayGeom.width();
        float windowH = static_cast<float>(windowGeom.height()) / displayGeom.height();
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
        QMatrix4x4 o = _observer->trackingMatrix();
        cornerBottomLeft = o * cornerBottomLeft;
        cornerBottomRight = o * cornerBottomRight;
        cornerTopLeft = o * cornerTopLeft;
    }
}

const QVRRenderContext& QVRWindow::computeRenderContext(float n, float f, unsigned int textures[2])
{
    Q_ASSERT(!isMaster());
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
    Q_ASSERT(QOpenGLContext::currentContext() != _winContext);

    /* Compute the render context */

    _renderContext.setWindowGeometry(geometry());
    _renderContext.setScreenGeometry(screen()->geometry());
    _renderContext.setNavigation(_observer->navigationPosition(), _observer->navigationOrientation());
    _renderContext.setOutputConf(config().outputMode());
    QVector3D wallBl, wallBr, wallTl;
    if (config().outputMode() != QVR_Output_Oculus
            && config().outputMode() != QVR_Output_OpenVR
            && config().outputMode() != QVR_Output_GoogleVR) {
        screenWall(wallBl, wallBr, wallTl);
    }
    _renderContext.setScreenWall(wallBl, wallBr, wallTl);
    for (int i = 0; i < _renderContext.viewCount(); i++) {
        QVREye eye = _renderContext.eye(i);
        _renderContext.setTracking(i, _observer->trackingPosition(eye), _observer->trackingOrientation(eye));
        QVector3D viewPos;
        QQuaternion viewRot;
        if (config().outputMode() == QVR_Output_Oculus) {
#ifdef HAVE_OCULUS
            const ovrFovPort& fov = QVROculusEyeRenderDesc[i].Fov;
            _renderContext.setFrustum(i, QVRFrustum(
                        -fov.LeftTan * n,
                        fov.RightTan * n,
                        -fov.DownTan * n,
                        fov.UpTan * n,
                        n, f));
#endif
            viewPos = _renderContext.trackingPosition(i);
            viewRot = _renderContext.trackingOrientation(i);
        } else if (config().outputMode() == QVR_Output_OpenVR) {
            float l = 0.0f, r = 0.0f, b = 0.0f, t = 0.0f;
#ifdef HAVE_OPENVR
            QVROpenVRSystem->GetProjectionRaw(
                    eye == QVR_Eye_Left ? vr::Eye_Left : vr::Eye_Right,
                    &l, &r, &t, &b);
#endif
            QVRFrustum frustum(l, r, t, b, 1.0f, f);
            frustum.adjustNearPlane(n);
            _renderContext.setFrustum(i, frustum);
            viewPos = _renderContext.trackingPosition(i);
            viewRot = _renderContext.trackingOrientation(i);
        } else if (config().outputMode() == QVR_Output_GoogleVR) {
#ifdef ANDROID
            QVRFrustum frustum(QVRGoogleVRlrbt[i][0], QVRGoogleVRlrbt[i][1],
                    QVRGoogleVRlrbt[i][2], QVRGoogleVRlrbt[i][3], 1.0f, f);
            frustum.adjustNearPlane(n);
            _renderContext.setFrustum(i, frustum);
            viewPos = _renderContext.trackingPosition(i);
            viewRot = _renderContext.trackingOrientation(i);
#endif
        } else {
            // Determine the eye position
            QVector3D eyePosition = _observer->trackingPosition(eye);
            // Get the geometry of the screen area relative to the eye
            QVector3D bl = wallBl - eyePosition;
            QVector3D br = wallBr - eyePosition;
            QVector3D tl = wallTl - eyePosition;
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
            float q = n / planeDistance;
            _renderContext.setFrustum(i, QVRFrustum(l * q, r * q, b * q, t * q, n, f));
            // Compute the view matrix
            QVector3D eyeProjection = -QVector3D::dotProduct(-bl, planeNormal) * planeNormal;
            viewPos = eyePosition;
            viewRot = QQuaternion::fromDirection(-eyeProjection, planeUp);
        }
        QMatrix4x4 viewMatrixPure;
        viewMatrixPure.rotate(viewRot.inverted());
        viewMatrixPure.translate(-viewPos);
        _renderContext.setViewMatrixPure(i, viewMatrixPure);
        QMatrix4x4 viewMatrix;
        if (config().screenIsFixedToObserver()) {
            // XXX why is this special case necessary?? the code below should always work!
            viewMatrix.rotate(viewRot.inverted());
            viewMatrix.rotate(_renderContext.navigationOrientation().inverted());
            viewMatrix.translate(-viewPos);
            viewMatrix.translate(-_renderContext.navigationPosition());
        } else {
            viewMatrix.rotate(viewRot.inverted());
            viewMatrix.translate(-viewPos);
            viewMatrix.rotate(_renderContext.navigationOrientation().inverted());
            viewMatrix.translate(-_renderContext.navigationPosition());
        }
        _renderContext.setViewMatrix(i, viewMatrix);
    }

    /* Get the textures that the application needs to render into */

    GLint textureBinding2dBak;
    _gl->glGetIntegerv(GL_TEXTURE_BINDING_2D, &textureBinding2dBak);

#if defined(HAVE_OCULUS) && (OVR_PRODUCT_VERSION >= 1)
    if (config().outputMode() == QVR_Output_Oculus && _textures[0] == 0) {
        ovrHmdDesc hmdDesc = ovr_GetHmdDesc(QVROculus);
        ovrTextureSwapChainDesc tscDesc = {};
        tscDesc.Type = ovrTexture_2D;
        tscDesc.ArraySize = 1;
        tscDesc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
        tscDesc.MipLevels = 1;
        tscDesc.SampleCount = 1;
        tscDesc.StaticImage = ovrFalse;
        ovrSizei texSizeL = ovr_GetFovTextureSize(QVROculus, ovrEye_Left, hmdDesc.DefaultEyeFov[0], 1.0f);
        tscDesc.Width = texSizeL.w * config().renderResolutionFactor();
        tscDesc.Height = texSizeL.h * config().renderResolutionFactor();
        ovrRecti vpL;
        vpL.Pos.x = 0;
        vpL.Pos.y = 0;
        vpL.Size.w = tscDesc.Width;
        vpL.Size.h = tscDesc.Height;
        ovr_CreateTextureSwapChainGL(QVROculus, &tscDesc, &QVROculusTextureSwapChainL);
        ovrSizei texSizeR = ovr_GetFovTextureSize(QVROculus, ovrEye_Right, hmdDesc.DefaultEyeFov[1], 1.0f);
        tscDesc.Width = texSizeR.w * config().renderResolutionFactor();
        tscDesc.Height = texSizeR.h * config().renderResolutionFactor();
        ovrRecti vpR;
        vpR.Pos.x = 0;
        vpR.Pos.y = 0;
        vpR.Size.w = tscDesc.Width;
        vpR.Size.h = tscDesc.Height;
        ovr_CreateTextureSwapChainGL(QVROculus, &tscDesc, &QVROculusTextureSwapChainR);
        ovr_GetTextureSwapChainBufferGL(QVROculus, QVROculusTextureSwapChainL, 0, &(_textures[0]));
        ovr_GetTextureSwapChainBufferGL(QVROculus, QVROculusTextureSwapChainR, 0, &(_textures[1]));
        QVROculusLayer.Header.Type = ovrLayerType_EyeFov;
        QVROculusLayer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;
        QVROculusLayer.ColorTexture[0] = QVROculusTextureSwapChainL;
        QVROculusLayer.ColorTexture[1] = QVROculusTextureSwapChainR;
        QVROculusLayer.Fov[0] = QVROculusEyeRenderDesc[0].Fov;
        QVROculusLayer.Fov[1] = QVROculusEyeRenderDesc[1].Fov;
        QVROculusLayer.Viewport[0] = vpL;
        QVROculusLayer.Viewport[1] = vpR;
        _textureWidths[0] = vpL.Size.w;
        _textureHeights[0] = vpL.Size.h;
        _textureWidths[1] = vpR.Size.w;
        _textureHeights[1] = vpR.Size.h;
    }
#endif
    for (int i = 0; i < _renderContext.viewCount(); i++) {
        if (_textures[i] == 0) {
            _textureWidths[i] = -1;
            _textureHeights[i] = -1;
            _gl->glGenTextures(1, &(_textures[i]));
            _gl->glBindTexture(GL_TEXTURE_2D, _textures[i]);
            bool wantBilinearInterpolation = true;
            if (std::abs(config().renderResolutionFactor() - 1.0f) <= 0.0f
                    && (config().outputMode() == QVR_Output_Center
                        || config().outputMode() == QVR_Output_Left
                        || config().outputMode() == QVR_Output_Right
                        || config().outputMode() == QVR_Output_Stereo
                        || config().outputMode() == QVR_Output_Red_Cyan
                        || config().outputMode() == QVR_Output_Green_Magenta
                        || config().outputMode() == QVR_Output_Amber_Blue
                        || config().outputMode() == QVR_Output_GoogleVR)) {
                wantBilinearInterpolation = false;
            }
            _gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, wantBilinearInterpolation ? GL_LINEAR : GL_NEAREST);
            _gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, wantBilinearInterpolation ? GL_LINEAR : GL_NEAREST);
            _gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            _gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        int w = 0, h = 0;
        if (config().outputMode() == QVR_Output_Oculus) {
#ifdef HAVE_OCULUS
# if (OVR_PRODUCT_VERSION >= 1)
            // we already created the textures before this loop, make sure that we don't do
            // anything inside this loop.
            w = _textureWidths[i];
            h = _textureHeights[i];
# else
            ovrSizei tex_size = ovrHmd_GetFovTextureSize(QVROculus,
                    i == 0 ? ovrEye_Left : ovrEye_Right,
                    QVROculus->DefaultEyeFov[i], 1.0f);
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
# endif
#endif
        } else if (config().outputMode() == QVR_Output_OpenVR) {
#ifdef HAVE_OPENVR
            uint32_t openVrW, openVrH;
            QVROpenVRSystem->GetRecommendedRenderTargetSize(&openVrW, &openVrH);
            w = openVrW * config().renderResolutionFactor();
            h = openVrH * config().renderResolutionFactor();
#endif
        } else if (config().outputMode() == QVR_Output_GoogleVR) {
#ifdef ANDROID
            w = QVRGoogleVRTexSize.width();
            h = QVRGoogleVRTexSize.height();
#endif
        } else {
            w = width() * config().renderResolutionFactor();
            h = height() * config().renderResolutionFactor();
        }
        if (_textureWidths[i] != w || _textureHeights[i] != h) {
            bool wantSRGB = true;
            if (config().outputMode() == QVR_Output_OpenVR) {
                // 2016-11-03: OpenVR cannot seem to handle SRGB textures; neither
                // ColorSpace_Linear nor ColorSpace_Gamma give correct rendering
                // results. So fall back to linear textures.
                wantSRGB = false;
            }
            _gl->glBindTexture(GL_TEXTURE_2D, _textures[i]);
            _gl->glTexImage2D(GL_TEXTURE_2D, 0,
                    wantSRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8,
                    w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            _textureWidths[i] = w;
            _textureHeights[i] = h;
        }
        _renderContext.setTextureSize(i, QSize(_textureWidths[i], _textureHeights[i]));
    }
    if (_renderContext.viewCount() == 1 && _textures[1] != 0) {
        _gl->glDeleteTextures(1, &(_textures[1]));
        _textures[1] = 0;
        _textureWidths[1] = -1;
        _textureHeights[1] = -1;
        _renderContext.setTextureSize(1, QSize(-1, -1));
    }
    textures[0] = _textures[0];
    textures[1] = _textures[1];
#if defined(HAVE_OCULUS) && (OVR_PRODUCT_VERSION >= 1)
    if (config().outputMode() == QVR_Output_Oculus) {
        ovr_GetTextureSwapChainBufferGL(QVROculus, QVROculusTextureSwapChainL, -1, &(textures[0]));
        ovr_GetTextureSwapChainBufferGL(QVROculus, QVROculusTextureSwapChainR, -1, &(textures[1]));
    }
#endif

    _gl->glBindTexture(GL_TEXTURE_2D, textureBinding2dBak);

    return _renderContext;
}

void QVRWindow::renderOutput()
{
    Q_ASSERT(!isMaster());
    Q_ASSERT(config().outputMode() != QVR_Output_GoogleVR);
    Q_ASSERT(QThread::currentThread() == _thread);
    Q_ASSERT(QOpenGLContext::currentContext() == _winContext);

    unsigned int tex0 = _textures[0];
    unsigned int tex1 = _textures[1];
#if defined(HAVE_OCULUS) && (OVR_PRODUCT_VERSION >= 1)
    if (config().outputMode() == QVR_Output_Oculus) {
        ovr_GetTextureSwapChainBufferGL(QVROculus, QVROculusTextureSwapChainL, -1, &tex0);
        ovr_GetTextureSwapChainBufferGL(QVROculus, QVROculusTextureSwapChainR, -1, &tex1);
    }
#endif
    if (!config().outputPlugin().isEmpty()) {
        unsigned int texs[2] = { tex0, tex1 };
        _outputPluginFunc(this, _renderContext, texs);
#if defined(HAVE_OCULUS) && (OVR_PRODUCT_VERSION < 1)
    } else if (config().outputMode() == QVR_Output_Oculus) {
        // do nothing here, the output is done by ovrHmd_EndFrame()
#endif
    } else {
        _gl->glDisable(GL_DEPTH_TEST);
        _gl->glUseProgram(_outputPrg->programId());
        _gl->glActiveTexture(GL_TEXTURE0);
        _gl->glBindTexture(GL_TEXTURE_2D, tex0);
        _gl->glUniform1i(_gl->glGetUniformLocation(_outputPrg->programId(), "tex_l"), 0);
        _gl->glUniform1i(_gl->glGetUniformLocation(_outputPrg->programId(), "tex_r"), 0);
        _gl->glUniform1i(_gl->glGetUniformLocation(_outputPrg->programId(), "output_mode"), config().outputMode());
        _gl->glBindVertexArray(_outputQuadVao);
        if (tex1 != 0) {
            _gl->glActiveTexture(GL_TEXTURE1);
            _gl->glBindTexture(GL_TEXTURE_2D, tex1);
            _gl->glUniform1i(_gl->glGetUniformLocation(_outputPrg->programId(), "tex_r"), 1);
        }
        _gl->glViewport(0, 0, width(), height());
        if (config().outputMode() == QVR_Output_Stereo) {
#ifdef GL_BACK_LEFT
            GLenum buf = GL_BACK_LEFT;
            _gl->glDrawBuffers(1, &buf);
#endif
        }
        _gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        if (config().outputMode() == QVR_Output_Stereo) {
#ifdef GL_BACK_RIGHT
            _gl->glActiveTexture(GL_TEXTURE0);
            _gl->glBindTexture(GL_TEXTURE_2D, tex1);
            GLenum buf = GL_BACK_RIGHT;
            _gl->glDrawBuffers(1, &buf);
            _gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
#endif
        } else if (config().outputMode() == QVR_Output_OpenVR) {
#ifdef HAVE_OPENVR
            vr::Texture_t l = { reinterpret_cast<void*>(tex0), vr::TextureType_OpenGL, vr::ColorSpace_Linear };
            vr::VRCompositor()->Submit(vr::Eye_Left, &l, NULL, vr::Submit_Default);
            vr::Texture_t r = { reinterpret_cast<void*>(tex1), vr::TextureType_OpenGL, vr::ColorSpace_Linear };
            vr::VRCompositor()->Submit(vr::Eye_Right, &r, NULL, vr::Submit_Default);
            _gl->glFlush(); // suggested by a comment in openvr.h
#endif
        }
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
        QVREventQueue->enqueue(QVREvent(QVR_Event_KeyPress, _renderContext, *event));
    }
}

void QVRWindow::keyReleaseEvent(QKeyEvent* event)
{
    QVREventQueue->enqueue(QVREvent(QVR_Event_KeyRelease, _renderContext, *event));
}

void QVRWindow::mouseMoveEvent(QMouseEvent* event)
{
    QVREventQueue->enqueue(QVREvent(QVR_Event_MouseMove, _renderContext, *event));
}

void QVRWindow::mousePressEvent(QMouseEvent* event)
{
    QVREventQueue->enqueue(QVREvent(QVR_Event_MousePress, _renderContext, *event));
}

void QVRWindow::mouseReleaseEvent(QMouseEvent* event)
{
    QVREventQueue->enqueue(QVREvent(QVR_Event_MouseRelease, _renderContext, *event));
}

void QVRWindow::mouseDoubleClickEvent(QMouseEvent* event)
{
    QVREventQueue->enqueue(QVREvent(QVR_Event_MouseDoubleClick, _renderContext, *event));
}

void QVRWindow::wheelEvent(QWheelEvent* event)
{
    QVREventQueue->enqueue(QVREvent(QVR_Event_Wheel, _renderContext, *event));
}
