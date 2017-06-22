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

#ifndef QVR_VNCVIEWER_HPP
#define QVR_VNCVIEWER_HPP

#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>

#include <qvr/app.hpp>

#include <rfb/rfbclient.h>


class QVRVNCViewer : public QVRApp, protected QOpenGLExtraFunctions
{
public:
    QVRVNCViewer(int& argc, char* argv[]);
    enum {
        screenTypeWall,
        screenTypeCylinder
    };

private:
    bool _wantExit;
    int& _argc;
    char** _argv;
    int _screenType;
    float _screenWall[9];
    float _screenCylinder[10];
    // VNC objects
    rfbClient* _vncClient;
    int _vncWidth;
    int _vncHeight;
    QVector<unsigned int> _vncFrame; // 32 bit BGRA pixels
    QVector<QRect> _vncDirtyRectangles;
    // OpenGL objects
    unsigned int _fbo;
    unsigned int _vncTex;
    unsigned int _screenVao;
    unsigned int _screenIndices;
    QOpenGLShaderProgram _prg;

    /* Helper functions for VNC callbacks */
    unsigned int* vncResize(int width, int height);
    void vncUpdate(const QRect& r);
    friend rfbBool vncResizeCallback(rfbClient* client);
    friend void vncUpdateCallback(rfbClient* client, int x, int y, int w, int h);

    /* Helper function to create the screen geometry */
    void createSceneGeometry(QVector<QVector3D>& positions,
            QVector<QVector2D>& texcoords, QVector<unsigned int>& indices);

public:
    void serializeDynamicData(QDataStream& ds) const override;
    void deserializeDynamicData(QDataStream& ds) override;

    void update(const QList<QVRObserver*>& observers) override;

    bool wantExit() override;

    bool initProcess(QVRProcess* p) override;

    void preRenderProcess(QVRProcess* p) override;

    void render(QVRWindow* w, const QVRRenderContext& context, const unsigned int* textures) override;

    void keyPressEvent(const QVRRenderContext& context, QKeyEvent* event) override;
};

#endif
