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

#include <QMatrix4x4>

#ifdef HAVE_VRPN
# include <vrpn_Tracker.h>
#endif

#include "config.hpp"

class QDataStream;

/*!
 * \brief Representation of an observer of the virtual world
 *
 * An observer typically has two eyes (left and right). This class additionally
 * handles an imaginary third eye at the center between left and right eye.
 *
 * Each eye has a position and an orientation in the virtual world. Both are
 * represented together in a transformation matrix.
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
    QMatrix4x4 _matrix[3];
#ifdef HAVE_VRPN
    vrpn_Tracker_Remote* _vrpnTracker;
#endif
    friend QDataStream &operator<<(QDataStream& ds, const QVRObserver& o);
    friend QDataStream &operator>>(QDataStream& ds, QVRObserver& o);

public:
    /*! \brief Constructor. */
    QVRObserver();
    /*! \brief Constructor for the observer with the given \a index in the QVR configuration. */
    QVRObserver(int index);
    /*! \brief Destructor. */
    ~QVRObserver();

    /*! \brief Returns the observer index in the QVR configuration. */
    int index() const;
    /*! \brief Returns the unique id. */
    const QString& id() const;
    /*! \brief Returns the configuration. */
    const QVRObserverConfig& config() const;

    /*! \brief Returns the transformation matrix of \a eye. */
    const QMatrix4x4& eyeMatrix(QVREye eye) const
    {
        return _matrix[eye];
    }

    /*! \brief Returns the position of \a eye (extracted from the transformation matrix). */
    QVector3D eyePosition(QVREye eye) const
    {
        return eyeMatrix(eye).column(3).toVector3D();
    }

    /*!
     * \brief Sets the transformation matrix for all three eyes.
     * @param centerMatrix      Transformation matrix for the center eye
     *
     * The matrices for the left and right eye are computed from \a centerMatrix
     * and the eye distance. See QObserverConfig::eyeDistance().
     *
     * This function is called internally by QVR except for custom observers,
     * which may be modified from \a QVRApp::updateObservers().
     */
    void setEyeMatrices(const QMatrix4x4& centerMatrix);

    /*!
     * \brief Sets the transformation matrix for all three eyes.
     * @param leftMatrix        Transformation matrix for the left eye
     * @param rightMatrix       Transformation matrix for the right eye
     *
     * The matrix for the center eye is computed from \a leftMatrix and \a rightMatrix.
     * For this purpose, it is assumed that both left and right eye have the same
     * orientation and only their position differs.
     *
     * This function is called internally by QVR except for custom observers,
     * which may be modified from \a QVRApp::updateObservers().
     */
    void setEyeMatrices(const QMatrix4x4& leftMatrix, const QMatrix4x4& rightMatrix);

    /*! \cond
     * This function is only used internally. It updates the eye matrices for
     * certain types of observers. Note that some types of observers are updated
     * from QVRManager or QVRWindow instead. */
    void update();
    /*! \endcond */
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
