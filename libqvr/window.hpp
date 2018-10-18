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

#ifndef QVR_WINDOW_HPP
#define QVR_WINDOW_HPP

#include <QWindow>

#include "config.hpp"
#include "rendercontext.hpp"

class QVRObserver;
class QVRWindowThread;
class QOpenGLShaderProgram;
class QOpenGLContext;
class QOpenGLExtraFunctions;
class QStringList;

/*!
 * \brief A Qt window on a screen and a window into the virtual world
 *
 * From the Qt point of view, a \a QVRWindow is a QWindow with an associated
 * position and size on a Qt display screen.
 *
 * From the Virtual Reality point of view, a \a QVRWindow is a window into
 * the virtual world, and thus has an associated rectangle geometry in the
 * virtual world. This rectangle is called the screen wall.
 *
 * It is important not to confuse the Qt meaning of screen and the VR meaning
 * of screen wall.
 *
 * A window provides a view of the virtual world for a \a QVRObserver. The
 * rendered images displayed by a window thus depend on the virtual world
 * geometry of the screen wall and on the position of the observer.
 *
 * Sometimes the virtual world screen wall is fixed to the observer (e.g. for
 * head-mounted displays), and sometimes it is fixed in the virtual world (e.g.
 * for stationary projection screens).
 *
 * The view may either be monoscopic (for a single eye) or stereoscopic (for
 * both left and right eye), depending on the output mode.
 *
 * An window is configured via \a QVRWindowConfig.
 */
class QVRWindow : public QWindow
{
private:
    bool _isValid;
    int _screen;
    QVRWindowThread* _thread;
    QVRObserver* _observer;
    int _windowIndex;
    unsigned int _textures[2];
    int _textureWidths[2], _textureHeights[2];
    unsigned int _outputQuadVao;
    QOpenGLShaderProgram* _outputPrg;
    bool (*_outputPluginInitFunc)(QVRWindow*, const QStringList&);
    void (*_outputPluginExitFunc)(QVRWindow*);
    void (*_outputPluginFunc)(QVRWindow*, const QVRRenderContext&, const unsigned int*);
    QOpenGLContext* _winContext;
    QOpenGLExtraFunctions* _gl;
    QVRRenderContext _renderContext;

    bool isMaster() const;
    void screenWall(QVector3D& cornerBottomLeft, QVector3D& cornerBottomRight, QVector3D& cornerTopLeft);

    // to be called from _thread:
    void renderOutput();

    // to be called by QVRManager from the main thread:
    bool isValid() const { return _isValid; }
    const QVRRenderContext& computeRenderContext(float n, float f, unsigned int textures[2]);
    void exitGL();
    void renderToScreen();
    void asyncSwapBuffers();
    void waitForSwapBuffers();

    // to be called from _thread and QVRManager:
    QOpenGLContext* winContext() { return _winContext; }

    // to be called from the constructor:
    bool initGL();

    /*! \cond
     * This is internal information. */
    friend class QVRWindowThread;
    friend class QVRManager;
    /*! \endcond */

protected:
    /*! \cond
     * These are only used internally. */
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* event) override;
    /*! \endcond */

public:
    /*! \brief Constructor.
     * @param masterWindow      Pointer to master window
     * @param observer          The observer to provide a view for
     * @param windowIndex       The index of this window in this process
     *
     * The first window has to be created with parameters (0, 0, -1), and it
     * will be the hidden master window. All other windows get a pointer to it
     * on creation time.
     */
    QVRWindow(QVRWindow* masterWindow, QVRObserver* observer, int windowIndex);
    /*! \brief Destructor. */
    virtual ~QVRWindow();

    /*! \brief Returns the index of the window in the process configuration. */
    int index() const;
    /*! \brief Returns the unique id. */
    const QString& id() const;
    /*! \brief Returns the configuration. */
    const QVRWindowConfig& config() const;

    /*! \brief Returns the index of the process that this window belongs to. */
    int processIndex() const;
    /*! \brief Returns the unique id of the process that this window belongs to. */
    const QString& processId() const;
    /*! \brief Returns the configuration of the process that this window belongs to. */
    const QVRProcessConfig& processConfig() const;

    /*! \brief Returns the index of the window observer in the QVR configuration. */
    int observerIndex() const;
    /*! \brief Returns the unique id of the window observer in the QVR configuration. */
    const QString& observerId() const;
    /*! \brief Returns the configuration of the window observer in the QVR configuration. */
    const QVRObserverConfig& observerConfig() const;
};

#endif
