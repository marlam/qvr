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

#include "qvr-example-openscenegraph.hpp"

#include <osgDB/ReadFile>


QVRExampleOSG::QVRExampleOSG(osg::ref_ptr<osg::Node> model) :
    _wantExit(false),
    _model(model)
{
}

bool QVRExampleOSG::wantExit()
{
    return _wantExit;
}

bool QVRExampleOSG::initProcess(QVRProcess* /* p */)
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

    // OSG
    // Since we always only have to deal with one OpenGL context, we set up
    // just one graphics window with a dummy size, and in render() resize and
    // reuse it as necessary.
    _graphicsWindow = _viewer.setUpViewerAsEmbeddedInWindow(0, 0, 640, 480);
    _viewer.setSceneData(_model.get());
    _viewer.realize();

    return true;
}

void QVRExampleOSG::render(QVRWindow* /* w */,
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
        // Set up OSG graphics window
        _graphicsWindow->resized(0, 0, width, height);
        // Set up OSG camera
        QMatrix4x4 P = context.frustum(view).toMatrix4x4();
        _viewer.getCamera()->setProjectionMatrix(osg::Matrix(P.constData()));
        QMatrix4x4 V = context.viewMatrix(view);
        _viewer.getCamera()->setViewMatrix(osg::Matrix(V.constData()));
        // Render
        _viewer.frame();
    }
}

void QVRExampleOSG::keyPressEvent(const QVRRenderContext& /* context */, QKeyEvent* event)
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

    /* Load the model file */
    if (argc != 2) {
        qCritical("%s: requires model filename argument.", argv[0]);
        return 1;
    }
    osg::ref_ptr<osgDB::ReaderWriter::Options> options = new osgDB::ReaderWriter::Options();
    options->setOptionString("noRotation");
    osg::ref_ptr<osg::Node> model = osgDB::readNodeFile(argv[1], options);
    if (!model) {
        qCritical("%s: no data loaded.", argv[0]);
        return 1;
    }

    /* First set the default surface format that all windows will use */
    QSurfaceFormat format;
    format.setProfile(QSurfaceFormat::CompatibilityProfile); // OSG cannot handle core profiles
    format.setVersion(3, 3);
    QSurfaceFormat::setDefaultFormat(format);

    /* Then start QVR with your app */
    QVRExampleOSG qvrapp(model);
    if (!manager.init(&qvrapp)) {
        qCritical("Cannot initialize QVR manager");
        return 1;
    }

    /* Enter the standard Qt loop */
    return app.exec();
}
