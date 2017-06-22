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

#include <assimp/DefaultLogger.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <QGuiApplication>
#include <QKeyEvent>
#include <QFileInfo>
#include <QDir>

#include <qvr/manager.hpp>
#include <qvr/window.hpp>

#include "qvr-sceneviewer.hpp"
#include "sceneviewer.hpp"


QVRSceneViewer::QVRSceneViewer(const QString& sceneFilename, const aiScene* scene, const QMatrix4x4& M) :
    _sceneFilename(sceneFilename), _aiScene(scene), _M(M), _wantExit(false)
{
}

bool QVRSceneViewer::wantExit()
{
    return _wantExit;
}

bool QVRSceneViewer::initProcess(QVRProcess* /* p */)
{
    // Qt-based OpenGL function pointers
    initializeOpenGLFunctions();

    // Initialize the scene viewer
    qInfo("Starting initialization of scene viewer...");
    QFileInfo fileInfo(_sceneFilename);
    if (!_sceneViewer.init(_aiScene, fileInfo.dir().path(), _M))
        return false;
    qInfo("Initialization of scene viewer finished.");

    // FBO
    glGenFramebuffers(1, &_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    glGenTextures(1, &_fboDepthTex);
    glBindTexture(GL_TEXTURE_2D, _fboDepthTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 1, 1,
            0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _fboDepthTex, 0);

    return true;
}

void QVRSceneViewer::getNearFar(float& nearPlane, float& farPlane)
{
    nearPlane = 0.05f;
    farPlane = 500.0f; // for large scenes such as Rungholt
}

void QVRSceneViewer::render(QVRWindow* /* window */,
        const QVRRenderContext& context, const unsigned int* textures)
{
    for (int view = 0; view < context.viewCount(); view++) {
        // Get view dimensions
        int width = context.textureSize(view).width();
        int height = context.textureSize(view).height();
        // Set up framebuffer object to render into
        glBindTexture(GL_TEXTURE_2D, _fboDepthTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height,
                0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[view], 0);
        // Render
        glViewport(0, 0, width, height);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        _sceneViewer.render(context.frustum(view).toMatrix4x4(), context.viewMatrix(view));
    }
}

void QVRSceneViewer::keyPressEvent(const QVRRenderContext&, QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Escape:
        _wantExit = true;
        break;
    case Qt::Key_M:
        _sceneViewer.setMipmapping(!_sceneViewer.mipmapping());
        break;
    case Qt::Key_F:
        _sceneViewer.setAnisotropicFiltering(!_sceneViewer.anisotropicFiltering());
        break;
    case Qt::Key_T:
        _sceneViewer.setTransparency(!_sceneViewer.transparency());
        break;
    case Qt::Key_L:
        _sceneViewer.setLighting(!_sceneViewer.lighting());
        break;
    case Qt::Key_N:
        _sceneViewer.setNormalMapping(!_sceneViewer.normalMapping());
        break;
    }
}

static void applyRotation(QMatrix4x4& M, const QString& s)
{
    QStringList l = s.split(',', QString::SkipEmptyParts);
    bool ok[4] = { false, false, false, false };
    float v[4];
    if (l.size() == 4)
        for (int i = 0; i < 4; i++)
            v[i] = l[i].toFloat(ok + i);
    if (ok[0] && ok[1] && ok[2] && ok[3]) {
        M.rotate(v[0], v[1], v[2], v[3]);
    } else {
        qWarning("Ignoring invalid rotation %s", qPrintable(s));
    }
}

static void applyScaling(QMatrix4x4& M, const QString& s)
{
    QStringList l = s.split(',', QString::SkipEmptyParts);
    bool ok[3] = { false, false, false };
    float v[3];
    if (l.size() == 1) {
        v[0] = l[0].toFloat(ok + 0);
        v[1] = v[0];
        v[2] = v[0];
        ok[1] = ok[0];
        ok[2] = ok[0];
    } else if (l.size() == 3) {
        for (int i = 0; i < 3; i++)
            v[i] = l[i].toFloat(ok + i);
    }
    if (ok[0] && ok[1] && ok[2])
        M.scale(v[0], v[1], v[2]);
    else
        qWarning("Ignoring invalid scaling %s", qPrintable(s));
}

static void applyTranslation(QMatrix4x4& M, const QString& s)
{
    QStringList l = s.split(',', QString::SkipEmptyParts);
    bool ok[3] = { false, false, false };
    float v[3];
    if (l.size() == 3)
        for (int i = 0; i < 3; i++)
            v[i] = l[i].toFloat(ok + i);
    if (ok[0] && ok[1] && ok[2])
        M.translate(v[0], v[1], v[2]);
    else
        qWarning("Ignoring invalid translation %s", qPrintable(s));
}

static QMatrix4x4 getMatrix(int& argc, char* argv[])
{
    QMatrix4x4 M;

    QVector<bool> remove(argc);
    for (int i = 0; i < argc; i++)
        remove[i] = false;

    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "--rotate") == 0 || strcmp(argv[i], "-r") == 0) && i < argc - 1) {
            applyRotation(M, argv[i + 1]);
            remove[i] = true; remove[i + 1] = true;
        } else if (strncmp(argv[i], "--rotate=", 9) == 0) {
            applyRotation(M, argv[i] + 9);
            remove[i] = true;
        } else if ((strcmp(argv[i], "--scale") == 0 || strcmp(argv[i], "-s") == 0) && i < argc - 1) {
            applyScaling(M, argv[i + 1]);
            remove[i] = true; remove[i + 1] = true;
        } else if (strncmp(argv[i], "--scale=", 8) == 0) {
            applyScaling(M, argv[i] + 8);
            remove[i] = true;
        } else if ((strcmp(argv[i], "--translate") == 0 || strcmp(argv[i], "-t") == 0) && i < argc - 1) {
            applyTranslation(M, argv[i + 1]);
            remove[i] = true; remove[i + 1] = true;
        } else if (strncmp(argv[i], "--translate=", 12) == 0) {
            applyTranslation(M, argv[i] + 12);
            remove[i] = true;
        }
    }
    for (int i = 1; i < argc;) {
        if (remove[i]) {
            for (int j = i; j < argc - 1; j++) {
                argv[j] = argv[j + 1];
                remove[j] = remove[j + 1];
            }
            argc -= 1;
        } else {
            i++;
        }
    }

    return M;
}

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QVRManager manager(argc, argv);

    /* Get the transformation matrix from the command line */
    QMatrix4x4 matrix = getMatrix(argc, argv);

    /* Load the scene file */
    if (argc != 2) {
        qCritical("%s: requires scene filename argument.", argv[0]);
        return 1;
    }
    qInfo("Starting scene import for %s ...", argv[1]);
    Assimp::DefaultLogger::create("", Assimp::Logger::NORMAL, aiDefaultLogStream_STDERR);
    Assimp::Importer importer;
    importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS,
            aiComponent_TANGENTS_AND_BITANGENTS | aiComponent_COLORS |
            aiComponent_ANIMATIONS | aiComponent_CAMERAS);
    const aiScene* scene = importer.ReadFile(argv[1],
            aiProcess_GenSmoothNormals
            | aiProcess_JoinIdenticalVertices
            | aiProcess_ImproveCacheLocality
            | aiProcess_Debone
            | aiProcess_RemoveRedundantMaterials
            | aiProcess_Triangulate
            | aiProcess_GenUVCoords
            | aiProcess_SortByPType
            | aiProcess_FindInvalidData
            | aiProcess_FindInstances
            | aiProcess_ValidateDataStructure
            | aiProcess_OptimizeMeshes
            | aiProcess_RemoveComponent
            | aiProcess_PreTransformVertices
            //| aiProcess_FixInfacingNormals
            | aiProcess_TransformUVCoords);
    Assimp::DefaultLogger::kill();
    if (!scene) {
        qCritical("Scene import for %s failed: %s", argv[1], importer.GetErrorString());
        return 1;
    }
    qInfo("Scene import finished.");

    /* First set the default surface format that all windows will use */
    QSurfaceFormat format;
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setVersion(3, 3);
    QSurfaceFormat::setDefaultFormat(format);

    /* Then start QVR with your app */
    QVRSceneViewer qvrapp(argv[1], scene, matrix);
    if (!manager.init(&qvrapp)) {
        qCritical("Cannot initialize QVR manager");
        return 1;
    }

    /* Enter the standard Qt loop */
    return app.exec();
}
