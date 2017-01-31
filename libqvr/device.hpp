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
    QVector<bool> _buttons;
    QVector<float> _analogs;

    QVRDeviceInternals* _internals;

    friend QDataStream &operator<<(QDataStream& ds, const QVRDevice& d);
    friend QDataStream &operator>>(QDataStream& ds, QVRDevice& d);

public:
    /*! \brief Constructor. */
    QVRDevice();
    /*! \brief Constructor for the device with the given \a index in the QVR configuration. */
    QVRDevice(int index);
    /*! \brief Destructor. */
    ~QVRDevice();

    /*! \brief Assignment operator. */
    const QVRDevice& operator=(const QVRDevice& d);

    /*! \brief Returns the index of the device in the QVR configuration. */
    int index() const;
    /*! \brief Returns the unique id. */
    const QString& id() const;
    /*! \brief Returns the configuration. */
    const QVRDeviceConfig& config() const;

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

    /*! \brief Returns the number of digital buttons on this device. */
    int buttons() const
    {
        return _buttons.length();
    }

    /*! \brief Returns whether the button with the given index is pressed. */
    bool button(int index) const
    {
        return _buttons.at(index);
    }

    /*! \brief Returns the number of analog joystick elements on this device.
     * Usually index 0 will be the x axis and index 1 the y axis for two-dimensional
     * joysticks. */
    int analogs() const
    {
        return _analogs.length();
    }

    /*! \brief Returns the value of the analog joystick element with the given index. */
    float analog(int index) const
    {
        return _analogs.at(index);
    }

    /*! \cond
     * These functions are used internally to update the state of this device. */
    void update();
    /*! \endcond */
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
