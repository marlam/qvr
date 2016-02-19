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

#ifndef QVR_WINDOW_HPP
#define QVR_WINDOW_HPP

#include <QWindow>
#include <QOpenGLFunctions_3_3_Core>

#include "config.hpp"

class QVRObserver;
class QVRWindowThread;
class QOpenGLShaderProgram;
class QOpenGLContext;
class QStringList;

class QVRWindow : public QWindow, protected QOpenGLFunctions_3_3_Core
{
private:
    bool _isValid;
    QVRWindowThread* _thread;
    QVRObserver* _observer;
    int _processIndex;
    int _windowIndex;
    unsigned int _textures[2];
    unsigned int _outputQuadVao;
    QOpenGLShaderProgram* _outputPrg;
    bool (*_outputPluginInitFunc)(QVRWindow*, const QStringList&);
    void (*_outputPluginExitFunc)(QVRWindow*);
    void (*_outputPluginFunc)(QVRWindow*, unsigned int, const float*, const QMatrix4x4&,
                                          unsigned int, const float*, const QMatrix4x4&);
    QOpenGLContext* _winContext;
    // only for HMDs:
    void* _hmdHandle;
    float _hmdLRBTTan[4][2];
    float _hmdToEyeViewOffset[3][2];
    QMatrix4x4 _hmdInitialObserverMatrix;
    // last frustum / view matrix for each view pass; for events and output plugins:
    float _viewPassFrustum[2][6];
    QMatrix4x4 _viewPassViewMatrix[2];

    bool isMaster() const;
    void screenGeometry(QVector3D& cornerBottomLeft, QVector3D& cornerBottomRight, QVector3D& cornerTopLeft);

    // to be called from _thread:
    void renderOutput();

    // to be called by QVRManager from the main thread:
    bool isValid() const { return _isValid; }
    void getTextures(unsigned int textures[2]);
    QMatrix4x4 getFrustumAndViewMatrix(int viewPass, float near, float far, float frustum[6]);
    void exitGL();
    void renderToScreen();
    void asyncSwapBuffers();
    void waitForSwapBuffers();
    void updateObserver();

    // to be called from _thread and QVRManager:
    QOpenGLContext* winContext() { return _winContext; }

    // to be called from the constructor:
    bool initGL();

    friend class QVRWindowThread;
    friend class QVRManager;

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* event) override;

public:
    QVRWindow(QOpenGLContext* masterContext, QVRObserver* observer,
            int processIndex, int windowIndex);
    virtual ~QVRWindow();

    int index() const;
    const QString& id() const;
    const QVRWindowConfig& config() const;

    int processIndex() const;
    const QString& processId() const;
    const QVRProcessConfig& processConfig() const;

    int observerIndex() const;
    const QString& observerId() const;
    const QVRObserverConfig& observerConfig() const;
};

#endif
