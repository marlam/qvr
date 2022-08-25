/*
 * Copyright (C) 2017, 2018, 2019, 2020, 2021, 2022
 * Computer Graphics Group, University of Siegen
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

#include <cstring>

#include <QGuiApplication>
#include <QCommandLineParser>
#include <QKeyEvent>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QFileInfo>
#include <QTemporaryFile>

#include <qvr/manager.hpp>

#include "qvr-videoplayer.hpp"


VideoFrame::VideoFrame() :
    stereoLayout(Layout_Unknown), aspectRatio(0.0f), image()
{
}

void VideoFrame::update(enum StereoLayout sl, const QVideoFrame& frame)
{
    bool valid = (frame.pixelFormat() != QVideoFrameFormat::Format_Invalid);
    if (valid) {
        stereoLayout = sl;
        image = frame.toImage();
        image.convertTo(QImage::Format_RGB30);
        aspectRatio = float(image.width()) / image.height();
    } else {
        // Synthesize a black frame
        stereoLayout = Layout_Mono;
        image = QImage(1, 1, QImage::Format_RGB30);
        image.fill(0);
        aspectRatio = 1.0f;
    }
}

QDataStream &operator<<(QDataStream& ds, const VideoFrame& f)
{
    ds << static_cast<int>(f.stereoLayout);
    ds << f.aspectRatio;
    ds << f.image.width();
    ds << f.image.height();
    ds << f.image.sizeInBytes();
    ds.writeRawData(reinterpret_cast<const char*>(f.image.bits()), f.image.sizeInBytes());
    return ds;
}

QDataStream &operator>>(QDataStream& ds, VideoFrame& f)
{
    int sl, w, h;
    qsizetype size;
    ds >> sl;
    f.stereoLayout = static_cast<enum VideoFrame::StereoLayout>(sl);
    ds >> f.aspectRatio;
    ds >> w;
    ds >> h;
    ds >> size;
    f.image = QImage(w, h, QImage::Format_RGB30);
    Q_ASSERT(size == f.image.sizeInBytes());
    ds.readRawData(reinterpret_cast<char*>(f.image.bits()), size);
    return ds;
}

VideoSink::VideoSink(VideoFrame* frame, bool* frameIsNew) :
    _frame(frame), _frameIsNew(frameIsNew), _stereoLayout(VideoFrame::Layout_Unknown)
{
    connect(this, SIGNAL(videoFrameChanged(const QVideoFrame&)), this, SLOT(processNewFrame(const QVideoFrame&)));
}

void VideoSink::newUrl(const QUrl& url) // called whenever a new media URL is played
{
    // Reset stereo layout, then try to guess it from URL.
    // This should be compatible to these conventions:
    // http://bino3d.org/doc/bino.html#File-Name-Conventions-1
    _stereoLayout = VideoFrame::Layout_Unknown;
    QString fileName = url.fileName();
    QString marker = fileName.left(fileName.lastIndexOf('.'));
    marker = marker.right(marker.length() - marker.lastIndexOf('-') - 1);
    marker = marker.toLower();
    if (marker == "lr")
        _stereoLayout = VideoFrame::Layout_Left_Right;
    else if (marker == "rl")
        _stereoLayout = VideoFrame::Layout_Right_Left;
    else if (marker == "lrh" || marker == "lrq")
        _stereoLayout = VideoFrame::Layout_Left_Right_Half;
    else if (marker == "rlh" || marker == "rlq")
        _stereoLayout = VideoFrame::Layout_Right_Left_Half;
    else if (marker == "tb" || marker == "ab")
        _stereoLayout = VideoFrame::Layout_Top_Bottom;
    else if (marker == "bt" || marker == "ba")
        _stereoLayout = VideoFrame::Layout_Bottom_Top;
    else if (marker == "tbh" || marker == "abq")
        _stereoLayout = VideoFrame::Layout_Top_Bottom_Half;
    else if (marker == "bth" || marker == "baq")
        _stereoLayout = VideoFrame::Layout_Bottom_Top_Half;
    else if (marker == "2d")
        _stereoLayout = VideoFrame::Layout_Mono;
}

void VideoSink::newMetaData(const QMediaMetaData& metaData)
{
    /* TODO: we should set the stereoscopic layout from media meta data */
}

void VideoSink::processNewFrame(const QVideoFrame& frame)
{
    _frame->update(_stereoLayout, frame);
    *_frameIsNew = true;
}

static bool isGLES = false; // Is this OpenGL ES or plain OpenGL? Initialized in main().

QVRVideoPlayer::QVRVideoPlayer(const Screen& screen, const QUrl& source) :
    _wantExit(false),
    _source(source),
    _player(NULL),
    _sink(NULL),
    _screen(screen),
    _frame(NULL),
    _frameIsNew(false)
{
}

void QVRVideoPlayer::serializeStaticData(QDataStream& ds) const
{
    ds << _screen;
}

void QVRVideoPlayer::deserializeStaticData(QDataStream& ds)
{
    ds >> _screen;
}

void QVRVideoPlayer::serializeDynamicData(QDataStream& ds) const
{
    ds << _frameIsNew;
    if (_frameIsNew)
        ds << (*_frame);
}

void QVRVideoPlayer::deserializeDynamicData(QDataStream& ds)
{
    ds >> _frameIsNew;
    if (_frameIsNew)
        ds >> (*_frame);
}

bool QVRVideoPlayer::wantExit()
{
    return _wantExit;
}

// Helper function: read a complete file into a QString (without error checking)
static QString readFile(const char* fileName)
{
    QFile f(fileName);
    f.open(QIODevice::ReadOnly);
    QTextStream in(&f);
    return in.readAll();
}

bool QVRVideoPlayer::initProcess(QVRProcess* /* p */)
{
    // Qt-based OpenGL function pointers
    initializeOpenGLFunctions();

    // FBO and PBO
    glGenBuffers(1, &_pbo);
    glGenFramebuffers(1, &_viewFbo);
    if (_screen.isPlanar) {
        _depthTex = 0;
    } else {
        glGenTextures(1, &_depthTex);
        glBindTexture(GL_TEXTURE_2D, _depthTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 1, 1,
                0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
        glBindFramebuffer(GL_FRAMEBUFFER, _viewFbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTex, 0);
    }

    // Quad geometry
    const float quadPositions[] = {
        -1.0f, +1.0f, 0.0f,
        +1.0f, +1.0f, 0.0f,
        +1.0f, -1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f
    };
    const float quadTexCoords[] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 0.0f
    };
    static const unsigned short quadIndices[] = {
        0, 3, 1, 1, 3, 2
    };
    glGenVertexArrays(1, &_quadVao);
    glBindVertexArray(_quadVao);
    GLuint quadPositionBuf;
    glGenBuffers(1, &quadPositionBuf);
    glBindBuffer(GL_ARRAY_BUFFER, quadPositionBuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadPositions), quadPositions, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    GLuint quadTexCoordBuf;
    glGenBuffers(1, &quadTexCoordBuf);
    glBindBuffer(GL_ARRAY_BUFFER, quadTexCoordBuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadTexCoords), quadTexCoords, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);
    GLuint quadIndexBuf;
    glGenBuffers(1, &quadIndexBuf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadIndexBuf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

    // Frame texture
    glGenTextures(1, &_frameTex);
    glBindTexture(GL_TEXTURE_2D, _frameTex);
    unsigned int black = 0;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB10_A2, 1, 1, 0, GL_RGBA, GL_UNSIGNED_INT_10_10_10_2, &black);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (_screen.isPlanar) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        if (!isGLES)
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4.0f);
    }

    // Screen geometry
    glGenVertexArrays(1, &_screenVao);
    glBindVertexArray(_screenVao);
    GLuint positionBuf;
    glGenBuffers(1, &positionBuf);
    glBindBuffer(GL_ARRAY_BUFFER, positionBuf);
    glBufferData(GL_ARRAY_BUFFER, _screen.positions.size() * sizeof(float),
            _screen.positions.constData(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    GLuint texcoordBuf;
    glGenBuffers(1, &texcoordBuf);
    glBindBuffer(GL_ARRAY_BUFFER, texcoordBuf);
    glBufferData(GL_ARRAY_BUFFER, _screen.texCoords.size() * sizeof(float),
            _screen.texCoords.constData(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);
    GLuint indexBuf;
    glGenBuffers(1, &indexBuf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            _screen.indices.length() * sizeof(unsigned short),
            _screen.indices.constData(), GL_STATIC_DRAW);

    // Shader program
    QString vertexShaderSource = readFile(":vertex-shader.glsl");
    QString fragmentShaderSource = readFile(":fragment-shader.glsl");
    if (isGLES) {
        vertexShaderSource.prepend("#version 300 es\n");
        fragmentShaderSource.prepend("#version 300 es\n");
    } else {
        vertexShaderSource.prepend("#version 330\n");
        fragmentShaderSource.prepend("#version 330\n");
    }
    _prg.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    _prg.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    _prg.link();

    // Media Player
    _frame = new VideoFrame;
    if (QVRManager::processIndex() == 0) {
        _sink = new VideoSink(_frame, &_frameIsNew);
        _player = new QMediaPlayer;
        _player->connect(_player, &QMediaPlayer::errorOccurred,
                [=](QMediaPlayer::Error error, const QString& errorString) {
                    qCritical("Error: %s", qPrintable(errorString));
                    _wantExit = true;
                });
        _player->connect(_player, &QMediaPlayer::playbackStateChanged,
                [=](QMediaPlayer::PlaybackState state) {
                    if (state == QMediaPlayer::StoppedState)
                        _wantExit = true;
                });
        _player->connect(_player, &QMediaPlayer::sourceChanged,
                [=](const QUrl& source) {
                    _sink->newUrl(source);
                });
        _player->connect(_player, &QMediaPlayer::metaDataChanged,
                [=]() {
                    _sink->newMetaData(_player->metaData());
                });
        _player->setVideoOutput(_sink);
        _player->setAudioOutput(new QAudioOutput());
        _player->setSource(_source);
        _player->play();
    }

    return true;
}

void QVRVideoPlayer::preRenderProcess(QVRProcess* /* p */)
{
    /* We need to get new frame data into a texture that is suitable for
     * rendering the screen: _frameTex.
     * To make this easy, we have ensured in VideoFrame that the frame data is
     * in format 2-10-10-10 per pixel, and we just upload that to _frameTex. */
    if (_frameIsNew && QVRManager::windowCount() > 0) {
        // First copy the frame data into a PBO and from there into the texture.
        // This is faster than using glTexImage2D() directly on the frame data.
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pbo);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, _frame->image.sizeInBytes(), NULL, GL_STREAM_DRAW);
        void* ptr = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, _frame->image.sizeInBytes(),
                GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
        Q_ASSERT(ptr);
        std::memcpy(ptr, _frame->image.constBits(), _frame->image.sizeInBytes());
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        const int w = _frame->image.width();
        const int h = _frame->image.height();
        glBindTexture(GL_TEXTURE_2D, _frameTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB10_A2, w, h, 0, GL_BGRA, GL_UNSIGNED_INT_2_10_10_10_REV, NULL);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, _frameTex);
        if (!_screen.isPlanar)
            glGenerateMipmap(GL_TEXTURE_2D);
        _frameIsNew = false;
    }
}

void QVRVideoPlayer::render(QVRWindow* /* w */,
        const QVRRenderContext& context, const unsigned int* textures)
{
    for (int view = 0; view < context.viewCount(); view++) {
        // Get view dimensions
        int width = context.textureSize(view).width();
        int height = context.textureSize(view).height();
        // Set up framebuffer object to render into
        if (!_screen.isPlanar) {
            glBindTexture(GL_TEXTURE_2D, _depthTex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height,
                    0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, _viewFbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[view], 0);
        // Set up view
        glViewport(0, 0, width, height);
        glClear(_screen.isPlanar ? GL_COLOR_BUFFER_BIT : (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
        QMatrix4x4 projectionMatrix = context.frustum(view).toMatrix4x4();
        QMatrix4x4 viewMatrix = context.viewMatrix(view);
        // Set up stereo layout
        enum VideoFrame::StereoLayout stereoLayout = _frame->stereoLayout;
        if (stereoLayout == VideoFrame::Layout_Unknown) {
            if (_frame->aspectRatio > 3.0f)
                stereoLayout = VideoFrame::Layout_Left_Right;
            else if (_frame->aspectRatio < 1.0f)
                stereoLayout = VideoFrame::Layout_Top_Bottom;
        }
        float frameAspectRatio = _frame->aspectRatio;
        float viewOffsetX = 0.0f;
        float viewFactorX = 1.0f;
        float viewOffsetY = 0.0f;
        float viewFactorY = 1.0f;
        switch (stereoLayout) {
        case VideoFrame::Layout_Unknown: // assume it is Mono
        case VideoFrame::Layout_Mono:
            // nothing to do
            break;
        case VideoFrame::Layout_Top_Bottom:
            viewFactorY = 0.5f;
            viewOffsetY = (context.eye(view) == QVR_Eye_Right ? 0.5f : 0.0f);
            frameAspectRatio *= 2.0f;
            break;
        case VideoFrame::Layout_Top_Bottom_Half:
            viewFactorY = 0.5f;
            viewOffsetY = (context.eye(view) == QVR_Eye_Right ? 0.5f : 0.0f);
            break;
        case VideoFrame::Layout_Bottom_Top:
            viewFactorY = 0.5f;
            viewOffsetY = (context.eye(view) != QVR_Eye_Right ? 0.5f : 0.0f);
            frameAspectRatio *= 2.0f;
            break;
        case VideoFrame::Layout_Bottom_Top_Half:
            viewFactorY = 0.5f;
            viewOffsetY = (context.eye(view) != QVR_Eye_Right ? 0.5f : 0.0f);
            break;
        case VideoFrame::Layout_Left_Right:
            viewFactorX = 0.5f;
            viewOffsetX = (context.eye(view) == QVR_Eye_Right ? 0.5f : 0.0f);
            frameAspectRatio /= 2.0f;
            break;
        case VideoFrame::Layout_Left_Right_Half:
            viewFactorX = 0.5f;
            viewOffsetX = (context.eye(view) == QVR_Eye_Right ? 0.5f : 0.0f);
            break;
        case VideoFrame::Layout_Right_Left:
            viewFactorX = 0.5f;
            viewOffsetX = (context.eye(view) != QVR_Eye_Right ? 0.5f : 0.0f);
            frameAspectRatio /= 2.0f;
            break;
        case VideoFrame::Layout_Right_Left_Half:
            viewFactorX = 0.5f;
            viewOffsetX = (context.eye(view) != QVR_Eye_Right ? 0.5f : 0.0f);
            break;
        }
        // Set up correct aspect ratio on screen
        float relWidth = 1.0f;
        float relHeight = 1.0f;
        if (_screen.aspectRatio < frameAspectRatio)
            relHeight = _screen.aspectRatio / frameAspectRatio;
        else
            relWidth = frameAspectRatio / _screen.aspectRatio;
        // Set up shader program
        glUseProgram(_prg.programId());
        _prg.setUniformValue("projection_model_view_matrix", projectionMatrix * viewMatrix);
        _prg.setUniformValue("view_offset_x", viewOffsetX);
        _prg.setUniformValue("view_factor_x", viewFactorX);
        _prg.setUniformValue("view_offset_y", viewOffsetY);
        _prg.setUniformValue("view_factor_y", viewFactorY);
        _prg.setUniformValue("relative_width", relWidth);
        _prg.setUniformValue("relative_height", relHeight);
        // Render scene
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _frameTex);
        glBindVertexArray(_screenVao);
        glDrawElements(GL_TRIANGLES, _screen.indices.size(), GL_UNSIGNED_SHORT, 0);
        // Invalidate depth attachment (to help OpenGL ES performance)
        if (!_screen.isPlanar) {
            const GLenum fboInvalidations[] = { GL_DEPTH_ATTACHMENT };
            glInvalidateFramebuffer(GL_FRAMEBUFFER, 1, fboInvalidations);
        }
    }
}

void QVRVideoPlayer::seek(qint64 milliseconds)
{
    _player->setPosition(_player->position() + milliseconds);
}

void QVRVideoPlayer::togglePause()
{
    if (_player->playbackState() == QMediaPlayer::PlayingState)
        _player->pause();
    else if (_player->playbackState() == QMediaPlayer::PausedState)
        _player->play();
}

void QVRVideoPlayer::pause()
{
    if (_player->playbackState() == QMediaPlayer::PlayingState)
        _player->pause();
}

void QVRVideoPlayer::play()
{
    if (_player->playbackState() == QMediaPlayer::PausedState)
        _player->play();
}

void QVRVideoPlayer::toggleMute()
{
    _player->audioOutput()->setMuted(!_player->audioOutput()->isMuted());
}

void QVRVideoPlayer::changeVolume(int offset)
{
    _player->audioOutput()->setVolume(_player->audioOutput()->volume() + offset);
}

void QVRVideoPlayer::stop()
{
    _player->stop();
}

void QVRVideoPlayer::keyPressEvent(const QVRRenderContext& /* context */, QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Escape:
    case Qt::Key_Q:
    case Qt::Key_MediaStop:
        stop();
        break;
    case Qt::Key_Space:
    case Qt::Key_MediaTogglePlayPause:
        togglePause();
        break;
    case Qt::Key_MediaPause:
        pause();
        break;
    case Qt::Key_MediaPlay:
        play();
        break;
    case Qt::Key_M:
    case Qt::Key_VolumeMute:
        toggleMute();
        break;
    case Qt::Key_VolumeDown:
        changeVolume(-5);
        break;
    case Qt::Key_VolumeUp:
        changeVolume(+5);
        break;
    case Qt::Key_Left:
        seek(-10000);
        break;
    case Qt::Key_Right:
        seek(+10000);
        break;
    case Qt::Key_Down:
        seek(-60000);
        break;
    case Qt::Key_Up:
        seek(+60000);
        break;
    case Qt::Key_PageDown:
        seek(-600000);
        break;
    case Qt::Key_PageUp:
        seek(+600000);
        break;
    }
}

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName("qvr-videoplayer");
    QVRManager manager(argc, argv);
    isGLES = (QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGLES);

    /* Process command line */
    Screen screen(
            QVector3D(-16.0f / 9.0f, -1.0f + QVRObserverConfig::defaultEyeHeight, -8.0f),
            QVector3D(+16.0f / 9.0f, -1.0f + QVRObserverConfig::defaultEyeHeight, -8.0f),
            QVector3D(-16.0f / 9.0f, +1.0f + QVRObserverConfig::defaultEyeHeight, -8.0f));
    QUrl source;
    if (QVRManager::processIndex() == 0) {
        QCommandLineParser parser;
        parser.setApplicationDescription("QVR video player");
        parser.addHelpOption();
        parser.addVersionOption();
        parser.addPositionalArgument("video...", "Video file(s) to play.");
        parser.addOptions({
                { { "s", "screen" }, "Set screen geometry.", "screen" },
        });
        parser.process(app);
        QStringList posArgs = parser.positionalArguments();
        if (posArgs.length() == 0) {
            QFile file(":logo.mp4");
            QTemporaryFile* tmpFile = QTemporaryFile::createNativeFile(file);
            source = QUrl::fromLocalFile(tmpFile->fileName());
        } else if (posArgs.length() == 1) {
            for (int i = 0; i < posArgs.length(); i++) {
                QUrl url(posArgs[i]);
                if (url.isRelative()) {
                    QFileInfo videoFileInfo(posArgs[i]);
                    if (videoFileInfo.exists()) {
                        url = QUrl::fromLocalFile(videoFileInfo.canonicalFilePath());
                    } else {
                        qCritical("File does not exist: %s", qPrintable(posArgs[i]));
                        return 1;
                    }
                }
                source = url;
            }
        } else {
            qInfo("Only one video file can be played");
            return 1;
        }
        if (parser.isSet("screen")) {
            QStringList paramList = parser.value("screen").split(',');
            float values[9];
            if (paramList.length() == 9
                    && 9 == std::sscanf(qPrintable(parser.value("screen")),
                        "%f,%f,%f,%f,%f,%f,%f,%f,%f",
                        values + 0, values + 1, values + 2,
                        values + 3, values + 4, values + 5,
                        values + 6, values + 7, values + 8)) {
                screen = Screen(
                        QVector3D(values[0], values[1], values[2]),
                        QVector3D(values[3], values[4], values[5]),
                        QVector3D(values[6], values[7], values[8]));
            } else if (paramList.length() == 2) {
                float ar;
                float ar2[2];
                if (2 == std::sscanf(qPrintable(paramList[0]), "%f:%f", ar2 + 0, ar2 + 1)) {
                    ar = ar2[0] / ar2[1];
                } else if (1 != std::sscanf(qPrintable(paramList[0]), "%f", &ar)) {
                    qCritical("Invalid aspect ratio %s", qPrintable(paramList[0]));
                    return 1;
                }
                screen = Screen(paramList[1], ar);
                if (screen.indices.size() == 0)
                    return 1;
            } else {
                qCritical("Invalid screen definition: %s", qPrintable(parser.value("screen")));
                return 1;
            }
        } else {
            qInfo("Using default video screen");
        }
    }

    /* First set the default surface format that all windows will use */
    QSurfaceFormat format;
    if (isGLES) {
        format.setVersion(3, 0);
    } else {
        format.setProfile(QSurfaceFormat::CoreProfile);
        format.setVersion(3, 3);
    }
    QSurfaceFormat::setDefaultFormat(format);

    /* Then start QVR with your app */
    QVRVideoPlayer qvrapp(screen, source);
    if (!manager.init(&qvrapp)) {
        qCritical("Cannot initialize QVR manager");
        return 1;
    }

    /* Enter the standard Qt loop */
    return app.exec();
}
