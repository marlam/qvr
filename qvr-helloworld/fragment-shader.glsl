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

uniform vec3 material_color;
uniform float material_kd;
uniform float material_ks;
uniform float material_shininess;
uniform bool material_has_diff_tex;
uniform sampler2D material_diff_tex;
uniform bool material_has_norm_tex;
uniform sampler2D material_norm_tex;
uniform bool material_has_spec_tex;
uniform sampler2D material_spec_tex;
uniform float material_tex_coord_factor;

smooth in vec3 vnormal;
smooth in vec3 vlight;
smooth in vec3 vview;
smooth in vec2 vtexcoord;

layout(location = 0) out vec4 fcolor;

// This computation of a cotangent frame per fragment is taken from
// http://www.thetenthplanet.de/archives/1180
mat3 cotangent_frame(vec3 N, vec3 p, vec2 uv)
{
    // get edge vectors of the pixel triangle
    vec3 dp1 = dFdx(p);
    vec3 dp2 = dFdy(p);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);
    // solve the linear system
    vec3 dp2perp = cross(dp2, N);
    vec3 dp1perp = cross(N, dp1);
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
    // construct a scale-invariant frame
    float invmax = inversesqrt(max(dot(T,T), dot(B,B)));
    return mat3(T * invmax, B * invmax, N);
}

void main(void)
{
    vec2 tc = material_tex_coord_factor * vtexcoord;

    vec3 color = material_color;
    if (material_has_diff_tex)
        color = texture(material_diff_tex, tc).rgb;

    float kd = material_kd;
    float ks = material_ks;
    if (material_has_spec_tex)
        ks = texture(material_spec_tex, tc).r;

    vec3 normal = normalize(vnormal);
    if (material_has_norm_tex) {
        mat3 TBN = cotangent_frame(normal, -vview, tc);
        normal = texture(material_norm_tex, tc).rgb;
        normal.y = 1.0 - normal.y;
        normal = normalize(2.0 * normal - 1.0);
        normal = TBN * normal;
    }

    const vec3 light_color = vec3(1.2);
    vec3 light = normalize(vlight);
    vec3 view = normalize(vview);
    vec3 halfv = normalize(light + view);
    vec3 ambient = vec3(0.2);
    vec3 diffuse = kd * light_color * max(dot(light, normal), 0.0);
    vec3 specular = ks * light_color * pow(max(dot(halfv, normal), 0.0), material_shininess);
    color *= ambient + diffuse + specular;

    fcolor = vec4(color, 1.0);
}
