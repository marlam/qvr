/*
 * Copyright (C) 2017 Computer Graphics Group, University of Siegen
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

#ifndef QVR_VIDEOPLAYER_HPP
#define QVR_VIDEOPLAYER_HPP

#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
class QMediaPlaylist;
class QMediaPlayer;
class VideoSurface;
class VideoFrame;

#include <qvr/app.hpp>

#include "screen.hpp"


class QVRVideoPlayer : public QVRApp, protected QOpenGLExtraFunctions
{
public:
    QVRVideoPlayer(const Screen& screen, QMediaPlaylist* playlist);

private:
    /* Data not directly relevant for rendering */
    bool _wantExit;
    QMediaPlaylist* _playlist;
    QMediaPlayer* _player;
    VideoSurface* _surface;

    /* Static data for rendering, initialized on the master process */
    Screen _screen;

    /* Static data for rendering, initialized in initProcess() */
    unsigned int _fbo;
    unsigned int _pbo;
    unsigned int _rgbTex;
    unsigned int _yuvTex[3];
    unsigned int _quadVao;
    QOpenGLShaderProgram _colorConvPrg;
    unsigned int _viewFbo;
    unsigned int _depthTex;
    unsigned int _frameTex;
    unsigned int _screenVao;
    QOpenGLShaderProgram _prg;

    /* Dynamic data for rendering */
    VideoFrame* _frame;
    bool _frameIsNew;

    /* Interaction functions */
    void playlistNext();
    void playlistPrevious();
    void seek(qint64 milliseconds);
    void togglePause();
    void pause();
    void play();
    void toggleMute();
    void changeVolume(int offset);
    void stop();

public:
    void serializeStaticData(QDataStream& ds) const override;
    void deserializeStaticData(QDataStream& ds) override;

    void serializeDynamicData(QDataStream& ds) const override;
    void deserializeDynamicData(QDataStream& ds) override;

    bool wantExit() override;

    bool initProcess(QVRProcess* p) override;

    void preRenderProcess(QVRProcess* p) override;

    void render(QVRWindow* w, const QVRRenderContext& c, const unsigned int* textures) override;

    void keyPressEvent(const QVRRenderContext& context, QKeyEvent* event) override;
};

#endif
