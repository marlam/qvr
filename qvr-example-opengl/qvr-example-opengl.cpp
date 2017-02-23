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

#include <QApplication>
#include <QKeyEvent>
#include <QImage>

#include <qvr/manager.hpp>
#include <qvr/window.hpp>
#include <qvr/device.hpp>

#include "qvr-example-opengl.hpp"

#include "geometries.hpp"


QVRExampleOpenGL::QVRExampleOpenGL() :
    _wantExit(false),
    _objectRotationAngle(0.0f)
{
    _timer.start();
}

unsigned int QVRExampleOpenGL::setupTex(const QString& filename)
{
    QImage img;
    img.load(filename);
    img = img.mirrored(false, true);
    img = img.convertToFormat(QImage::Format_RGBA8888);
    return setupTex(img);
}

unsigned int QVRExampleOpenGL::setupTex(const QImage& img)
{
    unsigned int tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
            img.width(), img.height(), 0,
            GL_RGBA, GL_UNSIGNED_BYTE, img.constBits());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4.0f);
    return tex;
}

unsigned int QVRExampleOpenGL::setupVao(int vertexCount,
        const float* positions, const float* normals, const float* texcoords,
        int indexCount, const unsigned short* indices)
{
    GLuint vao;
    GLuint positionBuf, normalBuf, texcoordBuf, indexBuf;

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &positionBuf);
    glBindBuffer(GL_ARRAY_BUFFER, positionBuf);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * 3 * sizeof(float), positions, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    glGenBuffers(1, &normalBuf);
    glBindBuffer(GL_ARRAY_BUFFER, normalBuf);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * 3 * sizeof(float), normals, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);
    glGenBuffers(1, &texcoordBuf);
    glBindBuffer(GL_ARRAY_BUFFER, texcoordBuf);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * 2 * sizeof(float), texcoords, GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(2);
    glGenBuffers(1, &indexBuf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned short), indices, GL_STATIC_DRAW);
    return vao;
}

void QVRExampleOpenGL::setMaterial(const Material& m)
{
    _prg.setUniformValue("material_color", m.r, m.g, m.b);
    _prg.setUniformValue("material_kd", m.kd);
    _prg.setUniformValue("material_ks", m.ks);
    _prg.setUniformValue("material_shininess", m.shininess);
    _prg.setUniformValue("material_has_diff_tex", m.diffTex == 0 ? 0 : 1);
    _prg.setUniformValue("material_diff_tex", 0);
    _prg.setUniformValue("material_has_norm_tex", m.normTex == 0 ? 0 : 1);
    _prg.setUniformValue("material_norm_tex", 1);
    _prg.setUniformValue("material_has_spec_tex", m.specTex == 0 ? 0 : 1);
    _prg.setUniformValue("material_spec_tex", 2);
    _prg.setUniformValue("material_tex_coord_factor", m.texCoordFactor);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m.diffTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m.normTex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m.specTex);
}

void QVRExampleOpenGL::renderVao(
        const QMatrix4x4& viewMatrix, const QMatrix4x4& modelMatrix,
        unsigned int vao, unsigned int indices)
{
    QMatrix4x4 modelViewMatrix = viewMatrix * modelMatrix;
    _prg.setUniformValue("model_view_matrix", modelViewMatrix);
    _prg.setUniformValue("normal_matrix", modelViewMatrix.normalMatrix());
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indices, GL_UNSIGNED_SHORT, 0);
}

void QVRExampleOpenGL::serializeDynamicData(QDataStream& ds) const
{
    ds << _objectRotationAngle;
}

void QVRExampleOpenGL::deserializeDynamicData(QDataStream& ds)
{
    ds >> _objectRotationAngle;
}

void QVRExampleOpenGL::update(const QList<QVRObserver*>&)
{
    float seconds = _timer.elapsed() / 1000.0f;
    _objectRotationAngle = seconds * 20.0f;

    // Trigger a haptic pulse on devices that support it
    for (int i = 0; i < QVRManager::deviceCount(); i++) {
        const QVRDevice& device = QVRManager::device(i);
        if (device.supportsHapticPulse() && device.hasAnalog(QVR_Analog_Trigger)) {
            int microseconds = device.analog(QVR_Analog_Trigger) * 3999;
            device.triggerHapticPulse(microseconds);
        }
    }
}

bool QVRExampleOpenGL::wantExit()
{
    return _wantExit;
}

bool QVRExampleOpenGL::initProcess(QVRProcess* /* p */)
{
    /* Initialize per-process OpenGL resources and state here */
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> texcoords;
    std::vector<unsigned short> indices;

    // Qt-based OpenGL function pointers
    initializeOpenGLFunctions();

    // FBO
    glGenFramebuffers(1, &_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    glGenTextures(1, &_fboDepthTex);
    glBindTexture(GL_TEXTURE_2D, _fboDepthTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 1, 1,
            0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _fboDepthTex, 0);

    // Floor
    geom_quad(positions, normals, texcoords, indices);
    _floorVao = setupVao(positions.size() / 3, positions.data(), normals.data(), texcoords.data(),
            indices.size(), indices.data());
    _floorIndices = indices.size();
    _floorMaterial = Material(0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
            setupTex(":floor-diff.jpg"), setupTex(":floor-norm.jpg"), 0, 10.0f);

    // Pillar
    geom_cylinder(positions, normals, texcoords, indices);
    _pillarVaos[0] = setupVao(positions.size() / 3, positions.data(), normals.data(), texcoords.data(),
            indices.size(), indices.data());
    _pillarIndices[0] = indices.size();
    geom_disk(positions, normals, texcoords, indices, 0.0f);
    _pillarVaos[1] = setupVao(positions.size() / 3, positions.data(), normals.data(), texcoords.data(),
            indices.size(), indices.data());
    _pillarIndices[1] = indices.size();
    _pillarMaterial = Material(0.5f, 0.5f, 0.3f, 0.5f, 0.5f, 100.0f,
            setupTex(":pillar-diff.jpg"), setupTex(":pillar-norm.jpg"), setupTex(":pillar-spec.jpg"));

    // Object
    geom_cube(positions, normals, texcoords, indices);
    _objectVaos[0] = setupVao(positions.size() / 3, positions.data(), normals.data(), texcoords.data(),
            indices.size(), indices.data());
    _objectIndices[0] = indices.size();
    _objectMaterials[0] = Material(0.8f, 0.3f, 0.3f, 0.8f, 0.2f, 20.0f);
    _objectMatrices[0].rotate(15.0f, 1.0f, 1.0f, 0.0f);
    _objectMatrices[0].scale(0.5f);
    geom_cone(positions, normals, texcoords, indices);
    _objectVaos[1] = setupVao(positions.size() / 3, positions.data(), normals.data(), texcoords.data(),
            indices.size(), indices.data());
    _objectIndices[1] = indices.size();
    _objectMaterials[1] = Material(0.8f, 0.6f, 0.3f, 0.8f, 0.2f, 20.0f);
    _objectMatrices[1].rotate(15.0f, 1.0f, 1.0f, 0.0f);
    _objectMatrices[1].scale(0.5f);
    geom_torus(positions, normals, texcoords, indices);
    _objectVaos[2] = setupVao(positions.size() / 3, positions.data(), normals.data(), texcoords.data(),
            indices.size(), indices.data());
    _objectIndices[2] = indices.size();
    _objectMaterials[2] = Material(0.4f, 0.8f, 0.3f, 0.8f, 0.2f, 20.0f);
    _objectMatrices[2].rotate(15.0f, 1.0f, 1.0f, 0.0f);
    _objectMatrices[2].scale(0.5f);
    geom_teapot(positions, normals, texcoords, indices);
    _objectVaos[3] = setupVao(positions.size() / 3, positions.data(), normals.data(), texcoords.data(),
            indices.size(), indices.data());
    _objectIndices[3] = indices.size();
    _objectMaterials[3] = Material(0.3f, 0.3f, 0.8f, 0.8f, 0.2f, 20.0f);
    _objectMatrices[3].rotate(15.0f, 1.0f, 1.0f, 0.0f);
    geom_cylinder(positions, normals, texcoords, indices);
    _objectVaos[4] = setupVao(positions.size() / 3, positions.data(), normals.data(), texcoords.data(),
            indices.size(), indices.data());
    _objectIndices[4] = indices.size();
    _objectMaterials[4] = Material(0.3f, 0.8f, 0.8f, 0.8f, 0.2f, 20.0f);
    _objectMatrices[4].rotate(15.0f, 1.0f, 1.0f, 0.0f);
    _objectMatrices[4].scale(0.5f);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Shader program
    _prg.addShaderFromSourceFile(QOpenGLShader::Vertex, ":vertex-shader.glsl");
    _prg.addShaderFromSourceFile(QOpenGLShader::Fragment, ":fragment-shader.glsl");
    _prg.link();

    // Device model data
    for (int i = 0; i < QVRManager::deviceModelVertexDataCount(); i++) {
        _devModelVaos.append(setupVao(
                    QVRManager::deviceModelVertexCount(i),
                    QVRManager::deviceModelVertexPositions(i),
                    QVRManager::deviceModelVertexNormals(i),
                    QVRManager::deviceModelVertexTexCoords(i),
                    QVRManager::deviceModelVertexIndexCount(i),
                    QVRManager::deviceModelVertexIndices(i)));
        _devModelVaoIndices.append(QVRManager::deviceModelVertexIndexCount(i));
    }
    for (int i = 0; i < QVRManager::deviceModelTextureCount(); i++) {
        _devModelTextures.append(setupTex(QVRManager::deviceModelTexture(i)));
    }

    return true;
}

void QVRExampleOpenGL::render(QVRWindow* /* w */,
        const QVRRenderContext& context, int viewPass,
        unsigned int texture)
{
    // Set up framebuffer object to render into
    GLint width, height;
    glBindTexture(GL_TEXTURE_2D, texture);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
    glBindTexture(GL_TEXTURE_2D, _fboDepthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height,
            0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);

    // Set up view
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    QMatrix4x4 projectionMatrix = context.frustum(viewPass).toMatrix4x4();
    QMatrix4x4 viewMatrix = context.viewMatrix(viewPass);

    // Set up shader program
    glUseProgram(_prg.programId());
    _prg.setUniformValue("projection_matrix", projectionMatrix);
    //glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    // Render scene
    setMaterial(_floorMaterial);
    QMatrix4x4 groundMatrix;
    groundMatrix.scale(5.0f);
    groundMatrix.rotate(-90.0f, 1.0f, 0.0f, 0.0f);
    renderVao(viewMatrix, groundMatrix, _floorVao, _floorIndices);
    for (int i = 0; i < 5; i++) {
        setMaterial(_pillarMaterial);
        QMatrix4x4 pillarMatrix, pillarDiskMatrix, objectMatrix;
        pillarMatrix.rotate(18.0f + (i + 1) * 72.0f, 0.0f, 1.0f, 0.0f);
        pillarMatrix.translate(2.0f, 0.0f, 0.0f);
        pillarDiskMatrix = pillarMatrix;
        objectMatrix = pillarMatrix;
        pillarMatrix.translate(0.0f, 0.8f, 0.0f);
        pillarMatrix.scale(0.2f, 0.8f, 0.2f);
        renderVao(viewMatrix, pillarMatrix, _pillarVaos[0], _pillarIndices[0]);
        pillarDiskMatrix.translate(0.0f, 1.6f, 0.0f);
        pillarDiskMatrix.rotate(-90.0f, 1.0f, 0.0f, 0.0f);
        pillarDiskMatrix.scale(0.2f);
        renderVao(viewMatrix, pillarDiskMatrix, _pillarVaos[1], _pillarIndices[1]);
        setMaterial(_objectMaterials[i]);
        objectMatrix.translate(0.0f, 1.75f, 0.0f);
        objectMatrix.scale(0.2f);
        objectMatrix.rotate(_objectRotationAngle, 0.0f, 1.0f, 0.0f);
        objectMatrix *= _objectMatrices[i];
        renderVao(viewMatrix, objectMatrix, _objectVaos[i], _objectIndices[i]);
    }

    // Render device models (optional)
    for (int i = 0; i < QVRManager::deviceCount(); i++) {
        const QVRDevice& device = QVRManager::device(i);
        for (int j = 0; j < device.modelNodeCount(); j++) {
            QMatrix4x4 nodeMatrix = device.matrix();
            nodeMatrix.translate(device.modelNodePosition(j));
            nodeMatrix.rotate(device.modelNodeOrientation(j));
            int vertexDataIndex = device.modelNodeVertexDataIndex(j);
            int textureIndex = device.modelNodeTextureIndex(j);
            Material material(1.0f, 1.0f, 1.0f,
                    1.0f, 0.0f, 0.0f,
                    _devModelTextures[textureIndex], 0, 0,
                    1.0f);
            setMaterial(material);
            renderVao(viewMatrix, nodeMatrix,
                    _devModelVaos[vertexDataIndex],
                    _devModelVaoIndices[vertexDataIndex]);
        }
    }
}

void QVRExampleOpenGL::keyPressEvent(const QVRRenderContext& /* context */, QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Escape:
        _wantExit = true;
        break;
    }
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QVRManager manager(argc, argv);

    /* First set the default surface format that all windows will use */
    QSurfaceFormat format;
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setVersion(3, 3);
    QSurfaceFormat::setDefaultFormat(format);

    /* Then start QVR with your app */
    QVRExampleOpenGL qvrapp;
    if (!manager.init(&qvrapp)) {
        qCritical("Cannot initialize QVR manager");
        return 1;
    }

    /* Enter the standard Qt loop */
    return app.exec();
}
