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

#include <cstdlib>

#include <QtDebug>
#include <QMap>
#include <QOpenGLShaderProgram>
#include <QElapsedTimer>

#include <qvr/outputplugin.hpp>
#include <qvr/window.hpp>

#include "qvr-example-outputplugin.hpp"


QVRExampleOutputPlugin::QVRExampleOutputPlugin(QVRWindow* window) :
    _window(window), _timer(new QElapsedTimer)
{
}

QVRExampleOutputPlugin::~QVRExampleOutputPlugin()
{
    delete _timer;
}

bool QVRExampleOutputPlugin::init(const QStringList& args)
{
    _timer->start();
    _ripple_effect = args.contains("ripple");
    _edge_effect = args.contains("edge");

    if (!QOpenGLFunctions_3_3_Core::initializeOpenGLFunctions()) {
        qCritical() << "Cannot initialize qvr-example-outputplugin: OpenGL functions not available";
        return false;
    }

    static QVector3D quadPositions[] = {
        QVector3D(-1.0f, -1.0f, 0.0f), QVector3D(+1.0f, -1.0f, 0.0f),
        QVector3D(+1.0f, +1.0f, 0.0f), QVector3D(-1.0f, +1.0f, 0.0f)
    };
    static QVector2D quadTexcoords[] = {
        QVector2D(0.0f, 0.0f), QVector2D(1.0f, 0.0f),
        QVector2D(1.0f, 1.0f), QVector2D(0.0f, 1.0f)
    };
    static GLuint quadIndices[] = {
        0, 1, 3, 1, 2, 3
    };
    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);
    GLuint positionBuffer;
    glGenBuffers(1, &positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
    glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(QVector3D), quadPositions, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    GLuint texcoordBuffer;
    glGenBuffers(1, &texcoordBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, texcoordBuffer);
    glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(QVector2D), quadTexcoords, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);
    GLuint indexBuffer;
    glGenBuffers(1, &indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), quadIndices, GL_STATIC_DRAW);
    glBindVertexArray(0);
    if (glGetError() != GL_NO_ERROR) {
        qCritical() << "Cannot initialize qvr-example-outputplugin: OpenGL error";
        return false;
    }
    _prg = new QOpenGLShaderProgram();
    if (!_prg->addShaderFromSourceFile(QOpenGLShader::Vertex,
                ":/qvr-example-outputplugin/vertex-shader.glsl")) {
        qCritical() << "Cannot initialize qvr-example-outputplugin: vertex shader error";
        return false;
    }
    if (!_prg->addShaderFromSourceFile(QOpenGLShader::Fragment,
                ":/qvr-example-outputplugin/fragment-shader.glsl")) {
        qCritical() << "Cannot initialize qvr-example-outputplugin: fragment shader error";
        return false;
    }
    if (!_prg->link()) {
        qCritical() << "Cannot initialize qvr-example-outputplugin: linking error";
        return false;
    }
    return true;
}

void QVRExampleOutputPlugin::exit()
{
    glDeleteVertexArrays(1, &_vao);
    delete _prg;
}

void QVRExampleOutputPlugin::output(const QVRRenderContext& /* context */,
        const unsigned int* textures)
{
    // This toy example plugin only uses the first view regardless of output
    // mode information in the render context.
    glViewport(0, 0, _window->width(), _window->height());
    glUseProgram(_prg->programId());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures[0]);
    glUniform1i(glGetUniformLocation(_prg->programId(), "tex"), 0);
    glUniform1i(glGetUniformLocation(_prg->programId(), "ripple_effect"), _ripple_effect);
    glUniform1f(glGetUniformLocation(_prg->programId(), "ripple_offset"), _timer->elapsed() / 100.0f);
    glUniform1i(glGetUniformLocation(_prg->programId(), "edge_effect"), _edge_effect);
    glBindVertexArray(_vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

/* The plugin interface manages one instance of our plugin per window. */

QMap<QString, QVRExampleOutputPlugin*> instanceMap;
extern int qInitResources_qvr_example_outputplugin();

extern "C" bool QVROutputPluginInit(QVRWindow* window, const QStringList& args)
{
    Q_INIT_RESOURCE(qvr_example_outputplugin);
    instanceMap.insert(window->id(), new QVRExampleOutputPlugin(window));
    return instanceMap[window->id()]->init(args);
}

extern "C" void QVROutputPluginExit(QVRWindow* window)
{
    instanceMap[window->id()]->exit();
    instanceMap.remove(window->id());
}

extern "C" void QVROutputPlugin(QVRWindow* window,
        const QVRRenderContext& context, const unsigned int* textures)
{
    instanceMap[window->id()]->output(context, textures);
}
