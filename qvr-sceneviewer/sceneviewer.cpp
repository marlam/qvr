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

#include <QImage>
#include <QFile>
#include <QTextStream>
#include <QMap>

#include "sceneviewer.hpp"


Light::Light() :
    type(pointlight),
    relative_to_camera(true),
    position { 0.0f, 0.0f, 0.0f },
    direction { 0.0f, 0.0f, -1.0f },
    inner_cone_angle(0.0f),
    outer_cone_angle(0.0f),
    attenuation_constant(1.0f),
    attenuation_linear(0.0f),
    attenuation_quadratic(0.0f),
    ambient { 1.0f, 1.0f, 1.0f },
    diffuse { 1.0f, 1.0f, 1.0f },
    specular { 1.0f, 1.0f, 1.0f }
{
}

Material::Material() :
    twosided(false),
    ambient { 0.0f, 0.0f, 0.0f },
    diffuse { 0.5f, 0.5f, 0.5f },
    specular { 0.5f, 0.5f, 0.5f },
    emissive { 0.0f, 0.0f, 0.0f },
    shininess(100.0f),
    opacity(1.0f),
    bumpscaling(1.0f),
    ambient_tex(0),
    diffuse_tex(0),
    specular_tex(0),
    emissive_tex(0),
    shininess_tex(0),
    lightness_tex(0),
    opacity_tex(0),
    bump_tex(0),
    normal_tex(0)
{
}

Shape::Shape() :
    material_index(-1),
    vao(0),
    indices(0)
{
}

Scene::Scene() :
    lights(),
    textures(),
    materials(),
    shapes()
{
}

SceneViewer::SceneViewer() :
    _mipmapping(true),
    _anisotropicFiltering(true),
    _transparency(true),
    _lighting(true),
    _normalMapping(true)
{
}

unsigned int SceneViewer::createTex(
        const QString& baseDirectory,
        QMap<QString, unsigned int>& texturemap,
        const aiMaterial* m, aiTextureType t, unsigned int i, bool scalar)
{
    aiString path;
    aiTextureMapMode mapmode[3] = { aiTextureMapMode_Wrap, aiTextureMapMode_Wrap, aiTextureMapMode_Wrap };
    m->GetTexture(t, i, &path, NULL, NULL, NULL, NULL, mapmode);

    QString filename = baseDirectory + '/' + path.C_Str();
    filename.replace('\\', '/');

    unsigned int tex = 0;
    auto it = texturemap.find(filename);
    if (it != texturemap.end()) {
        tex = it.value();
    } else {
        QImage img;
        if (!img.load(filename)) {
            qWarning("Cannot load texture %s; ignoring it", qPrintable(filename));
        } else {
            // Using Qt bindTexture() does not work for some reason, maybe it's
            // because we use a core context. So we do it ourselves.
            img = img.mirrored(false, true);
            img = img.convertToFormat(QImage::Format_ARGB32);
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexImage2D(GL_TEXTURE_2D, 0, scalar ? GL_R8 : GL_RGBA8,
                    img.width(), img.height(), 0,
                    GL_BGRA, GL_UNSIGNED_BYTE, img.constBits());
            glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            switch (mapmode[0]) {
            case aiTextureMapMode_Wrap:
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                break;
            case aiTextureMapMode_Clamp:
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                break;
            case aiTextureMapMode_Decal:
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                break;
            case aiTextureMapMode_Mirror:
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
                break;
            default:
                break;
            }
        }
        texturemap.insert(filename, tex);
    }
    return tex;
}

static QString readFile(const QString& filename)
{
    QFile f(filename);
    f.open(QIODevice::ReadOnly);
    QTextStream in(&f);
    return in.readAll();
}

bool SceneViewer::init(const aiScene* s, const QString& baseDirectory, const QMatrix4x4& transformationMatrix)
{
    initializeOpenGLFunctions();

    /* Transform ASSIMP scene data to our OpenGL representation */

    QMap<QString, unsigned int> texturemap;
    for (unsigned int i = 0; i < s->mNumLights; i++) {
        Light light;
        const aiLight* l = s->mLights[i];
        switch (l->mType) {
        case aiLightSource_DIRECTIONAL:
            light.type = Light::dirlight;
            break;
        case aiLightSource_POINT:
            light.type = Light::pointlight;
            break;
        case aiLightSource_SPOT:
            light.type = Light::spotlight;
            break;
        default:
            qWarning("Ignoring light %u", i);
            continue;
        }
        QVector3D p = QVector3D(l->mPosition[0], l->mPosition[1], l->mPosition[2]);
        QVector3D tp = transformationMatrix * p;
        light.position[0] = tp.x();
        light.position[1] = tp.y();
        light.position[2] = tp.z();
        QVector3D d = QVector3D(l->mDirection[0], l->mDirection[1], l->mDirection[2]);
        QVector3D td = (transformationMatrix * QVector4D(d, 0.0f)).toVector3D();
        light.direction[0] = td.x();
        light.direction[1] = td.y();
        light.direction[2] = td.z();
        light.inner_cone_angle = l->mAngleInnerCone;
        light.outer_cone_angle = l->mAngleOuterCone;
        light.attenuation_constant = l->mAttenuationConstant;
        light.attenuation_linear = l->mAttenuationLinear;
        light.attenuation_quadratic = l->mAttenuationQuadratic;
        light.ambient[0] = l->mColorAmbient[0];
        light.ambient[1] = l->mColorAmbient[1];
        light.ambient[2] = l->mColorAmbient[2];
        light.diffuse[0] = l->mColorDiffuse[0];
        light.diffuse[1] = l->mColorDiffuse[1];
        light.diffuse[2] = l->mColorDiffuse[2];
        light.specular[0] = l->mColorSpecular[0];
        light.specular[1] = l->mColorSpecular[1];
        light.specular[2] = l->mColorSpecular[2];
        _scene.lights.push_back(light);
    }
    for (unsigned int i = 0; i < s->mNumMaterials; i++) {
        Material mat;
        const aiMaterial* m = s->mMaterials[i];
        int twosided = 0;
        m->Get(AI_MATKEY_TWOSIDED, twosided);
        mat.twosided = twosided;
        aiColor3D ambient(0.0f, 0.0f, 0.0f);
        m->Get(AI_MATKEY_COLOR_AMBIENT, ambient);
        mat.ambient[0] = ambient[0];
        mat.ambient[1] = ambient[1];
        mat.ambient[2] = ambient[2];
        aiColor3D diffuse(0.5f, 0.5f, 0.5f);
        m->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
        mat.diffuse[0] = diffuse[0];
        mat.diffuse[1] = diffuse[1];
        mat.diffuse[2] = diffuse[2];
        aiColor3D specular(0.5f, 0.5f, 0.5f);
        m->Get(AI_MATKEY_COLOR_SPECULAR, specular);
        mat.specular[0] = specular[0];
        mat.specular[1] = specular[1];
        mat.specular[2] = specular[2];
        aiColor3D emissive(0.0f, 0.0f, 0.0f);
        m->Get(AI_MATKEY_COLOR_EMISSIVE, emissive);
        mat.emissive[0] = emissive[0];
        mat.emissive[1] = emissive[1];
        mat.emissive[2] = emissive[2];
        float shininess = 100.0f;
        m->Get(AI_MATKEY_SHININESS, shininess);
        mat.shininess = shininess;
        float opacity = 1.0f;
        m->Get(AI_MATKEY_OPACITY, opacity);
        mat.opacity = opacity;
        float bumpscaling = 0.5f;
        m->Get(AI_MATKEY_BUMPSCALING, bumpscaling);
        mat.bumpscaling = bumpscaling;
        if (m->GetTextureCount(aiTextureType_AMBIENT) > 0)
            mat.ambient_tex = createTex(baseDirectory, texturemap, m, aiTextureType_AMBIENT, 0, false);
        if (m->GetTextureCount(aiTextureType_DIFFUSE) > 0)
            mat.diffuse_tex = createTex(baseDirectory, texturemap, m, aiTextureType_DIFFUSE, 0, false);
        if (m->GetTextureCount(aiTextureType_SPECULAR) > 0)
            mat.specular_tex = createTex(baseDirectory, texturemap, m, aiTextureType_SPECULAR, 0, false);
        if (m->GetTextureCount(aiTextureType_EMISSIVE) > 0)
            mat.emissive_tex = createTex(baseDirectory, texturemap, m, aiTextureType_EMISSIVE, 0, false);
        if (m->GetTextureCount(aiTextureType_SHININESS) > 0)
            mat.shininess_tex = createTex(baseDirectory, texturemap, m, aiTextureType_SHININESS, 0, true);
        if (m->GetTextureCount(aiTextureType_LIGHTMAP) > 0)
            mat.lightness_tex = createTex(baseDirectory, texturemap, m, aiTextureType_LIGHTMAP, 0, true);
        if (m->GetTextureCount(aiTextureType_OPACITY) > 0)
            mat.opacity_tex = createTex(baseDirectory, texturemap, m, aiTextureType_OPACITY, 0, true);
        if (m->GetTextureCount(aiTextureType_HEIGHT) > 0)
            mat.bump_tex = createTex(baseDirectory, texturemap, m, aiTextureType_HEIGHT, 0, true);
        if (m->GetTextureCount(aiTextureType_NORMALS) > 0)
            mat.normal_tex = createTex(baseDirectory, texturemap, m, aiTextureType_NORMALS, 0, false);
        _scene.materials.push_back(mat);
    }
    for (auto it = texturemap.begin(); it != texturemap.end(); it++) {
        _scene.textures.push_back(it.value());
    }
    QMatrix4x4 normalMatrix = QMatrix4x4(transformationMatrix.normalMatrix());
    for (unsigned int i = 0; i < s->mNumMeshes; i++) {
        Shape shape;
        const aiMesh* m = s->mMeshes[i];
        if (m->mPrimitiveTypes != aiPrimitiveType_TRIANGLE)
            continue;
        shape.material_index = m->mMaterialIndex;
        glGenVertexArrays(1, &(shape.vao));
        glBindVertexArray(shape.vao);
        GLuint position_buffer;
        glGenBuffers(1, &position_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, position_buffer);
        std::vector<float> positions(m->mNumVertices * 3);
        for (unsigned int j = 0; j < m->mNumVertices; j++) {
            QVector3D v = QVector3D(m->mVertices[j].x, m->mVertices[j].y, m->mVertices[j].z);
            QVector3D w = transformationMatrix * v;
            positions[3 * j + 0] = w.x();
            positions[3 * j + 1] = w.y();
            positions[3 * j + 2] = w.z();
        }
        glBufferData(GL_ARRAY_BUFFER, m->mNumVertices * 3 * sizeof(float), positions.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);
        GLuint normal_buffer;
        glGenBuffers(1, &normal_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
        std::vector<float> normals(m->mNumVertices * 3);
        for (unsigned int j = 0; j < m->mNumVertices; j++) {
            QVector3D v = QVector3D(m->mNormals[j].x, m->mNormals[j].y, m->mNormals[j].z);
            QVector3D w = normalMatrix * v;
            normals[3 * j + 0] = w.x();
            normals[3 * j + 1] = w.y();
            normals[3 * j + 2] = w.z();
        }
        glBufferData(GL_ARRAY_BUFFER, m->mNumVertices * 3 * sizeof(float), normals.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(1);
        GLuint texcoord_buffer;
        glGenBuffers(1, &texcoord_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, texcoord_buffer);
        std::vector<float> texcoords(m->mNumVertices * 2, 0.0f);
        if (m->mTextureCoords[0]) {
            for (unsigned int j = 0; j < m->mNumVertices; j++) {
                texcoords[2 * j + 0] = m->mTextureCoords[0][j][0];
                texcoords[2 * j + 1] = m->mTextureCoords[0][j][1];
            }
        }
        glBufferData(GL_ARRAY_BUFFER, m->mNumVertices * 2 * sizeof(float), texcoords.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(2);
        GLuint index_buffer;
        glGenBuffers(1, &index_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
        std::vector<unsigned int> indices(m->mNumFaces * 3);
        for (unsigned int j = 0; j < m->mNumFaces; j++) {
            indices[3 * j + 0] = m->mFaces[j].mIndices[0];
            indices[3 * j + 1] = m->mFaces[j].mIndices[1];
            indices[3 * j + 2] = m->mFaces[j].mIndices[2];
        }
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        shape.indices = m->mNumFaces * 3;
        _scene.shapes.push_back(shape);
    }

    /* Create default light source if there is none */
    if (_scene.lights.size() == 0) {
        qWarning("Adding default light to scene");
        Light defaultLight;
        _scene.lights.push_back(defaultLight);
    }

    /* Create the shader program for our scene representation */

    QString vs = readFile(":vertex-shader.glsl");
    QString fs = readFile(":fragment-shader.glsl");
    vs.replace("$LIGHT_SOURCES", QString::number(_scene.lights.size()));
    fs.replace("$LIGHT_SOURCES", QString::number(_scene.lights.size()));
    _prg.addShaderFromSourceCode(QOpenGLShader::Vertex, vs);
    _prg.addShaderFromSourceCode(QOpenGLShader::Fragment, fs);
    _prg.link();

    /* Set all uniforms that will not change between frames (i.e. light sources) */

    qInfo("Using %u light sources", static_cast<unsigned int>(_scene.lights.size()));
    glUseProgram(_prg.programId());
    QVector<int> tmp_int(_scene.lights.size());
    QVector<float> tmp_vec3(3 * _scene.lights.size());
    QVector<float> tmp_float(_scene.lights.size());
    for (size_t i = 0; i < _scene.lights.size(); i++)
        tmp_int[i] = _scene.lights[i].type;
    _prg.setUniformValueArray("light_type", tmp_int.data(), _scene.lights.size());
    for (size_t i = 0; i < _scene.lights.size(); i++)
        tmp_int[i] = _scene.lights[i].relative_to_camera ? 1 : 0;
    _prg.setUniformValueArray("light_relative_to_camera", tmp_int.data(), _scene.lights.size());
    for (size_t i = 0; i < _scene.lights.size(); i++)
        for (int j = 0; j < 3; j++)
            tmp_vec3[3 * i + j] = _scene.lights[i].position[j];
    _prg.setUniformValueArray("light_position", tmp_vec3.data(), _scene.lights.size(), 3);
    for (size_t i = 0; i < _scene.lights.size(); i++)
        for (int j = 0; j < 3; j++)
            tmp_vec3[3 * i + j] = _scene.lights[i].direction[j];
    _prg.setUniformValueArray("light_direction", tmp_vec3.data(), _scene.lights.size(), 3);
    for (size_t i = 0; i < _scene.lights.size(); i++)
        tmp_float[i] = _scene.lights[i].inner_cone_angle;
    _prg.setUniformValueArray("light_inner_cone_angle", tmp_float.data(), _scene.lights.size(), 1);
    for (size_t i = 0; i < _scene.lights.size(); i++)
        tmp_float[i] = _scene.lights[i].outer_cone_angle;
    _prg.setUniformValueArray("light_outer_cone_angle", tmp_float.data(), _scene.lights.size(), 1);
    for (size_t i = 0; i < _scene.lights.size(); i++) {
        tmp_vec3[3 * i + 0] = _scene.lights[i].attenuation_constant;
        tmp_vec3[3 * i + 1] = _scene.lights[i].attenuation_linear;
        tmp_vec3[3 * i + 2] = _scene.lights[i].attenuation_quadratic;
    }
    _prg.setUniformValueArray("light_attenuation", tmp_vec3.data(), _scene.lights.size(), 3);
    for (size_t i = 0; i < _scene.lights.size(); i++)
        for (int j = 0; j < 3; j++)
            tmp_vec3[3 * i + j] = _scene.lights[i].ambient[j];
    _prg.setUniformValueArray("light_ambient", tmp_vec3.data(), _scene.lights.size(), 3);
    for (size_t i = 0; i < _scene.lights.size(); i++)
        for (int j = 0; j < 3; j++)
            tmp_vec3[3 * i + j] = _scene.lights[i].diffuse[j];
    _prg.setUniformValueArray("light_diffuse", tmp_vec3.data(), _scene.lights.size(), 3);
    for (size_t i = 0; i < _scene.lights.size(); i++)
        for (int j = 0; j < 3; j++)
            tmp_vec3[3 * i + j] = _scene.lights[i].specular[j];
    _prg.setUniformValueArray("light_specular", tmp_vec3.data(), _scene.lights.size(), 3);

    return true;
}

void SceneViewer::render(const QMatrix4x4& projectionMatrix, const QMatrix4x4& viewMatrix)
{
    /* Set per-frame uniforms */
    glUseProgram(_prg.programId());
    _prg.setUniformValue("projection_modelview_matrix", projectionMatrix * viewMatrix);
    _prg.setUniformValue("modelview_matrix", viewMatrix);
    _prg.setUniformValue("normal_matrix", viewMatrix.transposed().inverted());
    _prg.setUniformValue("lighting", lighting() ? 1 : 0);
    _prg.setUniformValue("transparency", transparency() ? 1 : 0);
    _prg.setUniformValue("normalmapping", normalMapping() ? 1 : 0);

    for (size_t i = 0; i < _scene.shapes.size(); i++) {
        /* Set per-shape uniforms (i.e. material) */
        Material material;
        unsigned int m = _scene.shapes[i].material_index;
        if (m < _scene.materials.size())
            material = _scene.materials[m];
#if 0
        std::cerr << "Using material " << m << ":" << std::endl;
        std::cerr << "  twosided:      " << (material.twosided ? 1 : 0) << std::endl;
        std::cerr << "  ambient:       " << material.ambient[0] << " " << material.ambient[1] << " " << material.ambient[2] << std::endl;
        std::cerr << "  diffuse:       " << material.diffuse[0] << " " << material.diffuse[1] << " " << material.diffuse[2] << std::endl;
        std::cerr << "  specular:      " << material.specular[0] << " " << material.specular[1] << " " << material.specular[2] << std::endl;
        std::cerr << "  emissive:      " << material.emissive[0] << " " << material.emissive[1] << " " << material.emissive[2] << std::endl;
        std::cerr << "  shininess:     " << material.shininess << std::endl;
        std::cerr << "  opacity:       " << material.opacity << std::endl;
        std::cerr << "  bumpscaling:   " << material.bumpscaling << std::endl;
        std::cerr << "  ambient tex:   " << material.ambient_tex << std::endl;
        std::cerr << "  diffuse tex:   " << material.diffuse_tex << std::endl;
        std::cerr << "  specular tex:  " << material.specular_tex << std::endl;
        std::cerr << "  emissive tex:  " << material.emissive_tex << std::endl;
        std::cerr << "  shininess tex: " << material.shininess_tex << std::endl;
        std::cerr << "  lightness tex: " << material.lightness_tex << std::endl;
        std::cerr << "  opacity tex:   " << material.opacity_tex << std::endl;
        std::cerr << "  bump tex:      " << material.bump_tex << std::endl;
        std::cerr << "  normal tex:    " << material.normal_tex << std::endl;
#endif
        _prg.setUniformValue("material_twosided", material.twosided ? 1 : 0);
        _prg.setUniformValueArray("material_ambient", material.ambient, 1, 3);
        _prg.setUniformValueArray("material_diffuse", material.diffuse, 1, 3);
        _prg.setUniformValueArray("material_specular", material.specular, 1, 3);
        _prg.setUniformValueArray("material_emissive", material.emissive, 1, 3);
        _prg.setUniformValue("material_shininess", material.shininess);
        _prg.setUniformValue("material_opacity", material.opacity);
        _prg.setUniformValue("material_bumpscaling", material.bumpscaling);
        _prg.setUniformValue("material_have_ambient_tex", material.ambient_tex > 0 ? 1 : 0);
        _prg.setUniformValue("material_ambient_tex", 0);
        _prg.setUniformValue("material_have_diffuse_tex", material.diffuse_tex > 0 ? 1 : 0);
        _prg.setUniformValue("material_diffuse_tex", 1);
        _prg.setUniformValue("material_have_specular_tex", material.specular_tex > 0 ? 1 : 0);
        _prg.setUniformValue("material_specular_tex", 2);
        _prg.setUniformValue("material_have_emissive_tex", material.emissive_tex > 0 ? 1 : 0);
        _prg.setUniformValue("material_emissive_tex", 3);
        _prg.setUniformValue("material_have_shininess_tex", material.shininess_tex > 0 ? 1 : 0);
        _prg.setUniformValue("material_shininess_tex", 4);
        _prg.setUniformValue("material_have_lightness_tex", material.lightness_tex > 0 ? 1 : 0);
        _prg.setUniformValue("material_lightness_tex", 5);
        _prg.setUniformValue("material_have_opacity_tex", material.opacity_tex > 0 ? 1 : 0);
        _prg.setUniformValue("material_opacity_tex", 6);
        _prg.setUniformValue("material_have_bump_tex", material.bump_tex > 0 ? 1 : 0);
        _prg.setUniformValue("material_bump_tex", 7);
        _prg.setUniformValue("material_have_normal_tex", material.normal_tex > 0 ? 1 : 0);
        _prg.setUniformValue("material_normal_tex", 8);
        /* Bind textures */
        unsigned int textures[9] = { material.ambient_tex, material.diffuse_tex, material.specular_tex,
            material.emissive_tex, material.shininess_tex, material.lightness_tex, material.opacity_tex,
            material.bump_tex, material.normal_tex };
        for (int i = 0; i < 9; i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, textures[i]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    mipmapping() ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                    anisotropicFiltering() ? 4.0f : 1.0f);
        }
        /* Draw shape */
        glBindVertexArray(_scene.shapes[i].vao);
        glDrawElements(GL_TRIANGLES, _scene.shapes[i].indices, GL_UNSIGNED_INT, 0);
    }
}
