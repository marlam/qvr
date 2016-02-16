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

#ifndef QVR_OSGVIEWER_HPP
#define QVR_OSGVIEWER_HPP

#include <QOpenGLFunctions_3_3_Core>

#include <qvr/app.hpp>

#include <osgViewer/Viewer>


class QVROSGViewer : public QVRApp, protected QOpenGLFunctions_3_3_Core
{
public:
    QVROSGViewer(osg::ref_ptr<osg::Node> model);

private:
    bool _wantExit;
    // OSG objects
    osg::ref_ptr<osg::Node> _model;
    osgViewer::Viewer _viewer;
    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> _graphicsWindow;
    // OpenGL objects
    unsigned int _fbo;
    unsigned int _fboDepthTex;
    // Navigation: mouse-based looking and movement with WASD + QE
    bool _wasdqeIsPressed[6];
    QMatrix4x4 _wasdViewMatrix;
    int _mouseGrabProcessIndex;
    int _mouseGrabWindowIndex;
    bool _mouseGrabInitialized;
    QVector3D _pos;
    float _horzAngle;
    float _vertAngle;

public:
    void serializeDynamicData(QDataStream& ds) const override;
    void deserializeDynamicData(QDataStream& ds) override;

    void update(const QList<QVRObserver*>& customObservers) override;

    bool wantExit() override;

    bool initProcess(QVRProcess* p) override;

    void preRenderWindow(QVRWindow* w) override;

    void render(QVRWindow* w,
            unsigned int fboTex,
            const float* frustumLrbtnf,
            const QMatrix4x4& viewMatrix) override;

    void keyPressEvent(int processIndex, int windowIndex,
            const QRect& windowGeometry, const QRect& screenGeometry,
            const float* frustumLrbtnf, const QMatrix4x4& viewMatrix,
            QKeyEvent* event) override;
    void keyReleaseEvent(int processIndex, int windowIndex,
            const QRect& windowGeometry, const QRect& screenGeometry,
            const float* frustumLrbtnf, const QMatrix4x4& viewMatrix,
            QKeyEvent* event) override;
    void mousePressEvent(int processIndex, int windowIndex,
            const QRect& windowGeometry, const QRect& screenGeometry,
            const float* frustumLrbtnf, const QMatrix4x4& viewMatrix,
            QMouseEvent* event) override;
    void mouseMoveEvent(int processIndex, int windowIndex,
            const QRect& windowGeometry, const QRect& screenGeometry,
            const float* frustumLrbtnf, const QMatrix4x4& viewMatrix,
            QMouseEvent* event) override;
};

#endif
