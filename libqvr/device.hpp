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
    signed char _buttonsMap[QVR_Button_Unknown];
    QVector<bool> _buttons;
    signed char _analogsMap[QVR_Analog_Unknown];
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
    /*! \brief Copy constructor. */
    QVRDevice(const QVRDevice& d);
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

    /*! \brief Returns the type of the button with \a index. */
    QVRButton button(int index) const
    {
        QVRButton btn = QVR_Button_Unknown;
        if (index >= 0) {
            for (int i = 0; i < QVR_Button_Unknown; i++) {
                if (_buttonsMap[i] == index) {
                    btn = static_cast<QVRButton>(i);
                    break;
                }
            }
        }
        return btn;
    }

    /*! \brief Returns the index of the button \a btn, or -1 if this device does not have that button. */
    int buttonIndex(QVRButton btn) const
    {
        return (btn == QVR_Button_Unknown ? -1 : _buttonsMap[btn]);
    }

    /*! \brief Returns whether this device has button \a btn. */
    bool hasButton(QVRButton btn) const
    {
        return buttonIndex(btn) >= 0;
    }

    /*! \brief Returns whether the button with the given index is pressed. */
    bool isButtonPressed(int index) const
    {
        return (index >= 0 && index < buttonCount()) ? _buttons[index] : false;
    }

    /*! \brief Returns whether the button \a btn is pressed. Returns false if this device does not have that button. */
    bool isButtonPressed(QVRButton btn) const
    {
        return isButtonPressed(buttonIndex(btn));
    }

    /*! \brief Returns the number of analog joystick elements on this device.
     * Usually index 0 will be the x axis and index 1 the y axis for two-dimensional
     * joysticks. */
    int analogCount() const
    {
        return _analogs.length();
    }

    /*! \brief Returns the type of the analog element with \a index. */
    QVRAnalog analog(int index) const
    {
        QVRAnalog anlg = QVR_Analog_Unknown;
        if (index >= 0) {
            for (int i = 0; i < QVR_Analog_Unknown; i++) {
                if (_analogsMap[i] == index) {
                    anlg = static_cast<QVRAnalog>(i);
                    break;
                }
            }
        }
        return anlg;
    }

    /*! \brief Returns the index of the analog element \a anlg, or -1 if this device does not have that element. */
    int analogIndex(QVRAnalog anlg) const
    {
        return (anlg == QVR_Analog_Unknown ? -1 : _analogsMap[anlg]);
    }

    /*! \brief Returns whether this device has analog element \a anlg. */
    bool hasAnalog(QVRAnalog anlg) const
    {
        return analogIndex(anlg) >= 0;
    }

    /*! \brief Returns the value of the analog joystick element with the given index.
     * This is either in [-1,1] (for axis-type analog elements) or in [0,1] (for trigger-type analog elements). */
    float analogValue(int index) const
    {
        return (index >= 0 && index < analogCount()) ? _analogs[index] : 0.0f;
    }

    /*! \brief Returns the value of the analog element \a anlg. Returns 0 if this device does not have that element. */
    float analogValue(QVRAnalog anlg) const
    {
        return analogValue(analogIndex(anlg));
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

/*!
 * \brief Device event
 *
 * Two approaches are common to react on device usage:
 * - Query the current state of devices in QVRApp::update()
 * - Receive device events in QVRApp::deviceButtonPressEvent(),
 *   QVRApp::deviceButtonReleaseEvent(),
 *   and QVRApp::deviceAnalogChangeEvent()
 *
 * This device event class is for the second use case.
 *
 * The difference between the two is this: If a user presses a digital button on
 * a device, than it will be pressed for a certain amount of time, and therefore
 * the device will report that button as pressed for multiple consecutive frames
 * when queried from QVRApp::update(). This is useful if the application
 * implements something like "move forward as long as button X is pressed".
 * On the other hand, if the application implements something like "when button
 * X is pressed, trigger action Y", then it wants the action to be triggered only
 * once. In other words, it wants to react on the event "button X was pressed".
 */
class QVRDeviceEvent
{
private:
    QVRDevice _device;
    int _buttonIndex;
    int _analogIndex;

public:
    /* Construct a device event. Only used internally. */
    QVRDeviceEvent(const QVRDevice& device, int buttonIndex, int analogIndex) :
        _device(device), _buttonIndex(buttonIndex), _analogIndex(analogIndex)
    {
    }

    /*! \brief Returns the device state at the time this event was generated.
     * You can use this to identify the device that caused the event, and to
     * inspect other button and analog element states besides the one that
     * generated the event. */
    const QVRDevice& device() const
    {
        return _device;
    }

    /*! \brief Returns the button that triggered the event.
     * Only calid for button press and button release events.
     * This is a convenience function, you can get the same information from \a device().*/
    QVRButton button() const
    {
        return _device.button(buttonIndex());
    }

    /*! \brief Returns the index of the button that triggered the event.
     * Only valid for button press and button release events. */
    int buttonIndex() const
    {
        return _buttonIndex;
    }

    /*! \brief Returns the analog element that triggered the event.
     * Only valid for analog change events.
     * This is a convenience function, you can get the same information from \a device().*/
    QVRAnalog analog() const
    {
        return _device.analog(analogIndex());
    }

    /*! \brief Returns the index of the analog element that triggered the event.
     * Only valid for analog change events. */
    int analogIndex() const
    {
        return _analogIndex;
    }
};

#endif
