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

#ifndef QVR_RENDERCONTEXT_HPP
#define QVR_RENDERCONTEXT_HPP

#include <QRect>
#include <QVector3D>
#include <QMatrix4x4>
#include <QQuaternion>

#include "config.hpp"
#include "frustum.hpp"

class QDataStream;

/*!
 * \brief Context for rendering a frame.
 *
 * A render context provides information about the views into the virtual world
 * that are required for one output frame in a given window.
 *
 * This information depends on the \a QVRWindow that the frame is produced for
 * and on the \a QVRObserver that observes this window.
 *
 * The render context is used in various places:
 * - In \a QVRApp::render(), it provides the information necessary for the
 *   application to render a frame.
 * - In the event handling functions of \a QVRApp, it provides information about
 *   the frame displayed in the window that generated the event.
 * - In the output plugin function \a QVROutputPlugin(), the context provides
 *   information useful for a plugin to decide how to process the frame before
 *   displaying it.
 */
class QVRRenderContext
{
private:
    int _processIndex;
    int _windowIndex;
    QRect _windowGeometry;
    QRect _screenGeometry;
    QVector3D _navigationPosition;
    QQuaternion _navigationOrientation;
    QVector3D _screenWall[3];
    QVROutputMode _outputMode;
    int _viewCount;
    QVREye _eye[2];
    QSize _textureSize[2];
    QVector3D _trackingPosition[2];
    QQuaternion _trackingOrientation[2];
    QVRFrustum _frustum[2];
    QMatrix4x4 _viewMatrix[2];
    QMatrix4x4 _viewMatrixPure[2];

    friend QDataStream &operator<<(QDataStream& ds, const QVRRenderContext& rc);
    friend QDataStream &operator>>(QDataStream& ds, QVRRenderContext& rc);

    // These functions are used internally by QVRWindow when computing the render context information.
    friend class QVRWindow;
    void setProcessIndex(int pi) { _processIndex = pi; }
    void setWindowIndex(int wi) { _windowIndex = wi; }
    void setWindowGeometry(const QRect& r) { _windowGeometry = r; }
    void setScreenGeometry(const QRect& r) { _screenGeometry = r; }
    void setNavigation(const QVector3D& p, const QQuaternion& r) { _navigationPosition = p; _navigationOrientation = r; }
    void setScreenWall(const QVector3D& bl, const QVector3D& br, const QVector3D& tl)
    { _screenWall[0] = bl; _screenWall[1]= br; _screenWall[2] = tl; }
    void setOutputConf(QVROutputMode om);
    void setTextureSize(int vp, const QSize& size) { _textureSize[vp] = size; }
    void setTracking(int vp, const QVector3D& p, const QQuaternion& r) { _trackingPosition[vp] = p; _trackingOrientation[vp] = r; }
    void setFrustum(int vp, const QVRFrustum f) { _frustum[vp] = f; }
    void setViewMatrix(int vp, const QMatrix4x4& vm) { _viewMatrix[vp] = vm; }
    void setViewMatrixPure(int vp, const QMatrix4x4& vmp) { _viewMatrixPure[vp] = vmp; }

public:
    /*! \brief Constructor. */
    QVRRenderContext();

    /*! \brief Returns the index of the process that the window displaying the view belongs to. */
    int processIndex() const { return _processIndex; }
    /*! \brief Returns the index of the window displaying the view, relative to the process it belongs to. */
    int windowIndex() const { return _windowIndex; }
    /*! \brief Returns the pixel-based geometry of window on the Qt display screen (from \a QWindow::geometry()). */
    const QRect& windowGeometry() const { return _windowGeometry; }
    /*! \brief Returns the pixel-based geometry of the Qt screen that the window is displayed on (from \a QScreen::geometry()). */
    const QRect& screenGeometry() const { return _screenGeometry; }
    /*! \brief Returns the observer navigation position. */
    const QVector3D& navigationPosition() const { return _navigationPosition; }
    /*! \brief Returns the observer navigation orientation. */
    const QQuaternion& navigationOrientation() const { return _navigationOrientation; }
    /*! \brief Returns the observer navigation matrix. */
    QMatrix4x4 navigationMatrix() const { QMatrix4x4 m; m.translate(navigationPosition()); m.rotate(navigationOrientation()); return m; }
    /*! \brief Returns the virtual world coordinates of the bottom left corner of the screen wall. */
    QVector3D screenWallBottomLeft() const { return _screenWall[0]; }
    /*! \brief Returns the virtual world coordinates of the bottom right corner of the screen wall. */
    QVector3D screenWallBottomRight() const { return _screenWall[1]; }
    /*! \brief Returns the virtual world coordinates of the top left corner of the screen wall. */
    QVector3D screenWallTopLeft() const { return _screenWall[2]; }
    /*! \brief Returns the output mode of the window displaying the view. */
    QVROutputMode outputMode() const { return _outputMode; }
    /*! \brief Returns the number of views necessary to produce the frame. */
    int viewCount() const { return _viewCount; }
    /*! \brief Returns the eye for rendering \a view. */
    QVREye eye(int view) const { Q_ASSERT(view >= 0 && view < viewCount()); return _eye[view]; }
    /*! \brief Returns the texture size for rendering \a view. */
    QSize textureSize(int view) const { Q_ASSERT(view >= 0 && view < viewCount()); return _textureSize[view]; }
    /*! \brief Returns the observer tracking position for rendering \a view. */
    const QVector3D& trackingPosition(int view) const { Q_ASSERT(view >= 0 && view < viewCount()); return _trackingPosition[view]; }
    /*! \brief Returns the observer tracking orientation for rendering \a view. */
    const QQuaternion& trackingOrientation(int view) const { Q_ASSERT(view >= 0 && view < viewCount()); return _trackingOrientation[view]; }
    /*! \brief Returns the observer tracking matrix for rendering \a view. */
    QMatrix4x4 trackingMatrix(int view) const { Q_ASSERT(view >= 0 && view < viewCount()); QMatrix4x4 m; m.translate(trackingPosition(view)); m.rotate(trackingOrientation(view)); return m; }
    /*! \brief Returns the frustum for rendering \a view. */
    const QVRFrustum& frustum(int view) const { Q_ASSERT(view >= 0 && view < viewCount()); return _frustum[view]; }
    /*! \brief Returns the view matrix for rendering \a view. */
    const QMatrix4x4& viewMatrix(int view) const { Q_ASSERT(view >= 0 && view < viewCount()); return _viewMatrix[view]; }
    /*! \brief Returns the pure view matrix (i.e. in tracking space, without navigation) for rendering \a view. */
    const QMatrix4x4& viewMatrixPure(int view) const { Q_ASSERT(view >= 0 && view < viewCount()); return _viewMatrixPure[view]; }
};

QDataStream &operator<<(QDataStream& ds, const QVRRenderContext& rc);
QDataStream &operator>>(QDataStream& ds, QVRRenderContext& rc);

#endif
