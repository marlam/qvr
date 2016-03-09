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

#version 330

const int light_type_pointlight = 0;
const int light_type_spotlight = 1;
const int light_type_dirlight = 2;
const int light_sources = $LIGHT_SOURCES;

uniform mat4 projection_modelview_matrix;
uniform mat4 modelview_matrix;
uniform mat4 normal_matrix;

uniform int light_type[light_sources];
uniform bool light_relative_to_camera[light_sources];
uniform vec3 light_position[light_sources];
uniform vec3 light_direction[light_sources];

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;

smooth out vec3 vpos;                            // position in eye space
smooth out vec3 vnormal;                         // normal vector in eye space
smooth out vec3 vlightpos[light_sources];        // light position in eye space
smooth out vec3 vlightdir[light_sources];        // light direction in eye space
smooth out vec3 vview;                           // view vector in eye space
smooth out vec2 vtexcoord;

void main(void)
{
    vec3 pos = (modelview_matrix * position).xyz;

    vpos = pos;
    vnormal = mat3(normal_matrix) * normal;
    vtexcoord = texcoord;

    for (int i = 0; i < light_sources; i++) {
        vec3 lp = light_position[i];
        vec3 ld = light_direction[i];
        if (!light_relative_to_camera[i]) {
            lp = (modelview_matrix * vec4(lp, 1.0)).xyz;
            ld = mat3(modelview_matrix) * ld;
        }
        vlightpos[i] = lp;
        vlightdir[i] = ld;
    }

    gl_Position = projection_modelview_matrix * position;
}
