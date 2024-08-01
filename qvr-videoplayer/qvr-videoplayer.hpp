/*
 * Copyright (C) 2017, 2018, 2019, 2020, 2021, 2022
 * Computer Graphics Group, University of Siegen
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>
 * Copyright (C) 2023, 2024  Martin Lambers <marlam@marlam.de>
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

#include <QUrl>
#include <QVideoFrame>
#include <QVideoSink>
#include <QImage>
#include <QMediaMetaData>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
class QMediaPlayer;

#include <qvr/app.hpp>

#include "screen.hpp"


class VideoFrame
{
public:
    enum StereoLayout {
        Layout_Unknown,         // unknown; needs to be guessed
        Layout_Mono,            // monoscopic video
        Layout_Top_Bottom,      // stereoscopic video, left eye top, right eye bottom
        Layout_Top_Bottom_Half, // stereoscopic video, left eye top, right eye bottom, both half height
        Layout_Bottom_Top,      // stereoscopic video, left eye bottom, right eye top
        Layout_Bottom_Top_Half, // stereoscopic video, left eye bottom, right eye top, both half height
        Layout_Left_Right,      // stereoscopic video, left eye left, right eye right
        Layout_Left_Right_Half, // stereoscopic video, left eye left, right eye right, both half width
        Layout_Right_Left,      // stereoscopic video, left eye right, right eye left
        Layout_Right_Left_Half  // stereoscopic video, left eye right, right eye left, both half width
    };

    StereoLayout stereoLayout;
    float aspectRatio;
    QImage image;

    VideoFrame();

    void update(enum StereoLayout sl, const QVideoFrame& frame);
};

class VideoSink : public QVideoSink
{
Q_OBJECT

private:
    VideoFrame* _frame; // target video frame
    bool *_frameIsNew;  // flag to set when the target frame represents a new frame
    enum VideoFrame::StereoLayout _stereoLayout; // stereo layout of current media

public:
    VideoSink(VideoFrame* frame, bool* frameIsNew);

    void newUrl(const QUrl& url);

    void newMetaData(const QMediaMetaData& metaData);

public Q_SLOTS:
    void processNewFrame(const QVideoFrame& frame);
};

class QVRVideoPlayer : public QVRApp, protected QOpenGLExtraFunctions
{
public:
    enum ScreenType {
        ScreenUnited,           // global 2D united screen given by QVR
        ScreenIntersected,      // global 2D intersected screen given by QVR
        ScreenGeometry          // explicit geometry stored in _screen
    };

    QVRVideoPlayer(ScreenType type, const Screen& screen, const QUrl& source);

private:
    /* Data not directly relevant for rendering */
    bool _wantExit;
    QUrl _source;
    QMediaPlayer* _player;
    VideoSink* _sink;

    /* Static data for rendering, initialized on the main process */
    ScreenType _screenType;
    Screen _screen;

    /* Static data for rendering, initialized in initProcess() */
    unsigned int _pbo;
    unsigned int _viewFbo;
    unsigned int _depthTex;
    unsigned int _frameTex;
    unsigned int _screenVao, _positionBuf, _texcoordBuf, _indexBuf;
    QOpenGLShaderProgram _prg;

    /* Dynamic data for rendering */
    VideoFrame* _frame;
    bool _frameIsNew;

    /* Interaction functions */
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
