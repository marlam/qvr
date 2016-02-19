/*
 * Copyright (C) 2016 Computer Graphics Group, University of Siegen
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

#ifndef QVR_CONFIG_HPP
#define QVR_CONFIG_HPP

#include <QString>
#include <QMatrix4x4>
#include <QPoint>
#include <QSize>
#include <QRect>
#include <QList>


typedef enum {
    QVR_Observer_Stationary,
    QVR_Observer_Oculus,
    QVR_Observer_Custom
} QVRObserverType;

typedef enum {
    QVR_Eye_Left = 0,
    QVR_Eye_Right = 1
} QVREye;

typedef enum {
    // these values are re-used in a GLSL shader; keep them in sync
    QVR_Output_Center = 0,
    QVR_Output_Left = 1,
    QVR_Output_Right = 2,
    QVR_Output_Stereo_GL = 3,
    QVR_Output_Stereo_Red_Cyan = 4,
    QVR_Output_Stereo_Green_Magenta = 5,
    QVR_Output_Stereo_Amber_Blue = 6,
    QVR_Output_Stereo_Oculus = 7,
    QVR_Output_Stereo_Custom = 8
} QVROutputMode;

class QVRObserverConfig
{
private:
    // Unique identification string
    QString _id;
    // Type of observer. If stationary, the transformation matrix will be
    // constant. Otherwise, it will be updated for each frame.
    QVRObserverType _type;
    // Initial position and orientation for the center between
    // left and right eye, and eye distance
    QVector3D _initialPosition;
    QVector3D _initialForwardDirection;
    QVector3D _initialUpDirection;
    float _initialEyeDistance;

    friend class QVRConfig;

public:
    // Default eye height: average total height minus offset to eye
    static constexpr float defaultEyeHeight = 1.76f - 0.15f;
    // Default eye distance: average value for humans
    static constexpr float defaultEyeDistance = 0.064f;

    QVRObserverConfig();

    const QString& id() const { return _id; }
    QVRObserverType type() const { return _type; }

    const QVector3D& initialPosition() const { return _initialPosition; }
    const QVector3D& initialForwardDirection() const { return _initialForwardDirection; }
    const QVector3D& initialUpDirection() const { return _initialUpDirection; }
    float initialEyeDistance() const { return _initialEyeDistance; }
    QMatrix4x4 initialEyeMatrix(QVREye eye) const;
};

class QVRWindowConfig
{
private:
    // Unique identification string
    QString _id;
    // The observer that gets to view this window.
    int _observerIndex;
    // Output mode.
    QVROutputMode _outputMode;
    // Output postprocessing.
    QString _outputPlugin;
    // Initial window configuration. This may change at runtime.
    int _initialDisplayScreen;
    bool _initialFullscreen;
    QPoint _initialPosition;
    QSize _initialSize;
    // Virtual world geometry of the screen area covered by this window.
    // This may change at runtime if the screen is fixed to the observer.
    bool _screenIsFixedToObserver;
    QVector3D _screenCornerBottomLeft;
    QVector3D _screenCornerBottomRight;
    QVector3D _screenCornerTopLeft;
    // This is an alternative to the above,
    // the actual dimensions will be taken from monitor specs
    bool _screenIsGivenByCenter;
    QVector3D _screenCenter;
    // Factor for optional lower-resolution rendering
    float _renderResolutionFactor;

    friend class QVRConfig;

public:
    QVRWindowConfig();

    const QString& id() const { return _id; }
    int observerIndex() const { return _observerIndex; }
    QVROutputMode outputMode() const { return _outputMode; }
    const QString& outputPlugin() const { return _outputPlugin; }
    int initialDisplayScreen() const { return _initialDisplayScreen; }
    bool initialFullscreen() const { return _initialFullscreen; }
    QPoint initialPosition() const { return _initialPosition; }
    QSize initialSize() const { return _initialSize; }
    QRect initialGeometry() const { return QRect(initialPosition(), initialSize()); }
    bool screenIsFixedToObserver() const { return _screenIsFixedToObserver; }
    const QVector3D& screenCornerBottomLeft() const { return _screenCornerBottomLeft; }
    const QVector3D& screenCornerBottomRight() const { return _screenCornerBottomRight; }
    const QVector3D& screenCornerTopLeft() const { return _screenCornerTopLeft; }
    bool screenIsGivenByCenter() const { return _screenIsGivenByCenter; }
    const QVector3D& screenCenter() const { return _screenCenter; }
    float renderResolutionFactor() const { return _renderResolutionFactor; }
};

class QVRProcessConfig
{
private:
    // Unique identification string
    QString _id;
    // The display that this process works on. Only relevant for X11 at this time.
    QString _display;
    // The launcher command, e.g. ssh
    QString _launcher;
    // The windows driven by this process.
    QList<QVRWindowConfig> _windowConfigs;

    friend class QVRConfig;

public:
    QVRProcessConfig();

    const QString& id() const { return _id; }
    const QString& display() const { return _display; }
    const QString& launcher() const { return _launcher; }
    const QList<QVRWindowConfig>& windowConfigs() const { return _windowConfigs; }
};

class QVRConfig
{
private:
    // The observers that get a view on the virtual world.
    QList<QVRObserverConfig> _observerConfigs;
    // The processes associated with this configuration.
    // The first process is always the one that is started first.
    // Each process config has a list of associated window configs.
    QList<QVRProcessConfig> _processConfigs;

public:
    QVRConfig();

    void createDefault();
    bool readFromFile(const QString& filename);

    const QList<QVRObserverConfig>& observerConfigs() const { return _observerConfigs; }
    const QList<QVRProcessConfig>& processConfigs() const { return _processConfigs; }
};

#endif
