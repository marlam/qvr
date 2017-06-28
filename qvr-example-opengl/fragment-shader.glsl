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

#define WITH_NORMAL_MAPS $WITH_NORMAL_MAPS
#define WITH_SPEC_MAPS   $WITH_SPEC_MAPS

uniform lowp vec3 material_color;
uniform lowp float material_kd;
uniform lowp float material_ks;
uniform lowp float material_shininess;
uniform bool material_has_diff_tex;
uniform sampler2D material_diff_tex;
#if WITH_NORMAL_MAPS
uniform bool material_has_norm_tex;
uniform sampler2D material_norm_tex;
#endif
#if WITH_SPEC_MAPS
uniform bool material_has_spec_tex;
uniform sampler2D material_spec_tex;
#endif
uniform mediump float material_tex_coord_factor;

smooth in mediump vec3 vnormal;
smooth in mediump vec3 vlight;
smooth in mediump vec3 vview;
smooth in mediump vec2 vtexcoord;

layout(location = 0) out vec4 fcolor;

#if WITH_NORMAL_MAPS
// This computation of a cotangent frame per fragment is taken from
// http://www.thetenthplanet.de/archives/1180
mediump mat3 cotangent_frame(mediump vec3 N, mediump vec3 p, mediump vec2 uv)
{
    // get edge vectors of the pixel triangle
    mediump vec3 dp1 = dFdx(p);
    mediump vec3 dp2 = dFdy(p);
    mediump vec2 duv1 = dFdx(uv);
    mediump vec2 duv2 = dFdy(uv);
    // solve the linear system
    mediump vec3 dp2perp = cross(dp2, N);
    mediump vec3 dp1perp = cross(N, dp1);
    mediump vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    mediump vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
    // construct a scale-invariant frame
    mediump float invmax = inversesqrt(max(dot(T,T), dot(B,B)));
    return mediump mat3(T * invmax, B * invmax, N);
}
#endif

void main(void)
{
    mediump vec2 tc = material_tex_coord_factor * vtexcoord;

    lowp vec3 color = material_color;
    if (material_has_diff_tex)
        color = texture(material_diff_tex, tc).rgb;

    lowp float kd = material_kd;
    lowp float ks = material_ks;
#if WITH_SPEC_MAPS
    if (material_has_spec_tex)
        ks = texture(material_spec_tex, tc).r;
#endif

    mediump vec3 normal = normalize(vnormal);
#if WITH_NORMAL_MAPS
    if (material_has_norm_tex) {
        mediump mat3 TBN = cotangent_frame(normal, -vview, tc);
        normal = texture(material_norm_tex, tc).rgb;
        normal.y = 1.0 - normal.y;
        normal = normalize(2.0 * normal - 1.0);
        normal = TBN * normal;
    }
#endif

    const lowp vec3 light_color = vec3(1.2);
    mediump vec3 light = normalize(vlight);
    mediump vec3 view = normalize(vview);
    mediump vec3 halfv = normalize(light + view);
    lowp vec3 ambient = vec3(0.2);
    lowp vec3 diffuse = kd * light_color * max(dot(light, normal), 0.0);
    lowp vec3 specular = ks * light_color * pow(max(dot(halfv, normal), 0.0), material_shininess);
    color *= ambient + diffuse + specular;

    fcolor = vec4(color, 1.0);
}
