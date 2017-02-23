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

#ifndef QVR_DEVICE_HPP
#define QVR_DEVICE_HPP

#include <QVector>
#include <QVector3D>
#include <QMatrix4x4>
#include <QQuaternion>

#include "config.hpp"

class QDataStream;

struct QVRDeviceInternals;

/*!
 * \brief Device for interaction purposes
 *
 * A device is anything a Virtual Reality application might use for interaction
 * purposes, e.g. a wand, a flystick, a 3DOF or 6DOF tracker target, or similar;
 * in short, anything that may be tracked, has buttons, and/or has analog
 * joystick functionality.
 *
 * For example, tracked glasses can be used to track an observer, and a game
 * controller can be used for navigation.
 *
 * A device is configured via \a QVRDeviceConfig.
 */
class QVRDevice
{
private:
    int _index;

    QVector3D _position;
    QQuaternion _orientation;
    QVector3D _velocity;
    QVector3D _angularVelocity;
    signed char _buttonsMap[32]; // the size fixes the maximum number of QVRButton values that can be represented
    QVector<bool> _buttons;
    signed char _analogsMap[16]; // the size fixes the maximum number of QVRAnalog values that can be represented
    QVector<float> _analogs;

    QVRDeviceInternals* _internals;

    friend QDataStream &operator<<(QDataStream& ds, const QVRDevice& d);
    friend QDataStream &operator>>(QDataStream& ds, QVRDevice& d);

    friend class QVRManager;
    void update();

public:
    /**
     * \name Constructor / Destructor
     */
    /*@{*/

    /*! \brief Constructor. */
    QVRDevice();
    /*! \brief Constructor for the device with the given \a index in the QVR configuration. */
    QVRDevice(int index);
    /*! \brief Destructor. */
    ~QVRDevice();

    /*! \brief Assignment operator. */
    const QVRDevice& operator=(const QVRDevice& d);

    /*@}*/

    /**
     * \name Configuration
     */
    /*@{*/

    /*! \brief Returns the index of the device in the QVR configuration. */
    int index() const;
    /*! \brief Returns the unique id. */
    const QString& id() const;
    /*! \brief Returns the configuration. */
    const QVRDeviceConfig& config() const;

    /*@}*/

    /**
     * \name Pose and velocity
     */
    /*@{*/

    /*! \brief Returns the position. */
    const QVector3D& position() const
    {
        return _position;
    }

    /*! \brief Returns the orientation. */
    const QQuaternion& orientation() const
    {
        return _orientation;
    }

    /*! \brief Returns the velocity, in m/s. */
    const QVector3D& velocity() const
    {
        return _velocity;
    }

    /*! \brief Returns the angular velocity. The direction of the returned vector
     * specifies the axis of rotation, and its length gives the rotation speed in
     * radians/s. */
    const QVector3D& angularVelocity() const
    {
        return _angularVelocity;
    }

    /*! \brief Returns the position and orientation as a matrix. */
    QMatrix4x4 matrix() const
    {
        QMatrix4x4 m;
        m.translate(position());
        m.rotate(orientation());
        return m;
    }

    /*@}*/

    /**
     * \name State of buttons and analog elements
     */
    /*@{*/

    /*! \brief Returns the number of buttons on this device. */
    int buttonCount() const
    {
        return _buttons.length();
    }

    /*! \brief Returns the index of the button \a btn, or -1 if this device does not have that button. */
    int buttonIndex(QVRButton btn) const
    {
        return _buttonsMap[btn];
    }

    /*! \brief Returns whether this device has button \a btn. */
    bool hasButton(QVRButton btn) const
    {
        return buttonIndex(btn) >= 0;
    }

    /*! \brief Returns whether the button with the given index is pressed. */
    bool button(int index) const
    {
        return (index >= 0 && index < buttonCount()) ? _buttons[index] : false;
    }

    /*! \brief Returns whether the button \a btn is pressed. Returns false if this device does not have that button. */
    bool button(QVRButton btn) const
    {
        return button(buttonIndex(btn));
    }

    /*! \brief Returns the number of analog joystick elements on this device.
     * Usually index 0 will be the x axis and index 1 the y axis for two-dimensional
     * joysticks. */
    int analogCount() const
    {
        return _analogs.length();
    }

    /*! \brief Returns the index of the analog element \a anlg, or -1 if this device does not have that element. */
    int analogIndex(QVRAnalog anlg) const
    {
        return _analogsMap[anlg];
    }

    /*! \brief Returns whether this device has analog element \a anlg. */
    bool hasAnalog(QVRAnalog anlg) const
    {
        return analogIndex(anlg) >= 0;
    }

    /*! \brief Returns the value of the analog joystick element with the given index. */
    float analog(int index) const
    {
        return (index >= 0 && index < analogCount()) ? _analogs[index] : 0.0f;
    }

    /*! \brief Returns the value of the analog element \a anlg. Returns 0 if this device does not have that element. */
    float analog(QVRAnalog anlg) const
    {
        return analog(analogIndex(anlg));
    }

    /*@}*/

    /**
     * \name Rendering a device representation
     *
     * A renderable representation of a device consists of one or more nodes.
     * Each node has its own transformation (position and orientation) and contains
     * vertex data (positions, normals, texture coordinates) and optionally a texture.
     *
     * You can query the number of nodes with \a modelNodeCount() and then render all nodes
     * sequentially to get the full device representation.
     *
     * Since the representation of the device depends on its state (buttons, analog elements, ...),
     * the number of nodes and their transformations may change between frames. However, vertex
     * data and textures with a given index are constant and can therefore be uploaded
     * to GPU buffers once and then reused.
     */
    /*@{*/

    /*! \brief Returns the number of nodes that are currently in the renderable device
     * model, or 0 if there is no such model.
     * This depends on the current state of the device and can therefore change between frames. */
    int modelNodeCount() const;

    /*! \brief Returns the current position of the renderable device model node with the given index.
     * This depends on the current state of the device and can therefore change between frames. */
    QVector3D modelNodePosition(int nodeIndex) const;

    /*! \brief Returns the current orientation of the renderable device model node with the given index.
     * This depends on the current state of the device and can therefore change between frames. */
    QQuaternion modelNodeOrientation(int nodeIndex) const;

    /*! \brief Returns the position and orientation of the renderable device model node with the given index as a matrix.
     * This depends on the current state of the device and can therefore change between frames. */
    QMatrix4x4 modelNodeMatrix(int nodeIndex) const
    {
        QMatrix4x4 m;
        m.translate(modelNodePosition(nodeIndex));
        m.rotate(modelNodeOrientation(nodeIndex));
        return m;
    }

    /*! \brief Returns the index of the vertex data block associated with the given renderable device
     * model node. Pass this index to \a QVRManager::deviceModelVertexCound(), \a QVRManager::deviceModelVertexPositions(),
     * \a QVRManager::deviceModelVertexNormals(), and \a QVRManager::deviceModelVertexTexCoords()
     * to get the actual vertex data.
     * Vertex data with a given index never changes, so you can e.g. upload this data to a GPU buffer once
     * and reuse it. */
    int modelNodeVertexDataIndex(int nodeIndex) const;

    /*! \brief Returns the index of the texture associated with the given renderable device model node.
     * Pass this index to \a QVRManager::deviceModelTexture() to get the actual texture data.
     * Texture data with a given index never changes, so you can e.g. upload this data to a GPU buffer once
     * and reuse it. */
    int modelNodeTextureIndex(int nodeIndex) const;

    /*@}*/

    /**
     * \name Haptic feedback
     *
     * Some devices, usually hand-held controllers, support haptic pulses.
     */
    /*@{*/

    /*! \brief Returns whether this device supports haptic pulses. */
    bool supportsHapticPulse() const;

    /*! \brief Triggers a haptic pulse with the given duration in microseconds.
     * Note that there may be a device-dependent limit on the duration; for now, avoid values
     * larger than 3999. */
    void triggerHapticPulse(int microseconds) const;

    /*@}*/
};

/*!
 * \brief Writes the device \a d to the stream \a ds.
 */
QDataStream &operator<<(QDataStream& ds, const QVRDevice& d);

/*!
 * \brief Reads the device \a d from the stream \a ds.
 */
QDataStream &operator>>(QDataStream& ds, QVRDevice& d);

#endif
