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

uniform sampler2D tex;

uniform bool edge_effect;
uniform bool ripple_effect;
uniform float ripple_offset;

smooth in vec2 vtexcoord;

layout(location = 0) out vec4 fcolor;

vec2 mirror_texcoords(vec2 tc)
{
    if (tc.x < 0.0)
        tc.x = -tc.x;
    else if (tc.x > 1.0)
        tc.x = 2.0 - tc.x;
    if (tc.y < 0.0)
        tc.y = -tc.y;
    else if (tc.y > 1.0)
        tc.y = 2.0 - tc.y;
    return tc;
}

void main(void)
{
    // 1. Vertical wave effect (rippling)
    vec2 tc = vtexcoord;
    if (ripple_effect) {
        tc.x += 0.005 * sin(ripple_offset + tc.y * 100.0);
    }
    vec3 color = texture(tex, mirror_texcoords(tc)).rgb;

    // 2. Sobel filter for edge detection
    if (edge_effect) {
        vec2 ts = vec2(textureSize(tex, 0));
        vec3 a = texture(tex, mirror_texcoords(tc + vec2(-1, -1) / ts)).rgb; // lower left texel
        vec3 b = texture(tex, mirror_texcoords(tc + vec2( 0, -1) / ts)).rgb; // lower texel
        vec3 c = texture(tex, mirror_texcoords(tc + vec2(+1, -1) / ts)).rgb; // lower right texel
        vec3 d = texture(tex, mirror_texcoords(tc + vec2(-1,  0) / ts)).rgb; // left texel
        vec3 f = texture(tex, mirror_texcoords(tc + vec2(+1,  0) / ts)).rgb; // right texel
        vec3 g = texture(tex, mirror_texcoords(tc + vec2(-1, +1) / ts)).rgb; // upper left texel
        vec3 h = texture(tex, mirror_texcoords(tc + vec2( 0, +1) / ts)).rgb; // upper texel
        vec3 i = texture(tex, mirror_texcoords(tc + vec2(+1, +1) / ts)).rgb; // upper right texel
        vec3 fx = (- 1.0 * a + 1.0 * c - 2.0 * d + 2.0 * f - 1.0 * g + 1.0 * i) / 4.0;
        vec3 fy = (+ 1.0 * a + 2.0 * b + 1.0 * c - 1.0 * g - 2.0 * h - 1.0 * i) / 4.0;
        float edge_strength = sqrt(dot(fx, fx) + dot(fy, fy));
        if (edge_strength > 0.4)
            color = vec3(1.0, 0.0, 0.0); // mark all edges red
    }

    fcolor = vec4(color, 1.0);
}
