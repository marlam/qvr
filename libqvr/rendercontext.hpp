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

#ifndef QVR_RENDERCONTEXT_HPP
#define QVR_RENDERCONTEXT_HPP

#include <QRect>
#include <QVector3D>
#include <QMatrix4x4>

#include "config.hpp"
#include "frustum.hpp"

class QDataStream;

/*!
 * \brief Context for rendering a view.
 *
 * A render context provides information about a view into the virtual world.
 *
 * This information depends on the \a QVRWindow that the view is produced for
 * and on the \a QVRObserver that observes this window.
 *
 * The render context is used in various places:
 * - In \a QVRApp::render(), it provides the information necessary for the
 *   application to render the view.
 * - In the event handling functions of \a QVRApp, it provides information about
 *   the view displayed in the window that generated the event.
 * - In the output plugin function \a QVROutputPlugin(), the context provides
 *   information useful for a plugin to decide how to process the view before
 *   displaying it.
 */
class QVRRenderContext
{
private:
    int _processIndex;
    int _windowIndex;
    QRect _windowGeometry;
    QRect _screenGeometry;
    QVector3D _screenWall[3];
    QVROutputMode _outputMode;
    int _viewPasses;
    QVREye _eye[2];
    QMatrix4x4 _eyeMatrix[2];
    QVRFrustum _frustum[2];
    QMatrix4x4 _viewMatrix[2];

    friend QDataStream &operator<<(QDataStream& ds, const QVRRenderContext& rc);
    friend QDataStream &operator>>(QDataStream& ds, QVRRenderContext& rc);

public:
    /*! \brief Constructor. */
    QVRRenderContext();

    /*! \brief Returns the index of the process that the window displaying the view belongs to. */
    int processIndex() const { return _processIndex; }
    /*! \brief Returns the index of the window displaying the view, relative to the process it belongs to. */
    int windowIndex() const { return _windowIndex; }
    /*! \brief Returns the pixel-based geometry of window on the Qt display screen (from \a QWindow::geometry()). */
    QRect windowGeometry() const { return _windowGeometry; }
    /*! \brief Returns the pixel-based geometry of the Qt screen that the window is displayed on (from \a QScreen::geometry()). */
    QRect screenGeometry() const { return _screenGeometry; }
    /*! \brief Returns the virtual world coordinates of the bottom left corner of the screen wall. */
    QVector3D screenWallBottomLeft() const { return _screenWall[0]; }
    /*! \brief Returns the virtual world coordinates of the bottom right corner of the screen wall. */
    QVector3D screenWallBottomRight() const { return _screenWall[1]; }
    /*! \brief Returns the virtual world coordinates of the top left corner of the screen wall. */
    QVector3D screenWallTopLeft() const { return _screenWall[2]; }
    /*! \brief Returns the output mode of the window displaying the view. */
    QVROutputMode outputMode() const { return _outputMode; }
    /*! \brief Returns the number of rendering passes necessary to produce the view. */
    int viewPasses() const { return _viewPasses; }
    /*! \brief Returns the eye for rendering pass \a viewPass. */
    QVREye eye(int viewPass) const { return _eye[viewPass]; }
    /*! \brief Returns the eye matrix for rendering pass \a viewPass. */
    const QMatrix4x4& eyeMatrix(int viewPass) const { return _eyeMatrix[viewPass]; }
    /*! \brief Returns the frustum for rendering pass \a viewPass. */
    const QVRFrustum& frustum(int viewPass) const { return _frustum[viewPass]; }
    /*! \brief Returns the view matrix for rendering pass \a viewPass. */
    const QMatrix4x4& viewMatrix(int viewPass) const { return _viewMatrix[viewPass]; }

    /*! \cond
     * These functions are only used internally by QVRWindow when computing the
     * render context information. */
    void setProcessIndex(int pi) { _processIndex = pi; }
    void setWindowIndex(int wi) { _windowIndex = wi; }
    void setWindowGeometry(const QRect& r) { _windowGeometry = r; }
    void setScreenGeometry(const QRect& r) { _screenGeometry = r; }
    void setScreenWall(const QVector3D& bl, const QVector3D& br, const QVector3D& tl)
    { _screenWall[0] = bl; _screenWall[1]= br; _screenWall[2] = tl; }
    void setOutputConf(QVROutputMode om);
    void setEyeMatrix(int vp, const QMatrix4x4& em) { _eyeMatrix[vp] = em; }
    void setFrustum(int vp, const QVRFrustum f) { _frustum[vp] = f; }
    void setViewMatrix(int vp, const QMatrix4x4& vm) { _viewMatrix[vp] = vm; }
    /*! \endcond */
};

QDataStream &operator<<(QDataStream& ds, const QVRRenderContext& rc);
QDataStream &operator>>(QDataStream& ds, QVRRenderContext& rc);

#endif
