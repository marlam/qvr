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

#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMatrix4x4>
#include <QCursor>

#include <qvr/manager.hpp>
#include <qvr/window.hpp>

#include "qvr-osgviewer.hpp"

#include <osgDB/ReadFile>


QVROSGViewer::QVROSGViewer(osg::ref_ptr<osg::Node> model) :
    _wantExit(false),
    _model(model),
    _wasdqeIsPressed { false, false, false, false, false, false },
    _mouseGrabProcessIndex(-1),
    _mouseGrabWindowIndex(-1),
    _mouseGrabInitialized(false),
    _pos(0.0f, 0.0f, 0.0f),
    _horzAngle(0.0f),
    _vertAngle(0.0f)
{
}

void QVROSGViewer::serializeDynamicData(QDataStream& ds) const
{
    ds << _pos << _horzAngle << _vertAngle
        << _mouseGrabProcessIndex << _mouseGrabWindowIndex
        << _mouseGrabInitialized;
}

void QVROSGViewer::deserializeDynamicData(QDataStream& ds)
{
    ds >> _pos >> _horzAngle >> _vertAngle
        >> _mouseGrabProcessIndex >> _mouseGrabWindowIndex
        >> _mouseGrabInitialized;
}

void QVROSGViewer::update(const QList<QVRObserver*>& /* customObservers */)
{
    if (_wasdqeIsPressed[0] || _wasdqeIsPressed[1] || _wasdqeIsPressed[2] || _wasdqeIsPressed[3]) {
        QMatrix4x4 viewerMatrix;
        viewerMatrix.translate(_pos);
        viewerMatrix.rotate(QQuaternion::fromEulerAngles(_vertAngle, _horzAngle, 0.0f));
        viewerMatrix = _wasdViewMatrix * viewerMatrix.inverted();
        QVector3D dir;
        if (_wasdqeIsPressed[0]) {
            dir = -viewerMatrix.row(2).toVector3D();
        } else if (_wasdqeIsPressed[1]) {
            dir = -viewerMatrix.row(0).toVector3D();
        } else if (_wasdqeIsPressed[2]) {
            dir = viewerMatrix.row(2).toVector3D();
        } else if (_wasdqeIsPressed[3]) {
            dir = viewerMatrix.row(0).toVector3D();
        }
        dir.setY(0.0f);
        dir.normalize();
        _pos += dir * 0.04f;
    }
    if (_wasdqeIsPressed[4] || _wasdqeIsPressed[5]) {
        QVector3D dir;
        if (_wasdqeIsPressed[4]) {
            dir = QVector3D(0.0f, +1.0f, 0.0f);
        } else if (_wasdqeIsPressed[5]) {
            dir = QVector3D(0.0f, -1.0f, 0.0f);
        }
        _pos += dir * 0.04f;
    }
}

bool QVROSGViewer::wantExit()
{
    return _wantExit;
}

bool QVROSGViewer::initProcess(QVRProcess* /* p */)
{
    // Qt-based OpenGL function pointers
    initializeOpenGLFunctions();

    // FBO
    glGenFramebuffers(1, &_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    glGenTextures(1, &_fboDepthTex);
    glBindTexture(GL_TEXTURE_2D, _fboDepthTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 1, 1,
            0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _fboDepthTex, 0);

    // OSG
    // Since we always only have to deal with one OpenGL context, we set up
    // just one graphics window with a dummy size, and in render() resize and
    // reuse it as necessary.
    _graphicsWindow = _viewer.setUpViewerAsEmbeddedInWindow(0, 0, 640, 480);
    _viewer.setSceneData(_model.get());
    _viewer.realize();

    return true;
}

void QVROSGViewer::preRenderWindow(QVRWindow* w)
{
    if (!_mouseGrabInitialized) {
        if (_mouseGrabProcessIndex == w->processIndex()
                && _mouseGrabWindowIndex == w->index()) {
            w->setCursor(Qt::BlankCursor);
            QCursor::setPos(w->mapToGlobal(QPoint(w->width() / 2, w->height() / 2)));
        } else {
            w->unsetCursor();
        }
    }
}

void QVROSGViewer::render(QVRWindow* /* w */,
        unsigned int fboTex,
        const float* frustumLrbtnf,
        const QMatrix4x4& viewMatrix)
{
    _mouseGrabInitialized = true; // since preRenderWindow() was executed for all windows

    // Set up framebuffer object to render into
    GLint width, height;
    glBindTexture(GL_TEXTURE_2D, fboTex);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
    glBindTexture(GL_TEXTURE_2D, _fboDepthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height,
            0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fboTex, 0);

    // Set up OSG graphics window
    _graphicsWindow->resized(0, 0, width, height);

    // Set up OSG camera
    _viewer.getCamera()->setProjectionMatrixAsFrustum(
            frustumLrbtnf[0], frustumLrbtnf[1], frustumLrbtnf[2], frustumLrbtnf[3],
            frustumLrbtnf[4], frustumLrbtnf[5]);
    QMatrix4x4 viewerMatrix;
    viewerMatrix.translate(_pos);
    viewerMatrix.rotate(QQuaternion::fromEulerAngles(_vertAngle, _horzAngle, 0.0f));
    viewerMatrix = viewMatrix * viewerMatrix.inverted();
    _viewer.getCamera()->setViewMatrix(osg::Matrix(viewerMatrix.constData()));

    // Render
    _viewer.frame();
}

void QVROSGViewer::keyPressEvent(int /* processIndex */, int /* windowIndex */,
        const QRect& /* windowGeometry */, const QRect& /* screenGeometry */,
        const float* /* frustumLrbtnf */, const QMatrix4x4& viewMatrix,
        QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Escape:
        if (_mouseGrabProcessIndex >= 0) {
            _mouseGrabProcessIndex = -1;
            _mouseGrabWindowIndex = -1;
            _mouseGrabInitialized = false;
        } else {
            _wantExit = true;
        }
        break;
    case Qt::Key_W:
        _wasdqeIsPressed[0] = true;
        _wasdViewMatrix = viewMatrix;
        break;
    case Qt::Key_A:
        _wasdqeIsPressed[1] = true;
        _wasdViewMatrix = viewMatrix;
        break;
    case Qt::Key_S:
        _wasdqeIsPressed[2] = true;
        _wasdViewMatrix = viewMatrix;
        break;
    case Qt::Key_D:
        _wasdqeIsPressed[3] = true;
        _wasdViewMatrix = viewMatrix;
        break;
    case Qt::Key_Q:
        _wasdqeIsPressed[4] = true;
        break;
    case Qt::Key_E:
        _wasdqeIsPressed[5] = true;
        break;
    }
}

void QVROSGViewer::keyReleaseEvent(int /* processIndex */, int /* windowIndex */,
        const QRect& /* windowGeometry */, const QRect& /* screenGeometry */,
        const float* /* frustumLrbtnf */, const QMatrix4x4& /* viewMatrix */,
        QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_W:
        _wasdqeIsPressed[0] = false;
        break;
    case Qt::Key_A:
        _wasdqeIsPressed[1] = false;
        break;
    case Qt::Key_S:
        _wasdqeIsPressed[2] = false;
        break;
    case Qt::Key_D:
        _wasdqeIsPressed[3] = false;
        break;
    case Qt::Key_Q:
        _wasdqeIsPressed[4] = false;
        break;
    case Qt::Key_E:
        _wasdqeIsPressed[5] = false;
        break;
    }
}

void QVROSGViewer::mousePressEvent(int processIndex, int windowIndex,
        const QRect& /* windowGeometry */, const QRect& /* screenGeometry */,
        const float* /* frustumLrbtnf */, const QMatrix4x4& /* viewMatrix */,
        QMouseEvent* /* event */)
{
    _mouseGrabProcessIndex = processIndex;
    _mouseGrabWindowIndex = windowIndex;
    _mouseGrabInitialized = false;
}

void QVROSGViewer::mouseMoveEvent(int processIndex, int windowIndex,
        const QRect& windowGeometry, const QRect& /* screenGeometry */,
        const float* /* frustumLrbtnf */, const QMatrix4x4& /* viewMatrix */,
        QMouseEvent* event)
{
    if (_mouseGrabInitialized
            && _mouseGrabProcessIndex == processIndex
            && _mouseGrabWindowIndex == windowIndex) {
        // Horizontal angle
        float x = event->pos().x();
        float w = windowGeometry.width();
        float xf = x / w * 2.0f - 1.0f;
        _horzAngle = -xf * 180.0f;
        // Vertical angle
        // For HMDs, up/down views are realized via head movements. Additional
        // mouse-based up/down views should be disabled since they lead to
        // sickness fast ;)
        if (QVRManager::windowConfig(processIndex, windowIndex).stereoMode()
                != QVR_Stereo_Oculus) {
            float y = event->pos().y();
            float h = windowGeometry.height();
            float yf = y / h * 2.0f - 1.0f;
            _vertAngle = -yf * 90.0f;
        }
    }
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QVRManager manager(argc, argv);

    /* Load the model file */
    if (argc != 2) {
        std::cerr << argv[0] <<": requires model filename argument." << std::endl;
        return 1;
    }
    osg::ref_ptr<osgDB::ReaderWriter::Options> options = new osgDB::ReaderWriter::Options();
    options->setOptionString("noRotation");
    osg::ref_ptr<osg::Node> model = osgDB::readNodeFile(argv[1], options);
    if (!model) {
        std::cerr << argv[0] <<": no data loaded." << std::endl;
        return 1;
    }

    /* First set the default surface format that all windows will use */
    QSurfaceFormat format;
    format.setProfile(QSurfaceFormat::CompatibilityProfile); // OSG cannot handle core profiles
    format.setVersion(3, 3);
    QSurfaceFormat::setDefaultFormat(format);

    /* Then start QVR with your app */
    QVROSGViewer qvrapp(model);
    if (!manager.init(&qvrapp)) {
        std::cerr << "Cannot initialize QVR manager" << std::endl;
        return 1;
    }

    /* Enter the standard Qt loop */
    return app.exec();
}
