/*
 * Copyright (C) 2013, 2014, 2015, 2016, 2017
 * Computer Graphics Group, University of Siegen
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

#ifndef GEOMETRIES_HPP
#define GEOMETRIES_HPP

#include <vector>

/* These functions return basic geometries, scaled to fill [-1,+1]^3.
 * The arrays are cleared and geometry data is written to them. This
 * data is suitable for rendering with glDrawElements() in GL_TRIANGLES mode. */

void geom_quad(
        std::vector<float>& positions,
        std::vector<float>& normals,
        std::vector<float>& texcoords,
        std::vector<unsigned short>& indices);

void geom_cube(
        std::vector<float>& positions,
        std::vector<float>& normals,
        std::vector<float>& texcoords,
        std::vector<unsigned short>& indices);

void geom_disk(
        std::vector<float>& positions,
        std::vector<float>& normals,
        std::vector<float>& texcoords,
        std::vector<unsigned short>& indices,
        float inner_radius = 0.2f, int slices = 40);

void geom_sphere(
        std::vector<float>& positions,
        std::vector<float>& normals,
        std::vector<float>& texcoords,
        std::vector<unsigned short>& indices,
        int slices = 40, int stacks = 20);

void geom_cylinder(
        std::vector<float>& positions,
        std::vector<float>& normals,
        std::vector<float>& texcoords,
        std::vector<unsigned short>& indices,
        int slices = 40);

void geom_cone(
        std::vector<float>& positions,
        std::vector<float>& normals,
        std::vector<float>& texcoords,
        std::vector<unsigned short>& indices,
        int slices = 40, int stacks = 20);

void geom_torus(
        std::vector<float>& positions,
        std::vector<float>& normals,
        std::vector<float>& texcoords,
        std::vector<unsigned short>& indices,
        float inner_radius = 0.4f, int sides = 40, int rings = 40);

void geom_teapot(
        std::vector<float>& positions,
        std::vector<float>& normals,
        std::vector<float>& texcoords,
        std::vector<unsigned short>& indices);

#endif
