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

#include <string>
#include <utility>
#include <map>
#include <limits>

#include <QDataStream>

#include "screen.hpp"

#include "tiny_obj_loader.h"


Screen::Screen(const QVector3D& bottomLeftCorner,
        const QVector3D& bottomRightCorner,
        const QVector3D& topLeftCorner)
{
    QVector3D topRightCorner = bottomRightCorner + (topLeftCorner - bottomLeftCorner);
    positions = {
        topLeftCorner.x(), topLeftCorner.y(), topLeftCorner.z(),
        topRightCorner.x(), topRightCorner.y(), topRightCorner.z(),
        bottomRightCorner.x(), bottomRightCorner.y(), bottomRightCorner.z(),
        bottomLeftCorner.x(), bottomLeftCorner.y(), bottomLeftCorner.z()
    };
    texCoords = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 0.0f
    };
    indices = { 0, 3, 1, 1, 3, 2 };
    float width = (bottomRightCorner - bottomLeftCorner).length();
    float height = (topLeftCorner - bottomLeftCorner).length();
    aspectRatio = width / height;
    isPlanar = true;
}

Screen::Screen(const QString& objFileName, float aspectRatio)
{
    std::string fileName(qPrintable(objFileName));
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string errMsg;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &errMsg, fileName.c_str(), NULL)) {
        if (!errMsg.empty() && errMsg.back() == '\n')
            errMsg.back() = '\0';
        qCritical("Failed to load %s: %s", fileName.c_str(), errMsg.c_str());
        return;
    }
    if (shapes.size() == 0) {
        qCritical("No shapes in %s", fileName.c_str());
        return;
    }

    bool ok = true;
    std::map<std::pair<int, int>, unsigned short> indexPairMap;
    for (size_t i = 0; ok && i < shapes.size(); i++) {
        for (size_t j = 0; ok && j < shapes[i].mesh.indices.size(); j++) {
            tinyobj::index_t index = shapes[i].mesh.indices[j];
            int vertexIndex = index.vertex_index;
            int texCoordIndex = index.texcoord_index;
            if (texCoordIndex < 0) {
                qCritical("Some shapes without texture coordinates in %s", fileName.c_str());
                ok = false;
                break;
            }
            std::pair<int, int> indexPair = std::make_pair(vertexIndex, texCoordIndex);
            std::map<std::pair<int, int>, unsigned short>::iterator it = indexPairMap.find(indexPair);
            if (it == indexPairMap.end()) {
                unsigned short newIndex = indexPairMap.size();
                if (newIndex == std::numeric_limits<unsigned short>::max()) {
                    qCritical("More vertices than I can handle in %s", fileName.c_str());
                    ok = false;
                    break;
                }
                positions.push_back(attrib.vertices[3 * vertexIndex + 0]);
                positions.push_back(attrib.vertices[3 * vertexIndex + 1]);
                positions.push_back(attrib.vertices[3 * vertexIndex + 2]);
                texCoords.push_back(attrib.texcoords[2 * texCoordIndex + 0]);
                texCoords.push_back(attrib.texcoords[2 * texCoordIndex + 1]);
                indices.push_back(newIndex);
                indexPairMap.insert(std::make_pair(indexPair, newIndex));
            } else {
                indices.push_back(it->second);
            }
        }
    }
    if (!ok) {
        positions.clear();
        texCoords.clear();
        indices.clear();
        return;
    }

    this->aspectRatio = aspectRatio;
    isPlanar = false;
}

QDataStream &operator<<(QDataStream& ds, const Screen& s)
{
    ds << s.positions << s.texCoords << s.indices << s.aspectRatio << s.isPlanar;
    return ds;
}

QDataStream &operator>>(QDataStream& ds, Screen& s)
{
    ds >> s.positions >> s.texCoords >> s.indices >> s.aspectRatio >> s.isPlanar;
    return ds;
}
