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

#include <cstring>

#include <QGuiApplication>
#include <QCommandLineParser>
#include <QKeyEvent>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>
#include <QFileInfo>
#include <QTemporaryFile>

#include <qvr/manager.hpp>

#include "qvr-videoplayer.hpp"


/*
 * The VideoFrame class.
 *
 * This class represents one video frame, in a form that allows serialization
 * to a QDataStream (for multi-process support).
 * On the master process, copying the frame data is avoided.
 */

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

    QVideoFrame::PixelFormat pixelFormat;
    QVideoSurfaceFormat::YCbCrColorSpace yCbCrColorSpace;
    QSize size;
    float aspectRatio;
    StereoLayout stereoLayout;
    QByteArray data;

    VideoFrame() :
        pixelFormat(QVideoFrame::Format_Invalid),
        yCbCrColorSpace(QVideoSurfaceFormat::YCbCr_Undefined),
        size(-1, -1),
        aspectRatio(0.0f),
        stereoLayout(Layout_Unknown),
        data()
    {
    }

    // Map a QVideoFrame to this video frame; see also unmap()
    void map(enum StereoLayout sl, const QVideoSurfaceFormat& format, const QVideoFrame& frame)
    {
        bool valid = (frame.pixelFormat() != QVideoFrame::Format_Invalid);
        if (valid) {
            // This assignment does not copy the frame data:
            _mapFrame = frame;
            if (!_mapFrame.map(QAbstractVideoBuffer::ReadOnly))
                valid = false;
        }
        if (valid) {
            pixelFormat = frame.pixelFormat();
            yCbCrColorSpace = format.yCbCrColorSpace();
            size = frame.size();
            aspectRatio = static_cast<float>(size.width() * format.pixelAspectRatio().width())
                / static_cast<float>(size.height() * format.pixelAspectRatio().height());
            stereoLayout = sl;
            // This assignment does not copy the frame data:
            data.setRawData(reinterpret_cast<const char*>(_mapFrame.bits()), _mapFrame.mappedBytes());
        } else {
            // Synthesize a black frame
            pixelFormat = QVideoFrame::Format_ARGB32;
            yCbCrColorSpace = QVideoSurfaceFormat::YCbCr_Undefined;
            size = QSize(1, 1);
            aspectRatio = 1.0f;
            stereoLayout = Layout_Mono;
            static const char zeroes[4] = { 0, 0, 0, 0 };
            data.setRawData(zeroes, 4);
        }
    }

    // Unmap this video frame (must be called if map() was called)
    void unmap()
    {
        if (_mapFrame.isMapped())
            _mapFrame.unmap();
    }

private:
    QVideoFrame _mapFrame;
};

QDataStream &operator<<(QDataStream& ds, const VideoFrame& f)
{
    ds << static_cast<int>(f.pixelFormat);
    ds << static_cast<int>(f.yCbCrColorSpace);
    ds << f.size;
    ds << f.aspectRatio;
    ds << static_cast<int>(f.stereoLayout);
    ds << f.data;
    return ds;
}

QDataStream &operator>>(QDataStream& ds, VideoFrame& f)
{
    int tmp;
    ds >> tmp;
    f.pixelFormat = static_cast<QVideoFrame::PixelFormat>(tmp);
    ds >> tmp;
    f.yCbCrColorSpace = static_cast<QVideoSurfaceFormat::YCbCrColorSpace>(tmp);
    ds >> f.size;
    ds >> f.aspectRatio;
    ds >> tmp;
    f.stereoLayout = static_cast<enum VideoFrame::StereoLayout>(tmp);
    ds >> f.data;
    return ds;
}

/*
 * The VideoSurface class.
 *
 * This class is used by QMediaPlayer as a video surface, i.e. to output video
 * frames.
 * It specifies the video frame formats we can handle, and maps the incoming
 * QVideoFrame data to our video frame representation.
 */

class VideoSurface : public QAbstractVideoSurface
{
private:
    VideoFrame* _frame; // target video frame
    bool *_frameIsNew;  // flag to set when the target frame represents a new frame
    QVideoSurfaceFormat _format; // format with which playback is started
    enum VideoFrame::StereoLayout _stereoLayout; // stereo layout of current media

public:
    VideoSurface(VideoFrame* frame, bool* frameIsNew) :
        _frame(frame), _frameIsNew(frameIsNew), _stereoLayout(VideoFrame::Layout_Unknown)
    {
    }

    virtual QList<QVideoFrame::PixelFormat> supportedPixelFormats(
            QAbstractVideoBuffer::HandleType type = QAbstractVideoBuffer::NoHandle) const
    {
        Q_UNUSED(type);
        QList<QVideoFrame::PixelFormat> pixelFormats;
        pixelFormats.append(QVideoFrame::Format_RGB24);
        pixelFormats.append(QVideoFrame::Format_ARGB32);
        pixelFormats.append(QVideoFrame::Format_RGB32);
        pixelFormats.append(QVideoFrame::Format_RGB565);
        pixelFormats.append(QVideoFrame::Format_BGRA32);
        pixelFormats.append(QVideoFrame::Format_BGR32);
        pixelFormats.append(QVideoFrame::Format_BGR24);
        pixelFormats.append(QVideoFrame::Format_BGR565);
        pixelFormats.append(QVideoFrame::Format_YUV420P);
        pixelFormats.append(QVideoFrame::Format_YUV444);
        pixelFormats.append(QVideoFrame::Format_AYUV444);
        pixelFormats.append(QVideoFrame::Format_YV12);
        // TODO: we could support more formats with a little bit more effort in
        // preRenderProcess(), but probably RGB24 and YUV420P are the only
        // ones that are really relevant.
        return pixelFormats;
    }

    virtual bool present(const QVideoFrame &frame)
    {
        _frame->unmap();
        _frame->map(_stereoLayout, _format, frame);
        *_frameIsNew = true;
        return true;
    }

    virtual bool start(const QVideoSurfaceFormat &format)
    {
        _format = format;
        return QAbstractVideoSurface::start(format);
    }

    void newUrl(const QUrl& url) // called whenever a new media URL is played
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

    void newMetaData(const QMediaObject* mediaObject) // called whenever new media is played
    {
        /* TODO: we should set the stereoscopic layout from media meta data, but unfortunately
         * Qt does not give us access to the full media meta data, only to a small subset
         * of predefined keys, which do not include information about stereoscopic layouts... */
        Q_UNUSED(mediaObject);
    }
};


static bool isGLES = false; // Is this OpenGL ES or plain OpenGL? Initialized in main().

QVRVideoPlayer::QVRVideoPlayer(const Screen& screen, QMediaPlaylist* playlist) :
    _wantExit(false),
    _playlist(playlist),
    _player(NULL),
    _surface(NULL),
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
    glGenFramebuffers(1, &_fbo);
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

    // Color data textures
    glGenTextures(1, &_rgbTex);
    glBindTexture(GL_TEXTURE_2D, _rgbTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glGenTextures(3, _yuvTex);
    glBindTexture(GL_TEXTURE_2D, _yuvTex[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, _yuvTex[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, _yuvTex[2]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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

    // Color conversion shader program
    QString colorConvVertexShaderSource = readFile(":colorconv-vs.glsl");
    QString colorConvFragmentShaderSource = readFile(":colorconv-fs.glsl");
    if (isGLES) {
        colorConvVertexShaderSource.prepend("#version 300 es\n");
        colorConvFragmentShaderSource.prepend("#version 300 es\n");
    } else {
        colorConvVertexShaderSource.prepend("#version 330\n");
        colorConvFragmentShaderSource.prepend("#version 330\n");
    }
    _colorConvPrg.addShaderFromSourceCode(QOpenGLShader::Vertex, colorConvVertexShaderSource);
    _colorConvPrg.addShaderFromSourceCode(QOpenGLShader::Fragment, colorConvFragmentShaderSource);
    _colorConvPrg.link();

    // Frame texture
    glGenTextures(1, &_frameTex);
    glBindTexture(GL_TEXTURE_2D, _frameTex);
    unsigned int black = 0;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB10_A2, 1, 1, 0, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, &black);
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
        _surface = new VideoSurface(_frame, &_frameIsNew);
        _player = new QMediaPlayer(NULL, QMediaPlayer::VideoSurface);
        _player->connect(_player, static_cast<void(QMediaPlayer::*)(QMediaPlayer::Error)>(&QMediaPlayer::error),
                [=](QMediaPlayer::Error error) {
                    if (error != QMediaPlayer::NoError) {
                        //qCritical("Error: %s", qPrintable(_player->errorString()));
                        _wantExit = true;
                    }
                });
        _player->connect(_player, &QMediaPlayer::stateChanged,
                [=](QMediaPlayer::State state) {
                    if (state == QMediaPlayer::StoppedState)
                        _wantExit = true;
                });
        _player->connect(_player, &QMediaPlayer::currentMediaChanged,
                [=](const QMediaContent &content) {
                    _surface->newUrl(content.canonicalUrl());
                });
        _player->connect(_player, &QMediaPlayer::metaDataAvailableChanged,
                [=](bool available) {
                    if (available)
                        _surface->newMetaData(_player);
                });
        _player->setVideoOutput(_surface);
        _player->setPlaylist(_playlist);
        _player->play();
    }

    return true;
}

void QVRVideoPlayer::preRenderProcess(QVRProcess* /* p */)
{
    /* We need to get new frame data into a texture that is suitable for
     * rendering the screen: _frameTex.
     * On the way, we typically need to convert the color space with a fragment
     * shader. SRGB texture formats are not suitable since we cannot render
     * into them from OpenGL ES. Therefore we need to store linear RGB. This
     * means 8 bit per color is not enough since we would lose details in dark
     * regions. GL_RGB10_A2 seems to be a good choice for _frameTex: we can
     * render into that format, it is filterable, and 10 bit per color should be
     * enough. */
    if (_frameIsNew && QVRManager::windowCount() > 0) {
        // First copy the frame data into a PBO and from there into textures.
        // This is faster than using glTexImage2D() directly on the frame data.
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pbo);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, _frame->data.size(), NULL, GL_STREAM_DRAW);
        void* ptr = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, _frame->data.size(),
                GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
        Q_ASSERT(ptr);
        std::memcpy(ptr, _frame->data.constData(), _frame->data.size());
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        const int w = _frame->size.width();
        const int h = _frame->size.height();
        int shaderInput; // 0=rgbTex.rgb, 1=yuvTex.rgb, 2=vec3(yuvTex[0].r,yuvTex[1].r,yuvTex[2].r)
        switch (_frame->pixelFormat) {
        case QVideoFrame::Format_RGB24:
            glBindTexture(GL_TEXTURE_2D, _rgbTex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
            shaderInput = 0;
            break;
        case QVideoFrame::Format_BGR24:
            glBindTexture(GL_TEXTURE_2D, _rgbTex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
            shaderInput = 0;
            break;
        case QVideoFrame::Format_ARGB32:
        case QVideoFrame::Format_RGB32:
            glBindTexture(GL_TEXTURE_2D, _rgbTex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_GREEN);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_BLUE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ALPHA);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
            shaderInput = 0;
            break;
        case QVideoFrame::Format_BGRA32:
        case QVideoFrame::Format_BGR32:
            glBindTexture(GL_TEXTURE_2D, _rgbTex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
            shaderInput = 0;
            break;
        case QVideoFrame::Format_RGB565:
            glBindTexture(GL_TEXTURE_2D, _rgbTex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB565, w, h, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
            shaderInput = 0;
            break;
        case QVideoFrame::Format_BGR565:
            glBindTexture(GL_TEXTURE_2D, _rgbTex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB565, w, h, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
            shaderInput = 0;
            break;
        case QVideoFrame::Format_YUV444:
            glBindTexture(GL_TEXTURE_2D, _yuvTex[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
            shaderInput = 1;
            break;
        case QVideoFrame::Format_AYUV444:
            glBindTexture(GL_TEXTURE_2D, _yuvTex[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_GREEN);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_BLUE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ALPHA);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
            shaderInput = 1;
            break;
        case QVideoFrame::Format_YUV420P:
            glBindTexture(GL_TEXTURE_2D, _yuvTex[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ZERO);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ZERO);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
            glBindTexture(GL_TEXTURE_2D, _yuvTex[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w / 2, h / 2, 0, GL_RED, GL_UNSIGNED_BYTE,
                    reinterpret_cast<const GLvoid*>(w * h));
            glBindTexture(GL_TEXTURE_2D, _yuvTex[2]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w / 2, h / 2, 0, GL_RED, GL_UNSIGNED_BYTE,
                    reinterpret_cast<const GLvoid*>(w * h + w * h / 4));
            shaderInput = 2;
            break;
        case QVideoFrame::Format_YV12:
            glBindTexture(GL_TEXTURE_2D, _yuvTex[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ZERO);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ZERO);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
            glBindTexture(GL_TEXTURE_2D, _yuvTex[2]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w / 2, h / 2, 0, GL_RED, GL_UNSIGNED_BYTE,
                    reinterpret_cast<const GLvoid*>(w * h));
            glBindTexture(GL_TEXTURE_2D, _yuvTex[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w / 2, h / 2, 0, GL_RED, GL_UNSIGNED_BYTE,
                    reinterpret_cast<const GLvoid*>(w * h + w * h / 4));
            shaderInput = 2;
            break;
        default:
            qFatal("unknown pixel format %d", _frame->pixelFormat);
            break;
        }
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, _frameTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB10_A2, w, h, 0, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, NULL);
        glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _frameTex, 0);
        glDisable(GL_DEPTH_TEST);
        glViewport(0, 0, w, h);
        glUseProgram(_colorConvPrg.programId());
        _colorConvPrg.setUniformValue("rgb_tex", 0);
        _colorConvPrg.setUniformValue("yuv_tex0", 0);
        _colorConvPrg.setUniformValue("yuv_tex1", 0);
        _colorConvPrg.setUniformValue("yuv_tex2", 0);
        _colorConvPrg.setUniformValue("shader_input", shaderInput);
        if (shaderInput != 0) {
            switch (_frame->yCbCrColorSpace) {
            case QVideoSurfaceFormat::YCbCr_Undefined: // XXX: seems to be used when in doubt!?
            case QVideoSurfaceFormat::YCbCr_BT601:
                _colorConvPrg.setUniformValue("yuv_value_range_8bit_mpeg", true);
                _colorConvPrg.setUniformValue("yuv_709", false);
                break;
            case QVideoSurfaceFormat::YCbCr_BT709:
                _colorConvPrg.setUniformValue("yuv_value_range_8bit_mpeg", true);
                _colorConvPrg.setUniformValue("yuv_709", true);
                break;
            case QVideoSurfaceFormat::YCbCr_xvYCC601:
                _colorConvPrg.setUniformValue("yuv_value_range_8bit_mpeg", false);
                _colorConvPrg.setUniformValue("yuv_709", false);
                break;
            case QVideoSurfaceFormat::YCbCr_xvYCC709:
                _colorConvPrg.setUniformValue("yuv_value_range_8bit_mpeg", false);
                _colorConvPrg.setUniformValue("yuv_709", true);
                break;
            default:
                qFatal("unknown YCbCr color space");
                break;
            }
        }
        glActiveTexture(GL_TEXTURE0);
        if (shaderInput == 0) {
            glBindTexture(GL_TEXTURE_2D, _rgbTex);
        } else {
            glBindTexture(GL_TEXTURE_2D, _yuvTex[0]);
            if (shaderInput == 2) {
                _colorConvPrg.setUniformValue("yuv_tex1", 1);
                _colorConvPrg.setUniformValue("yuv_tex2", 2);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, _yuvTex[1]);
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, _yuvTex[2]);
            }
        }
        glBindVertexArray(_quadVao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

void QVRVideoPlayer::playlistNext()
{
    if (_playlist->currentIndex() < _playlist->mediaCount() - 1)
        _playlist->next();
}

void QVRVideoPlayer::playlistPrevious()
{
    if (_playlist->currentIndex() > 0 && _player->position() < 5000)
        _playlist->previous();
    else
        _playlist->setCurrentIndex(_playlist->currentIndex());
}

void QVRVideoPlayer::seek(qint64 milliseconds)
{
    _player->setPosition(_player->position() + milliseconds);
}

void QVRVideoPlayer::togglePause()
{
    if (_player->state() == QMediaPlayer::PlayingState)
        _player->pause();
    else if (_player->state() == QMediaPlayer::PausedState)
        _player->play();
}

void QVRVideoPlayer::pause()
{
    if (_player->state() == QMediaPlayer::PlayingState)
        _player->pause();
}

void QVRVideoPlayer::play()
{
    if (_player->state() == QMediaPlayer::PausedState)
        _player->play();
}

void QVRVideoPlayer::toggleMute()
{
    _player->setMuted(!_player->isMuted());
}

void QVRVideoPlayer::changeVolume(int offset)
{
    _player->setVolume(_player->volume() + offset);
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
    case Qt::Key_P:
    case Qt::Key_MediaPrevious:
        playlistPrevious();
        break;
    case Qt::Key_N:
    case Qt::Key_MediaNext:
        playlistNext();
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
    QMediaPlaylist playlist;
    if (QVRManager::processIndex() == 0) {
        QCommandLineParser parser;
        parser.setApplicationDescription("QVR video player");
        parser.addHelpOption();
        parser.addVersionOption();
        parser.addPositionalArgument("video...", "Video file(s) to play.");
        parser.addOptions({
                { { "l", "loop" }, "Loop playlist." },
                { { "s", "screen" }, "Set screen geometry.", "screen" },
        });
        parser.process(app);
        QStringList posArgs = parser.positionalArguments();
        if (posArgs.length() == 0) {
            QFile file(":logo.mp4");
            QTemporaryFile* tmpFile = QTemporaryFile::createNativeFile(file);
            playlist.addMedia(QUrl::fromLocalFile(tmpFile->fileName()));
            playlist.setPlaybackMode(QMediaPlaylist::Loop);
        } else {
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
                qInfo("Adding to playlist: %s", qPrintable(url.toDisplayString()));
                playlist.addMedia(url);
            }
        }
        if (parser.isSet("loop")) {
            qInfo("Setting playlist to loop mode");
            playlist.setPlaybackMode(QMediaPlaylist::Loop);
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
    QVRVideoPlayer qvrapp(screen, &playlist);
    if (!manager.init(&qvrapp)) {
        qCritical("Cannot initialize QVR manager");
        return 1;
    }

    /* Enter the standard Qt loop */
    return app.exec();
}
