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

#ifndef SCREEN_HPP
#define SCREEN_HPP

#include <QVector>
#include <QVector3D>

class Screen
{
public:
    QVector<float> positions; // each position consists of 3 floats
    QVector<float> texCoords; // each texCoord consists of 2 floats
    QVector<unsigned short> indices;
    float aspectRatio;
    bool isPlanar;

    Screen() : aspectRatio(0.0f), isPlanar(false) {}

    // Construct a planar screen.
    // The aspect ratio is inferred from the screen corners.
    Screen(const QVector3D& bottomLeftCorner,
            const QVector3D& bottomRightCorner,
            const QVector3D& topLeftCorner);

    // Construct a screen by reading the specified OBJ file.
    // Since the aspect ratio cannot be inferred, it has to be specified.
    // The OBJ data must contain positions and texture coordinates;
    // everything else is ignored. If indices.size() == 0
    // after constructing the screen in this way, then loading
    // the OBJ file failed.
    Screen(const QString& objFileName, float aspectRatio);
};

QDataStream &operator<<(QDataStream& ds, const Screen& f);
QDataStream &operator>>(QDataStream& ds, Screen& f);

#endif
