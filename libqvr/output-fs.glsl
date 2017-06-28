/*
 * Copyright (C) 2016, 2017 Computer Graphics Group, University of Siegen
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

uniform sampler2D tex_l;
uniform sampler2D tex_r;

uniform int output_mode;
// same values as QVROutputMode enum:
#define QVR_Output_Center 0
#define QVR_Output_Left 1
#define QVR_Output_Right 2
#define QVR_Output_Stereo 3
#define QVR_Output_Red_Cyan 4
#define QVR_Output_Green_Magenta 5
#define QVR_Output_Amber_Blue 6

smooth in vec2 vtexcoord;

layout(location = 0) out vec4 fcolor;

// Source of this matrix: http://www.site.uottawa.ca/~edubois/anaglyph/LeastSquaresHowToPhotoshop.pdf
const lowp mat3 dubois_red_cyan_m0 = mat3(
        0.437, -0.062, -0.048,
        0.449, -0.062, -0.050,
        0.164, -0.024, -0.017);
const lowp mat3 dubois_red_cyan_m1 = mat3(
        -0.011, 0.377, -0.026,
        -0.032, 0.761, -0.093,
        -0.007, 0.009, 1.234);
// Source of this matrix: http://www.flickr.com/photos/e_dubois/5132528166/
const lowp mat3 dubois_green_magenta_m0 = mat3(
        -0.062, 0.284, -0.015,
        -0.158, 0.668, -0.027,
        -0.039, 0.143, 0.021);
const lowp mat3 dubois_green_magenta_m1 = mat3(
        0.529, -0.016, 0.009,
        0.705, -0.015, 0.075,
        0.024, -0.065, 0.937);
// Source of this matrix: http://www.flickr.com/photos/e_dubois/5230654930/
const lowp mat3 dubois_amber_blue_m0 = mat3(
        1.062, -0.026, -0.038,
        -0.205, 0.908, -0.173,
        0.299, 0.068, 0.022);
const lowp mat3 dubois_amber_blue_m1 = mat3(
        -0.016, 0.006, 0.094,
        -0.123, 0.062, 0.185,
        -0.017, -0.017, 0.911);

void main(void)
{
    lowp vec3 l, r;
    lowp vec3 color = vec3(0.8, 0.4, 0.2);
    switch (output_mode) {
    case QVR_Output_Red_Cyan:
    case QVR_Output_Green_Magenta:
    case QVR_Output_Amber_Blue:
        l = texture(tex_l, vtexcoord).rgb;
        r = texture(tex_r, vtexcoord).rgb;
        if (output_mode == QVR_Output_Red_Cyan)
            color = dubois_red_cyan_m0 * l + dubois_red_cyan_m1 * r;
        else if (output_mode == QVR_Output_Green_Magenta)
            color = dubois_green_magenta_m0 * l + dubois_green_magenta_m1 * r;
        else
            color = dubois_amber_blue_m0 * l + dubois_amber_blue_m1 * r;
        break;
    default:
        color = texture(tex_l, vtexcoord).rgb;
        break;
    }
    fcolor = vec4(color, 1.0);
}
