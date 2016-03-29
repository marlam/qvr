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

#ifndef QVR_FRUSTUM_HPP
#define QVR_FRUSTUM_HPP

#include <QMatrix4x4>

class QDataStream;

/*!
 * \brief Frustum class.
 *
 * This class represents a view frustum.
 */
class QVRFrustum
{
private:
    float _lrbtnf[6];

    friend QDataStream &operator<<(QDataStream& ds, const QVRFrustum& f);
    friend QDataStream &operator>>(QDataStream& ds, QVRFrustum& f);

public:
    /*!
     * \brief Constructs an invalid frustum, where all values are zero.
     */
    QVRFrustum();

    /*!
     * \brief Constructs a view frustum.
     * \param l         Left clipping plane
     * \param r         Right clipping plane
     * \param b         Bottom clipping plane
     * \param t         Top clipping plane
     * \param n         Near clipping plane
     * \param f         Far clipping plane
     */
    QVRFrustum(float l, float r, float b, float t, float n, float f);

    /*!
     * \brief Constructs a view frustum.
     * \param lrbtnf    Left, right, bottom, top, near, and far clipping plane
     */
    QVRFrustum(const float* lrbtnf);

    /*!
     * \brief Returns the clipping plane values \a l, \a r, \a b, \a t, \a n, \a f.
     */
    void getClippingPlanes(float* l, float* r, float* b, float* t, float* n, float* f) const
    {
        *l = leftPlane();
        *r = rightPlane();
        *b = bottomPlane();
        *t = topPlane();
        *n = nearPlane();
        *f = farPlane();
    }

    /*!
     * \brief Returns the clipping plane values l, r, b, t, n, f in an array \a lrbtnf.
     */
    void getClippingPlanes(float* lrbtnf) const
    {
        lrbtnf[0] = leftPlane();
        lrbtnf[1] = rightPlane();
        lrbtnf[2] = bottomPlane();
        lrbtnf[3] = topPlane();
        lrbtnf[4] = nearPlane();
        lrbtnf[5] = farPlane();
    }

    /*!
     * \brief Creates a 4x4 matrix from this frustum
     * \return The matrix
     */
    QMatrix4x4 toMatrix4x4() const;

    /*!
     * \brief Adjusts the near plane while preserving the frustum shape.
     * \param n         The new near clipping plane
     */
    void adjustNearPlane(float n);

    /*!
     * \brief Returns the left clipping plane.
     * \return The left clipping plane
     */
    float leftPlane() const   { return _lrbtnf[0]; }

    /*!
     * \brief Returns the right clipping plane.
     * \return The right clipping plane
     */
    float rightPlane() const  { return _lrbtnf[1]; }

    /*!
     * \brief Returns the bottom clipping plane.
     * \return The bottom clipping plane
     */
    float bottomPlane() const { return _lrbtnf[2]; }

    /*!
     * \brief Returns the top clipping plane.
     * \return The top clipping plane
     */
    float topPlane() const    { return _lrbtnf[3]; }

    /*!
     * \brief Returns the near clipping plane.
     * \return The near clipping plane
     */
    float nearPlane() const   { return _lrbtnf[4]; }

    /*!
     * \brief Returns the far clipping plane.
     * \return The far clipping plane
     */
    float farPlane() const    { return _lrbtnf[5]; }

    /*!
     * \brief Sets the left clipping plane.
     * \param l         The left clipping plane
     */
    void setLeftPlane(float l)   { _lrbtnf[0] = l; }

    /*!
     * \brief Sets the right clipping plane.
     * \param r         The right clipping plane
     */
    void setRightPlane(float r)  { _lrbtnf[1] = r; }

    /*!
     * \brief Sets the bottom clipping plane.
     * \param b         The bottom clipping plane
     */
    void setBottomPlane(float b) { _lrbtnf[2] = b; }

    /*!
     * \brief Sets the top clipping plane.
     * \param t         The top clipping plane
     */
    void setTopPlane(float t)    { _lrbtnf[3] = t; }

    /*!
     * \brief Sets the near clipping plane.
     * \param n         The near clipping plane
     */
    void setNearPlane(float n)   { _lrbtnf[4] = n; }

    /*!
     * \brief Sets the far clipping plane.
     * \param f         The far clipping plane
     */
    void setFarPlane(float f)    { _lrbtnf[5] = f; }
};

/*!
 * \brief Writes the frustum \a f to the stream \a ds.
 */
QDataStream &operator<<(QDataStream& ds, const QVRFrustum& f);

/*!
 * \brief Reads the frustum \a f from the stream \a ds.
 */
QDataStream &operator>>(QDataStream& ds, QVRFrustum& f);

#endif
