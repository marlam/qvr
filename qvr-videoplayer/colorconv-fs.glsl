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

uniform sampler2D rgb_tex;
uniform sampler2D yuv_tex0;
uniform sampler2D yuv_tex1;
uniform sampler2D yuv_tex2;
uniform int shader_input;
uniform bool yuv_value_range_8bit_mpeg;
uniform bool yuv_709;

smooth in vec2 vtexcoord;

layout(location = 0) out vec4 fcolor;

float nonlinear_to_linear(float x)
{
    return (x <= 0.04045 ? (x / 12.92) : pow((x + 0.055) / 1.055, 2.4));
}

void main(void)
{
    // Get sRGB color data
    vec3 srgb;
    if (shader_input == 0) {
        srgb = texture(rgb_tex, vtexcoord).rgb;
    } else {
        // Get the YUV triplet
        vec3 yuv;
        if (shader_input == 1) {
            yuv = texture(yuv_tex0, vtexcoord).rgb;
        } else {
            yuv = vec3(texture(yuv_tex0, vtexcoord).r,
                    texture(yuv_tex1, vtexcoord).r,
                    texture(yuv_tex2, vtexcoord).r);
        }
        // Convert the MPEG range to the full range for each component, if necessary
        if (yuv_value_range_8bit_mpeg) {
            yuv = (yuv - vec3(16.0 / 255.0)) * vec3(256.0 / 220.0, 256.0 / 225.0, 256.0 / 225.0);
        }
        // Convert to sRGB
        if (yuv_709) {
            // According to ITU.BT-709 (see entries 3.2 and 3.3 in Sec. 3 ("Signal format"))
            mat3 m = mat3(
                    1.0,     1.0,      1.0,
                    0.0,    -0.187324, 1.8556,
                    1.5748, -0.468124, 0.0);
            srgb = m * (yuv - vec3(0.0, 0.5, 0.5));
        } else {
            // According to ITU.BT-601 (see formulas in Sec. 2.5.1 and 2.5.2)
            mat3 m = mat3(
                    1.0,    1.0,      1.0,
                    0.0,   -0.344136, 1.772,
                    1.402, -0.714136, 0.0);
            srgb = m * (yuv - vec3(0.0, 0.5, 0.5));
        }
    }

    // Convert sRGB to linear RGB
    vec3 linear_rgb = vec3(
            nonlinear_to_linear(srgb.r),
            nonlinear_to_linear(srgb.g),
            nonlinear_to_linear(srgb.b));

    fcolor = vec4(linear_rgb, 1.0);
}
