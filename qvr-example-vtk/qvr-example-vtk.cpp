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

#include <QGuiApplication>
#include <QKeyEvent>

#include <qvr/manager.hpp>
#include <qvr/window.hpp>

#include "qvr-example-vtk.hpp"

#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkPerspectiveTransform.h>
#include <vtkMarchingCubes.h>
#include <vtkVoxelModeller.h>
#include <vtkSphereSource.h>
#include <vtkImageData.h>


QVRExampleVTK::QVRExampleVTK() :
    _wantExit(false),
    _vtkRenderer(vtkSmartPointer<vtkRenderer>::New()),
    _vtkRenderWindow(vtkSmartPointer<vtkRenderWindow>::New()),
    _vtkCamera(vtkSmartPointer<vtkExternalOpenGLCamera>::New())
{
}

bool QVRExampleVTK::wantExit()
{
    return _wantExit;
}

bool QVRExampleVTK::initProcess(QVRProcess* /* p */)
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
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _fboDepthTex, 0);

    // VTK: Pipeline. This one is a shortened version of the Marching Cubes example.
    vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
    sphereSource->SetPhiResolution(20);
    sphereSource->SetThetaResolution(20);
    sphereSource->Update();
    double bounds[6];
    sphereSource->GetOutput()->GetBounds(bounds);
    for (int i = 0; i < 6; i += 2) {
        double range = bounds[i + 1] - bounds[i];
        bounds[i] = bounds[i] - 0.1 * range;
        bounds[i + 1] = bounds[i + 1] + 0.1 * range;
    }
    vtkSmartPointer<vtkVoxelModeller> voxelModeller = vtkSmartPointer<vtkVoxelModeller>::New();
    voxelModeller->SetSampleDimensions(50, 50, 50);
    voxelModeller->SetModelBounds(bounds);
    voxelModeller->SetScalarTypeToFloat();
    voxelModeller->SetMaximumDistance(0.1);
    voxelModeller->SetInputConnection(sphereSource->GetOutputPort());
    voxelModeller->Update();
    vtkSmartPointer<vtkImageData> volume = vtkSmartPointer<vtkImageData>::New();
    volume->DeepCopy(voxelModeller->GetOutput());
    vtkSmartPointer<vtkMarchingCubes> surface = vtkSmartPointer<vtkMarchingCubes>::New();
    surface->SetInputData(volume);
    surface->ComputeNormalsOn();
    surface->SetValue(0, 0.5);
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(surface->GetOutputPort());
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    double pos[3] = { 0.0, 1.5, -3.0 };
    actor->SetPosition(pos);

    // VTK: Renderer and render window
    // Since we always only have to deal with one OpenGL context, we set up
    // just one render window, and in render() reuse it as necessary.
    _vtkRenderer->SetActiveCamera(_vtkCamera);
    _vtkRenderer->AddActor(actor);
    _vtkRenderWindow->AddRenderer(_vtkRenderer);
    _vtkRenderWindow->InitializeFromCurrentContext();
    _vtkRenderWindow->SwapBuffersOff();
    _vtkRenderWindow->StereoRenderOff();

    return true;
}

static void qMatrixToVtkMatrix(const QMatrix4x4& qM, double vtkM[16])
{
    for (int i = 0; i < 16; i++)
        vtkM[i] = qM.constData()[i];
}

void QVRExampleVTK::render(QVRWindow* /* w */,
        const QVRRenderContext& context, const unsigned int* textures)
{
    for (int view = 0; view < context.viewCount(); view++) {
        // Get view dimensions
        int width = context.textureSize(view).width();
        int height = context.textureSize(view).height();
        // Set up framebuffer object to render into
        glBindTexture(GL_TEXTURE_2D, _fboDepthTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height,
                0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[view], 0);
        // Set up VTK render window
        _vtkRenderWindow->SetSize(width, height);
        // Set up VTK camera view and projection matrix
        double vtkMatrix[16];
        qMatrixToVtkMatrix(context.frustum(view).toMatrix4x4(), vtkMatrix);
        _vtkCamera->SetProjectionTransformMatrix(vtkMatrix);
        qMatrixToVtkMatrix(context.viewMatrix(view), vtkMatrix);
        _vtkCamera->SetViewTransformMatrix(vtkMatrix);
        // Render
        _vtkRenderWindow->Render();
    }
}

void QVRExampleVTK::keyPressEvent(const QVRRenderContext& /* context */, QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Escape:
        _wantExit = true;
        break;
    }
}

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QVRManager manager(argc, argv);

    /* First set the default surface format that all windows will use */
    QSurfaceFormat format;
    format.setProfile(QSurfaceFormat::CompatibilityProfile); // VTK cannot handle core profiles
    format.setVersion(3, 3);
    QSurfaceFormat::setDefaultFormat(format);

    /* Then start QVR with your app */
    QVRExampleVTK qvrapp;
    if (!manager.init(&qvrapp)) {
        qCritical("Cannot initialize QVR manager");
        return 1;
    }

    /* Enter the standard Qt loop */
    return app.exec();
}
