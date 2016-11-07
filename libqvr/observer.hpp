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

#ifndef QVR_OBSERVER_HPP
#define QVR_OBSERVER_HPP

#include <QVector3D>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QElapsedTimer>

#include "config.hpp"

class QDataStream;

/*!
 * \brief Observer of the virtual world
 *
 * Imagine your Virtual Reality setup is the cockpit of an UFO flying through
 * the virtual world. Inside the cockpit is a UFO crew member. This crew member
 * looks through the cockpit windows onto the virtual world. In QVR, this means
 * one or more windows (\a QVRWindow) are viewed by a \a QVRObserver.
 *
 * What a crew member will see in the cockpit windows depends on two things:
 * - The pose of the UFO in the virtual world. In QVR, this means an observer
 *   can navigate: a navigation transformation matrix will be applied to all
 *   the windows he views.
 * - The pose of the crew member, or more specifically the crew member's eyes,
 *   inside the UFO cockpit. In QVR, this is the tracking of an observer inside
 *   a restricted space. This determines the eye transformation matrices, which
 *   are relative to the navigation matrix.
 *
 * An observer typically has two eyes (left and right). This class additionally
 * handles an imaginary third eye at the center between left and right eye.
 * Each eye has a pose in the virtual world relative to the navigation pose.
 * All poses are represented as transformation matrices.
 *
 * A \a QVRWindow provides a view of the virtual world for exactly one observer.
 * An observer can view multiple windows, e.g. in multi-projector setups.
 *
 * An observer is configured via \a QVRObserverConfig.
 */
class QVRObserver
{
private:
    int _index;
    QVector3D _navigationPosition;
    QQuaternion _navigationOrientation;
    float _eyeDistance;
    QVector3D _trackingPosition[3];
    QQuaternion _trackingOrientation[3];

    friend QDataStream &operator<<(QDataStream& ds, const QVRObserver& o);
    friend QDataStream &operator>>(QDataStream& ds, QVRObserver& o);

public:
    /*! \brief Constructor. */
    QVRObserver();
    /*! \brief Constructor for the observer with the given \a index in the QVR configuration. */
    QVRObserver(int index);
    /*! \brief Destructor. */
    ~QVRObserver();

    /*! \brief Returns the index of the observer in the QVR configuration. */
    int index() const;
    /*! \brief Returns the unique id. */
    const QString& id() const;
    /*! \brief Returns the configuration. */
    const QVRObserverConfig& config() const;

    /*! \brief Returns the navigation position. */
    const QVector3D& navigationPosition() const
    {
        return _navigationPosition;
    }

    /*! \brief Returns the navigation orientation. */
    const QQuaternion& navigationOrientation() const
    {
        return _navigationOrientation;
    }

    /*! \brief Returns the navigation position and orientation as a matrix. */
    QMatrix4x4 navigationMatrix() const
    {
        QMatrix4x4 m;
        m.translate(navigationPosition());
        m.rotate(navigationOrientation());
        return m;
    }

    /*! \brief Returns the eye distance (interpupillary distance). */
    float eyeDistance() const
    {
        return _eyeDistance;
    }

    /*! \brief Returns the tracking position of \a eye. */
    const QVector3D& trackingPosition(QVREye eye = QVR_Eye_Center) const
    {
        return _trackingPosition[eye];
    }

    /*! \brief Returns the tracking orientation of \a eye. */
    const QQuaternion& trackingOrientation(QVREye eye = QVR_Eye_Center) const
    {
        return _trackingOrientation[eye];
    }

    /*! \brief Returns the tracking position and orientation of \a eye as a matrix. */
    QMatrix4x4 trackingMatrix(QVREye eye = QVR_Eye_Center) const
    {
        QMatrix4x4 m;
        m.translate(trackingPosition(eye));
        m.rotate(trackingOrientation(eye));
        return m;
    }

    /*!
     * \brief Sets the navigation position and orientation.
     * @param pos      Navigation position
     * @param rot      Navigation orientation
     *
     * This function is called internally by QVR except for custom observers,
     * which may be modified from \a QVRApp::update().
     */
    void setNavigation(const QVector3D& pos, const QQuaternion& rot)
    {
        _navigationPosition = pos;
        _navigationOrientation = rot;
    }

    /*! \brief Sets the eye distance (interpupillary distance) to \a d.
     *
     * Note that this only takes effect once \a setTracking() with arguments
     * for the center eye is called.
     *
     * A call to \a setTracking() with arguments for left and right eye will
     * set a new value for the eye distance.
     */
    void setEyeDistance(float d)
    {
        _eyeDistance = d;
    }

    /*!
     * \brief Sets the tracking position and orientation for all three eyes.
     * @param pos       Position of the center eye
     * @param rot       Orientation of the center eye
     *
     * The position and orientation for the left and right eye are computed
     * using \a eyeDistance().
     *
     * This function is called internally by QVR except for custom observers,
     * which may be modified from \a QVRApp::update().
     */
    void setTracking(const QVector3D& pos, const QQuaternion& rot);

    /*!
     * \brief Sets the tracking position and orientation for all three eyes.
     * @param posLeft   Position of the left eye
     * @param rotLeft   Orientation of the left eye
     * @param posRight  Position of the right eye
     * @param rotRight  Orientation of the right eye
     *
     * The position and orientation for the center eye and the eye distance is computed
     * from the information for the left and right eyes.
     *
     * This function is called internally by QVR except for custom observers,
     * which may be modified from \a QVRApp::update().
     */
    void setTracking(const QVector3D& posLeft, const QQuaternion& rotLeft, const QVector3D& posRight, const QQuaternion& rotRight);
};

/*!
 * \brief Writes the observer \a o to the stream \a ds.
 */
QDataStream &operator<<(QDataStream& ds, const QVRObserver& o);

/*!
 * \brief Reads the observer \a o from the stream \a ds.
 */
QDataStream &operator>>(QDataStream& ds, QVRObserver& o);

#endif
