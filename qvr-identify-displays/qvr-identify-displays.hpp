/*
 * Copyright (C) 2018 Computer Graphics Group, University of Siegen
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

#ifndef QVR_IDENTIFY_DISPLAYS_HPP
#define QVR_IDENTIFY_DISPLAYS_HPP

#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>

#include <qvr/app.hpp>

class QVRIdentifyDisplays : public QVRApp, protected QOpenGLExtraFunctions
{
private:
    /* Data not directly relevant for rendering */
    bool _wantExit;             // do we want to exit the app?

    /* Static data for rendering, initialized per process. */
    unsigned int _fbo;          // Framebuffer object to render into
    unsigned int _vao;          // Vertex array object for the quad
    unsigned int _tex;          // The texture that will hold the info image
    QOpenGLShaderProgram _prg;  // Shader program for rendering    

public:
    QVRIdentifyDisplays();

    bool initProcess(QVRProcess* p) override;

    void render(QVRWindow* w, const QVRRenderContext& c, const unsigned int* textures) override;

    bool wantExit() override;

    void keyPressEvent(const QVRRenderContext& context, QKeyEvent* event) override;
};

#endif
