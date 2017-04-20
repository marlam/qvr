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

#ifndef SCENEVIEWER_HPP
#define SCENEVIEWER_HPP

#include <vector>

#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QMap>

#include <assimp/scene.h>


/* A scene description suitable for OpenGL-based rendering */

class Light {
public:
    enum {
        pointlight = 0,
        spotlight = 1,
        dirlight = 2
    } type;
    bool relative_to_camera;
    float position[3];
    float direction[3];
    float inner_cone_angle;
    float outer_cone_angle;
    float attenuation_constant;
    float attenuation_linear;
    float attenuation_quadratic;
    float ambient[3];
    float diffuse[3];
    float specular[3];

    Light();
};

class Material {
public:
    bool twosided;
    float ambient[3];
    float diffuse[3];
    float specular[3];
    float emissive[3];
    float shininess;
    float opacity;
    float bumpscaling;
    unsigned int ambient_tex;
    unsigned int diffuse_tex;
    unsigned int specular_tex;
    unsigned int emissive_tex;
    unsigned int shininess_tex;
    unsigned int lightness_tex;
    unsigned int opacity_tex;
    unsigned int bump_tex;
    unsigned int normal_tex;

    Material();
};

class Shape {
public:
    unsigned int material_index;
    unsigned int vao;
    unsigned int indices;

    Shape();
};

class Scene {
public:
    std::vector<Light> lights;
    std::vector<unsigned int> textures;
    std::vector<Material> materials;
    std::vector<Shape> shapes;

    Scene();
};

/* A viewer for the scene description defined above */

class SceneViewer : protected QOpenGLExtraFunctions
{
private:
    Scene _scene;
    bool _mipmapping;
    bool _anisotropicFiltering;
    bool _transparency;
    bool _lighting;
    bool _normalMapping;
    QOpenGLShaderProgram _prg;

    unsigned int createTex(
            const QString& baseDirectory,
            QMap<QString, unsigned int>& texturemap,
            const aiMaterial* m, aiTextureType t, unsigned int i, bool scalar);

public:
    SceneViewer();

    bool init(const aiScene* s, const QString& baseDirectory, const QMatrix4x4& transformationMatrix);
    void exit();

    void render(const QMatrix4x4& projectionMatrix, const QMatrix4x4& viewMatrix);

    bool mipmapping() const { return _mipmapping; }
    void setMipmapping(bool m) { _mipmapping = m; }
    bool anisotropicFiltering() const { return _anisotropicFiltering; }
    void setAnisotropicFiltering(bool a) { _anisotropicFiltering = a; }
    bool transparency() const { return _transparency; }
    void setTransparency(bool t) { _transparency = t; }
    bool lighting() const { return _lighting; }
    void setLighting(bool l) { _lighting = l; }
    bool normalMapping() const { return _normalMapping; }
    void setNormalMapping(bool n) { _normalMapping = n; }
};

#endif
