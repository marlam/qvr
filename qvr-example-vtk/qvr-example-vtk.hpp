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

#ifndef QVR_EXAMPLE_VTK_HPP
#define QVR_EXAMPLE_VTK_HPP

#include <QOpenGLExtraFunctions>

#include <qvr/app.hpp>

#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkExternalOpenGLCamera.h>
#include <vtkSmartPointer.h>


class QVRExampleVTK : public QVRApp, protected QOpenGLExtraFunctions
{
public:
    QVRExampleVTK();

private:
    bool _wantExit;
    // VTK objects
    vtkSmartPointer<vtkRenderer> _vtkRenderer;
    vtkSmartPointer<vtkRenderWindow> _vtkRenderWindow;
    vtkSmartPointer<vtkExternalOpenGLCamera> _vtkCamera;
    // OpenGL objects
    unsigned int _fbo;
    unsigned int _fboDepthTex;

public:
    bool wantExit() override;

    bool initProcess(QVRProcess* p) override;

    void render(QVRWindow* w, const QVRRenderContext& context, const unsigned int* textures) override;

    void keyPressEvent(const QVRRenderContext& /* context */, QKeyEvent* event) override;
};

#endif
