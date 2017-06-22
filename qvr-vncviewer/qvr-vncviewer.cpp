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

#include <cmath>
#include <cstring>

#include <QGuiApplication>
#include <QKeyEvent>
#include <QtMath>

#include <qvr/manager.hpp>
#include <qvr/process.hpp>
#include <qvr/config.hpp>

#include "qvr-vncviewer.hpp"


/* Global QVR application instance.
 * Required so that VNC client callbacks can access it. */

static QVRVNCViewer* _qvrApp = NULL;

/* VNC client callbacks */

rfbBool vncResizeCallback(rfbClient* client)
{
    unsigned int* vncFrame = _qvrApp->vncResize(client->width, client->height);
    client->frameBuffer = reinterpret_cast<uint8_t*>(vncFrame);
    client->updateRect.x = 0;
    client->updateRect.y = 0;
    client->updateRect.w = client->width;
    client->updateRect.h = client->height;
    client->format.bitsPerPixel = 32;
    client->format.depth = 8;
    client->format.redMax = 255;
    client->format.greenMax = 255;
    client->format.blueMax = 255;
    client->format.redShift = 16;
    client->format.greenShift = 8;
    client->format.blueShift = 0;
    SetFormatAndEncodings(client);
    return TRUE;
}

void vncUpdateCallback(rfbClient* /* client */, int x, int y, int w, int h)
{
    _qvrApp->vncUpdate(QRect(x, y, w, h));
}

/* Helper functions for command line parsing */

static bool parseWall(const char* wallDef, float* wall)
{
    return std::sscanf(
            wallDef, "%f,%f,%f,%f,%f,%f,%f,%f,%f",
            wall + 0, wall + 1, wall + 2,
            wall + 3, wall + 4, wall + 5,
            wall + 6, wall + 7, wall + 8) == 9;
}

static bool parseCylinder(const char* cylinderDef, float* cylinder)
{
    bool ok = std::sscanf(
            cylinderDef, "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f",
            cylinder + 0, cylinder + 1, cylinder + 2,
            cylinder + 3, cylinder + 4, cylinder + 5,
            cylinder + 6, cylinder + 7, cylinder + 8, cylinder + 9) == 10;
    if (ok) {
        cylinder[7] = qDegreesToRadians(cylinder[7]);
        cylinder[8] = qDegreesToRadians(cylinder[8]);
        cylinder[9] = qDegreesToRadians(cylinder[9]);
    }
    return ok;
}

static void removeArgs(int& argc, char* argv[], int i, int n)
{
    for (int j = i; j < argc - n; j++)
        argv[j] = argv[j + n];
    argc -= n;
}

/* The VNC Viewer application */

QVRVNCViewer::QVRVNCViewer(int& argc, char* argv[]) :
    _wantExit(false),
    _argc(argc),
    _argv(argv),
    _vncClient(NULL),
    _vncWidth(0),
    _vncHeight(0)
{
    _qvrApp = this;
    bool haveScreenDef = false;
    bool haveValidScreenDef = false;
    for (int i = 1; i < _argc; i++) {
        if (strcmp(_argv[i], "--wall") == 0 && i < _argc - 1) {
            haveScreenDef = true;
            haveValidScreenDef = parseWall(_argv[i + 1], _screenWall);
            _screenType = screenTypeWall;
            removeArgs(_argc, _argv, i, 2);
        } else if (strncmp(_argv[i], "--wall=", 7) == 0) {
            haveScreenDef = true;
            haveValidScreenDef = parseWall(_argv[i] + 7, _screenWall);
            _screenType = screenTypeWall;
            removeArgs(_argc, _argv, i, 1);
        } else if (strcmp(_argv[i], "--cylinder") == 0 && i < _argc - 1) {
            haveScreenDef = true;
            haveValidScreenDef = parseCylinder(_argv[i + 1], _screenCylinder);
            _screenType = screenTypeCylinder;
            removeArgs(_argc, _argv, i, 2);
        } else if (strncmp(_argv[i], "--cylinder=", 11) == 0) {
            haveScreenDef = true;
            haveValidScreenDef = parseCylinder(_argv[i] + 11, _screenCylinder);
            _screenType = screenTypeCylinder;
            removeArgs(_argc, _argv, i, 1);
        }
    }
    if (!haveScreenDef)
        qWarning("No screen geometry given; using default wall");
    else if (!haveValidScreenDef)
        qWarning("Screen geometry invalid; falling back to default wall");
    if (!haveScreenDef || !haveValidScreenDef) {
        _screenType = screenTypeWall;
        _screenWall[0] = -1.0f;
        _screenWall[1] = -1.0f + QVRObserverConfig::defaultEyeHeight;
        _screenWall[2] = -3.0f;
        _screenWall[3] = +1.0f;
        _screenWall[4] = -1.0f + QVRObserverConfig::defaultEyeHeight;
        _screenWall[5] = -3.0f;
        _screenWall[6] = -1.0f;
        _screenWall[7] = +1.0f + QVRObserverConfig::defaultEyeHeight;
        _screenWall[8] = -3.0f;
    }
}

unsigned int* QVRVNCViewer::vncResize(int width, int height)
{
    _vncWidth = width;
    _vncHeight = height;
    _vncFrame.resize(_vncWidth * _vncHeight);
    _vncDirtyRectangles.clear();
    _vncDirtyRectangles.append(QRect(0, 0, _vncWidth, _vncHeight));
    return _vncFrame.data();
}

void QVRVNCViewer::vncUpdate(const QRect& r)
{
    _vncDirtyRectangles.append(r);
}

void QVRVNCViewer::createSceneGeometry(QVector<QVector3D>& positions,
        QVector<QVector2D>& texcoords, QVector<unsigned int>& indices)
{
    if (_screenType == screenTypeWall) {
        QVector3D bl(_screenWall[0], _screenWall[1], _screenWall[2]);
        QVector3D br(_screenWall[3], _screenWall[4], _screenWall[5]);
        QVector3D tl(_screenWall[6], _screenWall[7], _screenWall[8]);
        QVector3D tr = br + (tl - bl);
        positions.append(bl);
        positions.append(br);
        positions.append(tr);
        positions.append(tl);
        texcoords.append(QVector2D(0.0f, 1.0f));
        texcoords.append(QVector2D(1.0f, 1.0f));
        texcoords.append(QVector2D(1.0f, 0.0f));
        texcoords.append(QVector2D(0.0f, 0.0f));
        indices.append(0);
        indices.append(1);
        indices.append(3);
        indices.append(1);
        indices.append(2);
        indices.append(3);
    } else if (_screenType == screenTypeCylinder) {
        QVector3D center(_screenCylinder[0], _screenCylinder[1], _screenCylinder[2]);
        QVector3D up(_screenCylinder[3], _screenCylinder[4], _screenCylinder[5]);
        float radius = _screenCylinder[6];
        float phiCenter = _screenCylinder[7];
        float phiRange = _screenCylinder[8];
        float thetaRange = _screenCylinder[9];
        float py = radius * std::tan(thetaRange / 2.0f);
        QVector3D rotAxis = QVector3D::crossProduct(QVector3D(0.0f, 1.0f, 0.0f), up);
        float rotAngle = qRadiansToDegrees(
                std::acos(QVector3D::dotProduct(QVector3D(0.0f, 1.0f, 0.0f), up)
                    / std::sqrt(QVector3D::dotProduct(up, up))));
        QMatrix4x4 M;
        M.rotate(90.0f, 0.0f, 1.0f, 0.0f);
        M.rotate(rotAngle, rotAxis);
        M.translate(center);
        const int N = 1000;
        for (int x = 0; x <= N; x++) {
            float xf = x / static_cast<float>(N);
            float phi = phiCenter + (xf - 0.5f) * phiRange;
            float px = radius * std::cos(phi);
            float pz = radius * std::sin(phi);
            positions.append(M * QVector3D(px, py, pz));
            texcoords.append(QVector2D(xf, 0.0f));
            positions.append(M * QVector3D(px, -py, pz));
            texcoords.append(QVector2D(xf, 1.0f));
            if (x > 0) {
                indices.append(2 * (x - 1));
                indices.append(2 * (x - 1) + 1);
                indices.append(2 * x + 1);
                indices.append(2 * (x - 1));
                indices.append(2 * x + 1);
                indices.append(2 * x);
            }
        }
    }
}

void QVRVNCViewer::serializeDynamicData(QDataStream& ds) const
{
    ds << _vncWidth << _vncHeight;
    ds << _vncDirtyRectangles;
    for (int i = 0; i < _vncDirtyRectangles.size(); i++) {
        QRect r = _vncDirtyRectangles[i];
        for (int y = r.y(); y < r.y() + r.height(); y++) {
            ds.writeRawData(reinterpret_cast<const char *>(&_vncFrame[y * _vncWidth]),
                    r.width() * sizeof(unsigned int));
        }
    }
}

void QVRVNCViewer::deserializeDynamicData(QDataStream& ds)
{
    ds >> _vncWidth >> _vncHeight;
    _vncFrame.resize(_vncWidth * _vncHeight);
    ds >> _vncDirtyRectangles;
    for (int i = 0; i < _vncDirtyRectangles.size(); i++) {
        QRect r = _vncDirtyRectangles[i];
        for (int y = r.y(); y < r.y() + r.height(); y++) {
            ds.readRawData(reinterpret_cast<char *>(&_vncFrame[y * _vncWidth]),
                    r.width() * sizeof(unsigned int));
        }
    }
}

void QVRVNCViewer::update(const QList<QVRObserver*>&)
{
    _vncDirtyRectangles.clear();
    int i = WaitForMessage(_vncClient, 1);
    if (i > 0 && !HandleRFBServerMessage(_vncClient)) {
        qWarning("VNC event handling failed");
    }
}

bool QVRVNCViewer::wantExit()
{
    return _wantExit;
}

bool QVRVNCViewer::initProcess(QVRProcess* p)
{
    // Qt-based OpenGL function pointers
    initializeOpenGLFunctions();

    // OpenGL
    glGenFramebuffers(1, &_fbo);

    glGenTextures(1, &_vncTex);
    glBindTexture(GL_TEXTURE_2D, _vncTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _vncWidth, _vncHeight, 0,
            GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);

    glGenVertexArrays(1, &_screenVao);
    glBindVertexArray(_screenVao);
    QVector<QVector3D> positions;
    QVector<QVector2D> texcoords;
    QVector<unsigned int> indices;
    createSceneGeometry(positions, texcoords, indices);
    GLuint positionBuf, texcoordBuf, indexBuf;
    glGenBuffers(1, &positionBuf);
    glBindBuffer(GL_ARRAY_BUFFER, positionBuf);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(QVector3D), positions.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    glGenBuffers(1, &texcoordBuf);
    glBindBuffer(GL_ARRAY_BUFFER, texcoordBuf);
    glBufferData(GL_ARRAY_BUFFER, texcoords.size() * sizeof(QVector2D), texcoords.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);
    glGenBuffers(1, &indexBuf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    _screenIndices = indices.size();

    _prg.addShaderFromSourceFile(QOpenGLShader::Vertex, ":vertex-shader.glsl");
    _prg.addShaderFromSourceFile(QOpenGLShader::Fragment, ":fragment-shader.glsl");
    _prg.link();

    // VNC
    // We only have a VNC client on the master process, and send framebuffer
    // data to slave processes as necessary.
    if (p->index() == 0) {
        _vncClient = rfbGetClient(8, 3, 4); // 32 bpp
        _vncClient->MallocFrameBuffer = vncResizeCallback;
        _vncClient->canHandleNewFBSize = TRUE;
        _vncClient->GotFrameBufferUpdate = vncUpdateCallback;
        _vncClient->listenPort = LISTEN_PORT_OFFSET;
#ifdef HAVE_RFBCLIENT_LISTEN6PORT
        _vncClient->listen6Port = LISTEN_PORT_OFFSET;
#endif
        if (!rfbInitClient(_vncClient, &_argc, _argv)) {
            qCritical("Cannot initialize VNC client");
            return false;
        }
    }

    return true;
}

void QVRVNCViewer::preRenderProcess(QVRProcess* /* p */)
{
    GLint vncTexWidth, vncTexHeight;
    glBindTexture(GL_TEXTURE_2D, _vncTex);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &vncTexWidth);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &vncTexHeight);
    if (vncTexWidth != _vncWidth || vncTexHeight != _vncHeight) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _vncWidth, _vncHeight, 0,
                GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);
    }
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _vncWidth, _vncHeight,
            GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, _vncFrame.data());
}

void QVRVNCViewer::render(QVRWindow* /* w */,
        const QVRRenderContext& context, const unsigned int* textures)
{
    for (int view = 0; view < context.viewCount(); view++) {
        // Set up framebuffer object to render into
        glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[view], 0);
        // Set up view
        glViewport(0, 0, context.textureSize(view).width(), context.textureSize(view).height());
        glClear(GL_COLOR_BUFFER_BIT);
        QMatrix4x4 P = context.frustum(view).toMatrix4x4();
        QMatrix4x4 V = context.viewMatrix(view);
        // Set up shader program
        glUseProgram(_prg.programId());
        _prg.setUniformValue("pmv_matrix", P * V);
        _prg.setUniformValue("vnc_tex", 0);
        // Render
        glBindVertexArray(_screenVao);
        glBindTexture(GL_TEXTURE_2D, _vncTex);
        glDrawElements(GL_TRIANGLES, _screenIndices, GL_UNSIGNED_INT, 0);
    }
}

void QVRVNCViewer::keyPressEvent(const QVRRenderContext& /* context */, QKeyEvent* event)
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

    /* Then start QVR with your app */
    QVRVNCViewer qvrapp(argc, argv);
    if (!manager.init(&qvrapp)) {
        qCritical("Cannot initialize QVR manager");
        return 1;
    }

    /* Enter the standard Qt loop */
    return app.exec();
}
