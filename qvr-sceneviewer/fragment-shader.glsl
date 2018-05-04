/*
 * Copyright (C) 2016, 2017, 2018 Computer Graphics Group, University of Siegen
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

const float pi = 3.14159265358979323846;

const int light_type_pointlight = 0;
const int light_type_spotlight = 1;
const int light_type_dirlight = 2;
const int light_sources = $LIGHT_SOURCES;

// Render parameters
uniform bool transparency;
uniform bool lighting;
uniform bool normalmapping;

// Light sources
uniform int light_type[light_sources];
uniform float light_inner_cone_angle[light_sources];
uniform float light_outer_cone_angle[light_sources];
uniform vec3 light_attenuation[light_sources];
uniform vec3 light_ambient[light_sources];
uniform vec3 light_diffuse[light_sources];
uniform vec3 light_specular[light_sources];

// Material
uniform bool material_twosided;
uniform vec3 material_ambient;
uniform vec3 material_diffuse;
uniform vec3 material_specular;
uniform vec3 material_emissive;
uniform float material_shininess;
uniform float material_opacity;
uniform float material_bumpscaling;
uniform bool material_have_ambient_tex;
uniform sampler2D material_ambient_tex;
uniform bool material_have_diffuse_tex;
uniform sampler2D material_diffuse_tex;
uniform bool material_have_specular_tex;
uniform sampler2D material_specular_tex;
uniform bool material_have_emissive_tex;
uniform sampler2D material_emissive_tex;
uniform bool material_have_shininess_tex;
uniform sampler2D material_shininess_tex;
uniform bool material_have_lightness_tex;
uniform sampler2D material_lightness_tex;
uniform bool material_have_opacity_tex;
uniform sampler2D material_opacity_tex;
uniform bool material_have_bump_tex;
uniform sampler2D material_bump_tex;
uniform bool material_have_normal_tex;
uniform sampler2D material_normal_tex;

smooth in vec3 vpos;
smooth in vec3 vnormal;
smooth in vec3 vlightpos[light_sources];
smooth in vec3 vlightdir[light_sources];
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
    // Basic color
    vec3 ambient_color = material_ambient;
    vec3 diffuse_color = material_diffuse;
    vec3 specular_color = material_specular;
    vec3 emissive_color = material_emissive;
    float shininess = material_shininess;
    float opacity = material_opacity;
    float lightness = 1.0;
    if (material_have_lightness_tex)
        lightness = texture(material_lightness_tex, vtexcoord).r;

    // Texture color
    if (material_have_ambient_tex) {
        vec4 t = texture(material_ambient_tex, vtexcoord);
        ambient_color *= t.rgb;
        if (!material_have_diffuse_tex && !material_have_opacity_tex)
            opacity *= t.a;
    }
    if (material_have_diffuse_tex) {
        vec4 t = texture(material_diffuse_tex, vtexcoord);
        diffuse_color *= t.rgb;
        if (!material_have_opacity_tex)
            opacity *= t.a;
    }
    if (material_have_specular_tex)
        specular_color *= texture(material_specular_tex, vtexcoord).rgb;
    if (material_have_emissive_tex)
        emissive_color = texture(material_emissive_tex, vtexcoord).rgb;
    if (material_have_shininess_tex)
        shininess *= texture(material_shininess_tex, vtexcoord).r;
    if (transparency && material_have_opacity_tex)
        opacity *= texture(material_opacity_tex, vtexcoord).r;

    // Transparency
    if (transparency && opacity < 0.5)
        discard;

    if (lighting) {
        // Lighting vectors
        vec3 normal = normalize(vnormal);
        if (normalmapping && (material_have_bump_tex || material_have_normal_tex)) {
            // Matrix to transform from eye space to tangent space
            mat3 TBN = cotangent_frame(normal, vpos, vtexcoord);
            if (material_have_normal_tex) {
                // Get normal from normal map
                normal = texture(material_normal_tex, vtexcoord).rgb;
                normal = normalize(2.0 * normal - 1.0);
            } else {
                // Get normal from bump map
                float bumpscaling = 8.0; // XXX this is an arbitrary base value, might be wrong!
                bumpscaling *= material_bumpscaling;
                float height_t = textureOffset(material_bump_tex, vtexcoord, ivec2(0, +1)).r;
                float height_b = textureOffset(material_bump_tex, vtexcoord, ivec2(0, -1)).r;
                float height_l = textureOffset(material_bump_tex, vtexcoord, ivec2(-1, 0)).r;
                float height_r = textureOffset(material_bump_tex, vtexcoord, ivec2(+1, 0)).r;
                vec3 tx = normalize(vec3(2.0, 0.0, bumpscaling * (height_r - height_l)));
                vec3 ty = normalize(vec3(0.0, 2.0, bumpscaling * (height_t - height_b)));
                normal = normalize(cross(tx, ty));
            }
            normal = TBN * normal;
        }
        if (material_twosided && !gl_FrontFacing)
            normal = -normal;
        vec3 view = normalize(-vpos);
        // Lighting
        vec3 ambient_light = vec3(0.0, 0.0, 0.0);
        vec3 diffuse_light = vec3(0.0, 0.0, 0.0);
        vec3 specular_light = vec3(0.0, 0.0, 0.0);
        for (int i = 0; i < light_sources; i++) {
            float attenuation = 1.0;
            if (light_type[i] != light_type_dirlight) {
                float dist = length(vpos - vlightpos[i]);
                attenuation = 1.0 / (light_attenuation[i].x
                        + light_attenuation[i].y * dist
                        + light_attenuation[i].z * dist * dist);
            }
            float spot = 1.0;
            if (light_type[i] == light_type_spotlight) {
                float angle = acos(dot(normalize(vlightdir[i]), normalize(vpos - vlightpos[i])));
                if (angle > light_outer_cone_angle[i]) {
                    spot = 0.0;
                } else if (angle > light_inner_cone_angle[i]) {
                    spot = cos(0.5 * pi * (angle - light_inner_cone_angle[i])
                            / (light_outer_cone_angle[i] - light_inner_cone_angle[i]));
                }
            }
            vec3 light;
            if (light_type[i] == light_type_dirlight)
                light = -vlightdir[i];
            else
                light = vlightpos[i] - vpos;
            light = normalize(light);
            vec3 halfway = normalize(light + view);
            ambient_light += attenuation * spot * light_ambient[i];
            float D = max(dot(light, normal), 0.0);
            diffuse_light += attenuation * spot * light_diffuse[i] * D;
            if (D > 0.0)
                specular_light += attenuation * spot * light_specular[i] * pow(max(dot(halfway, normal), 0.0), material_shininess);
        }
        ambient_color *= ambient_light;
        diffuse_color *= diffuse_light;
        specular_color *= specular_light;
    }

    // Resulting color at this fragment
    vec3 color = lightness * (emissive_color + ambient_color + diffuse_color + specular_color);

    fcolor = vec4(color, 1.0);
}
