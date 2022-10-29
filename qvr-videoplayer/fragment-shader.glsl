/*
 * Copyright (C) 2017, 2022 Computer Graphics Group, University of Siegen
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>
 * Copyright (C) 2022 Martin Lambers <marlam@marlam.de>
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

uniform sampler2D tex;
uniform float relative_width;
uniform float relative_height;
uniform float view_offset_x;
uniform float view_factor_x;
uniform float view_offset_y;
uniform float view_factor_y;

smooth in vec2 vtexcoord;

layout(location = 0) out vec4 fcolor;

void main(void)
{
    float vtx = view_offset_x + view_factor_x * vtexcoord.x;
    float vty = view_offset_y + view_factor_y * vtexcoord.y;
    float tx = (      vtx - 0.5 * (1.0 - relative_width )) / relative_width;
    float ty = (1.0 - vty - 0.5 * (1.0 - relative_height)) / relative_height;
    vec3 rgb = texture(tex, vec2(tx, ty)).bgr;
    fcolor = vec4(rgb, 1.0);
}
