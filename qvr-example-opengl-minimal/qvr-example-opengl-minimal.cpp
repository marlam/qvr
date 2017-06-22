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

#include "qvr-example-opengl-minimal.hpp"


QVRExampleOpenGLMinimal::QVRExampleOpenGLMinimal() :
    _wantExit(false),
    _rotationAngle(0.0f)
{
    _timer.start();
}

bool QVRExampleOpenGLMinimal::initProcess(QVRProcess* /* p */)
{
    // Qt-based OpenGL function pointers
    initializeOpenGLFunctions();

    // Framebuffer object
    glGenFramebuffers(1, &_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    glGenTextures(1, &_fboDepthTex);
    glBindTexture(GL_TEXTURE_2D, _fboDepthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 1, 1,
            0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _fboDepthTex, 0);

    // Vertex array object
    static const GLfloat boxPositions[] = {
        -0.8f, +0.8f, +1.0f,   +0.8f, +0.8f, +1.0f,   +0.8f, -0.8f, +1.0f,   -0.8f, -0.8f, +1.0f, // front
        -0.8f, +0.8f, -1.0f,   +0.8f, +0.8f, -1.0f,   +0.8f, -0.8f, -1.0f,   -0.8f, -0.8f, -1.0f, // back
        +1.0f, +0.8f, -0.8f,   +1.0f, +0.8f, +0.8f,   +1.0f, -0.8f, +0.8f,   +1.0f, -0.8f, -0.8f, // right
        -1.0f, +0.8f, -0.8f,   -1.0f, +0.8f, +0.8f,   -1.0f, -0.8f, +0.8f,   -1.0f, -0.8f, -0.8f, // left
        -0.8f, +1.0f, +0.8f,   +0.8f, +1.0f, +0.8f,   +0.8f, +1.0f, -0.8f,   -0.8f, +1.0f, -0.8f, // top
        -0.8f, -1.0f, +0.8f,   +0.8f, -1.0f, +0.8f,   +0.8f, -1.0f, -0.8f,   -0.8f, -1.0f, -0.8f  // bottom
    };
    static const GLubyte boxColors[] = {
        237,  92, 130,   237,  92, 130,   237,  92, 130,   237,  92, 130, // front
         67, 109, 143,    67, 109, 143,    67, 109, 143,    67, 109, 143, // back
        226, 156,  50,   226, 156,  50,   226, 156,  50,   226, 156,  50, // right
        177, 195,  84,   177, 195,  84,   177, 195,  84,   177, 195,  84, // left
         82, 176, 126,    82, 176, 126,    82, 176, 126,    82, 176, 126, // top
         86, 140, 140,    86, 140, 140,    86, 140, 140,    86, 140, 140  // bottom
    };
    static const GLuint boxIndices[] = {
        0, 3, 1, 1, 3, 2, // front face
        4, 5, 7, 5, 6, 7, // back face
        8, 9, 11, 9, 10, 11, // right face
        12, 15, 13, 13, 15, 14, // left face
        16, 17, 19, 17, 18, 19, // top face
        20, 23, 21, 21, 23, 22, // bottom face
    };
    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);
    GLuint positionBuf;
    glGenBuffers(1, &positionBuf);
    glBindBuffer(GL_ARRAY_BUFFER, positionBuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(boxPositions), boxPositions, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    GLuint colorBuf;
    glGenBuffers(1, &colorBuf);
    glBindBuffer(GL_ARRAY_BUFFER, colorBuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(boxColors), boxColors, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);
    glEnableVertexAttribArray(1);
    GLuint indexBuf;
    glGenBuffers(1, &indexBuf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(boxIndices), boxIndices, GL_STATIC_DRAW);
    _vaoIndices = 36;

    // Shader program
    _prg.addShaderFromSourceFile(QOpenGLShader::Vertex, ":vertex-shader.glsl");
    _prg.addShaderFromSourceFile(QOpenGLShader::Fragment, ":fragment-shader.glsl");
    _prg.link();

    return true;
}

void QVRExampleOpenGLMinimal::render(QVRWindow* /* w */,
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
        // Set up view
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        QMatrix4x4 projectionMatrix = context.frustum(view).toMatrix4x4();
        QMatrix4x4 viewMatrix = context.viewMatrix(view);
        // Set up shader program
        glUseProgram(_prg.programId());
        _prg.setUniformValue("projection_matrix", projectionMatrix);
        glEnable(GL_DEPTH_TEST);
        // Render
        QMatrix4x4 modelMatrix;
        modelMatrix.translate(0.0f, 0.0f, -15.0f);
        modelMatrix.rotate(_rotationAngle, 1.0f, 0.5f, 0.0f);
        QMatrix4x4 modelViewMatrix = viewMatrix * modelMatrix;
        _prg.setUniformValue("modelview_matrix", modelViewMatrix);
        _prg.setUniformValue("normal_matrix", modelViewMatrix.normalMatrix());
        glBindVertexArray(_vao);
        glDrawElements(GL_TRIANGLES, _vaoIndices, GL_UNSIGNED_INT, 0);
    }
}

void QVRExampleOpenGLMinimal::update(const QList<QVRObserver*>&)
{
    float seconds = _timer.elapsed() / 1000.0f;
    _rotationAngle = seconds * 20.0f;
}

bool QVRExampleOpenGLMinimal::wantExit()
{
    return _wantExit;
}

void QVRExampleOpenGLMinimal::serializeDynamicData(QDataStream& ds) const
{
    ds << _rotationAngle;
}

void QVRExampleOpenGLMinimal::deserializeDynamicData(QDataStream& ds)
{
    ds >> _rotationAngle;
}

void QVRExampleOpenGLMinimal::keyPressEvent(const QVRRenderContext& /* context */, QKeyEvent* event)
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
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setVersion(3, 3);
    QSurfaceFormat::setDefaultFormat(format);

    /* Then start QVR with the app */
    QVRExampleOpenGLMinimal qvrapp;
    if (!manager.init(&qvrapp)) {
        qCritical("Cannot initialize QVR manager");
        return 1;
    }

    /* Enter the standard Qt loop */
    return app.exec();
}
