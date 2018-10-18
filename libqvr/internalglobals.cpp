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

#include <QGuiApplication>
#include <QScreen>

#include <cstring>

#include "internalglobals.hpp"
#include "manager.hpp"
#include "logging.hpp"

#ifdef HAVE_QGAMEPAD
# include <QGamepadManager>
# if defined(Q_OS_WIN) && (QT_VERSION <= QT_VERSION_CHECK(5, 9, 0))
#  include <QWindow>
#  include <QGuiApplication>
# endif
#endif

#ifdef ANDROID
# include <QtMath>
# include <QThread>
/* We should include <GLES3/gl3.h>, but Qt always uses Android API level 16
 * which does not have that, and there is no way to choose a different API level
 * in the .pro file, and we cannot use an AndroidManifest.xml file to choose the
 * API level because this is a library and not an app.
 * As a workaround, we use GLES2 headers plus extensions. */
# define GL_GLEXT_PROTOTYPES
# include <GLES2/gl2ext.h>
# define glGenVertexArrays glGenVertexArraysOES
# define glBindVertexArray glBindVertexArrayOES
#endif


/* Global manager instance (singleton) */
QVRManager* QVRManagerInstance = NULL;

/* Global screen info */
int QVRScreenCount = 0;
int QVRPrimaryScreen = -1;
QVector<QRect> QVRScreenGeometries;
QVector<QSizeF> QVRScreenSizes;
void QVRGetScreenInfo()
{
    QList<QScreen*> screenList = QGuiApplication::screens();
    QScreen* primaryScreen = QGuiApplication::primaryScreen();
    QVRScreenCount = screenList.length();
    QVRScreenGeometries.resize(QVRScreenCount);
    QVRScreenSizes.resize(QVRScreenCount);
    for (int i = 0; i < QVRScreenCount; i++) {
        if (primaryScreen == screenList[i])
            QVRPrimaryScreen = i;
        QVRScreenGeometries[i] = screenList[i]->geometry();
        QVRScreenSizes[i] = (screenList[i]->physicalSize()) / 1000.0f; // convert to meters
    }
}

/* Global helper functions */
void QVRMatrixToPose(const QMatrix4x4& matrix, QQuaternion* orientation, QVector3D* position)
{
    *orientation = QQuaternion::fromRotationMatrix(matrix.toGenericMatrix<3, 3>());
    *position = QVector3D(matrix(0, 3), matrix(1, 3), matrix(2, 3));
}

/* Global event queue */
QQueue<QVREvent>* QVREventQueue = NULL;

/* Global timer */
QElapsedTimer QVRTimer;

/* Global renderable device model data */
QList<QVector<float>> QVRDeviceModelVertexPositions;
QList<QVector<float>> QVRDeviceModelVertexNormals;
QList<QVector<float>> QVRDeviceModelVertexTexCoords;
QList<QVector<unsigned short>> QVRDeviceModelVertexIndices;
QList<QImage> QVRDeviceModelTextures;

/* Global list of gamepads */
#ifdef HAVE_QGAMEPAD
QList<int> QVRGamepads;
void QVRDetectGamepads()
{
    QGamepadManager* mgr = QGamepadManager::instance();
# if defined(Q_OS_WIN) && (QT_VERSION <= QT_VERSION_CHECK(5, 9, 0))
    /* See https://bugreports.qt.io/browse/QTBUG-61553 */
    QWindow* dummy = new QWindow();
    dummy->show();
    delete dummy;
    QGuiApplication::processEvents();
#endif
    QVRGamepads = mgr->connectedGamepads();
}
#endif

/* Global variables and functions for Oculus Rift support */
#ifdef HAVE_OCULUS
# if (OVR_PRODUCT_VERSION >= 1)
ovrSession QVROculus = NULL;
ovrGraphicsLuid QVROculusLuid;
ovrTextureSwapChain QVROculusTextureSwapChainL = 0;
ovrTextureSwapChain QVROculusTextureSwapChainR = 0;
long long QVROculusFrameIndex = 0;
ovrLayerEyeFov QVROculusLayer;
ovrPosef QVROculusHmdToEyeViewPose[2];
ovrInputState QVROculusInputState;
# else
ovrHmd QVROculus = NULL;
# endif
ovrTrackingState QVROculusTrackingState;
ovrPosef QVROculusRenderPoses[2];
ovrEyeRenderDesc QVROculusEyeRenderDesc[2];
static void oculusLogCallback(
# if (OVR_PRODUCT_VERSION >= 1)
    uintptr_t /* userData */,
# endif
    int level, const char* message)
{
    if (level == ovrLogLevel_Debug) {
        QVR_DEBUG("Oculus log: %s", message);
    } else if (level == ovrLogLevel_Info) {
        QVR_INFO("Oculus log: %s", message);
    } else {
        QVR_WARNING("Oculus log: %s", message);
    }
}
int QVROculusControllers = 0;
void QVRAttemptOculusInitialization()
{
    QVR_DEBUG("Oculus: SDK version %s", ovr_GetVersionString());
    ovrInitParams oculusInitParams;
    std::memset(&oculusInitParams, 0, sizeof(oculusInitParams));
    oculusInitParams.LogCallback = oculusLogCallback;
    if (
# if (OVR_PRODUCT_VERSION >= 1)
        OVR_FAILURE(ovr_Initialize(&oculusInitParams)) || OVR_FAILURE(ovr_Create(&QVROculus, &QVROculusLuid))
# else
        !ovr_Initialize(&oculusInitParams) || ovrHmd_Detect() <= 0 || !(QVROculus = ovrHmd_Create(0))
# endif
        ) {
        QVR_INFO("Oculus: cannot initialize SDK or HMD not available");
        return;
    }
    QVR_INFO("Oculus: HMD available");
# if (OVR_PRODUCT_VERSION >= 1)
    const auto oculusHmdDesc = ovr_GetHmdDesc(QVROculus);
    QVROculusEyeRenderDesc[0] = ovr_GetRenderDesc(QVROculus, ovrEye_Left, oculusHmdDesc.DefaultEyeFov[0]);
    QVROculusEyeRenderDesc[1] = ovr_GetRenderDesc(QVROculus, ovrEye_Right, oculusHmdDesc.DefaultEyeFov[1]);
    QVROculusHmdToEyeViewPose[0] = QVROculusEyeRenderDesc[0].HmdToEyePose;
    QVROculusHmdToEyeViewPose[1] = QVROculusEyeRenderDesc[1].HmdToEyePose;
    unsigned int connectedControllers = ovr_GetConnectedControllerTypes(QVROculus);
    if (connectedControllers & ovrControllerType_XBox)
        QVROculusControllers = 1;
    else if (connectedControllers & ovrControllerType_Touch)
        QVROculusControllers = 4;
    else if (connectedControllers & ovrControllerType_LTouch)
        QVROculusControllers = 2;
    else if (connectedControllers & ovrControllerType_RTouch)
        QVROculusControllers = 3;
# else
#  define oculusHmdDesc (*QVROculus)
# endif
    QVR_DEBUG("Oculus: product name: %s", oculusHmdDesc.ProductName);
    QVR_DEBUG("Oculus: resolution: %dx%d", oculusHmdDesc.Resolution.w, oculusHmdDesc.Resolution.h);
    QVR_INFO("Oculus: available controllers: %s",
            QVROculusControllers == 1 ? "xbox"
            : QVROculusControllers == 2 ? "touch (left)"
            : QVROculusControllers == 3 ? "touch (right)"
            : QVROculusControllers == 4 ? "touch (left and right)"
            : "none");
# if (OVR_PRODUCT_VERSION < 1)
    // this is done automatically with newer SDKs
    ovrHmd_ConfigureTracking(QVROculus,
            ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection | ovrTrackingCap_Position,
            ovrTrackingCap_Orientation | ovrTrackingCap_Position);
# endif
}
void QVRUpdateOculus()
{
# if (OVR_PRODUCT_VERSION >= 1)
    QVROculusFrameIndex++;
    double dt = ovr_GetPredictedDisplayTime(QVROculus, QVROculusFrameIndex);
    QVROculusTrackingState = ovr_GetTrackingState(QVROculus, dt, ovrTrue);
    ovr_CalcEyePoses(QVROculusTrackingState.HeadPose.ThePose, QVROculusHmdToEyeViewPose, QVROculusRenderPoses);
    if (QVROculusControllers == 1)
        ovr_GetInputState(QVROculus, ovrControllerType_XBox, &QVROculusInputState);
    else if (QVROculusControllers == 2 || QVROculusControllers == 3 || QVROculusControllers == 4)
        ovr_GetInputState(QVROculus, ovrControllerType_Touch, &QVROculusInputState);
    QVROculusLayer.SensorSampleTime = ovr_GetTimeInSeconds();
# else
    ovrHmd_BeginFrame(QVROculus, 0);
    ovrVector3f hmdToEyeViewOffset[2] = {
        QVROculusEyeRenderDesc[0].HmdToEyeViewOffset,
        QVROculusEyeRenderDesc[1].HmdToEyeViewOffset
    };
    ovrHmd_GetEyePoses(QVROculus, 0, hmdToEyeViewOffset,
            QVROculusRenderPoses, &QVROculusTrackingState);
# endif
}
#endif

/* Global variables and functions for OpenVR (HTC Vive) support */
#ifdef HAVE_OPENVR
vr::IVRSystem* QVROpenVRSystem = NULL;
unsigned int QVROpenVRControllerIndices[2];
vr::VRControllerState_t QVROpenVRControllerStates[2];
vr::TrackedDevicePose_t QVROpenVRPoses[3];
QQuaternion QVROpenVRTrackedOrientations[5];   // head, left eye, right eye, controller0, controller1
QVector3D QVROpenVRTrackedPositions[5];        // head, left eye, right eye, controller0, controller1
bool QVROpenVRHaveTrackedVelocities[5];        // head, left eye, right eye, controller0, controller1
QVector3D QVROpenVRTrackedVelocities[5];       // head, left eye, right eye, controller0, controller1
QVector3D QVROpenVRTrackedAngularVelocities[5];// head, left eye, right eye, controller0, controller1
QVector<QVector3D> QVROpenVRControllerModelPositions[2];
QVector<QQuaternion> QVROpenVRControllerModelOrientations[2];
QVector<int> QVROpenVRControllerModelVertexDataIndices[2];
QVector<int> QVROpenVRControllerModelTextureIndices[2];
static QMatrix4x4 QVROpenVRHmdToEye[2];
static QMatrix4x4 QVROpenVRConvertMatrix(const vr::HmdMatrix34_t& openVrMatrix)
{
    QMatrix4x4 m;
    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 4; c++)
            m(r, c) = openVrMatrix.m[r][c];
    m(3, 0) = 0;
    m(3, 1) = 0;
    m(3, 2) = 0;
    m(3, 3) = 1;
    return m;
}
static QMap<QString, int> QVROpenVRNameToVertexDataIndexMap;
static QMap<QString, int> QVROpenVRNameToTextureIndexMap;
static QMap<int, int> QVROpenVRTexIdToIndexMap;
void QVRAttemptOpenVRInitialization()
{
    if (!vr::VR_IsHmdPresent()) {
        QVR_INFO("OpenVR: no HMD available");
        return;
    }
    QVR_DEBUG("OpenVR: attempting initialization");
    vr::EVRInitError e = vr::VRInitError_None;
    QVROpenVRSystem = vr::VR_Init(&e, vr::VRApplication_Scene);
    if (!QVROpenVRSystem) {
        QVR_INFO("OpenVR: initialization failed: %s", vr::VR_GetVRInitErrorAsEnglishDescription(e));
        return;
    }
    QVR_INFO("OpenVR: initialization succeeded");
    std::memset(QVROpenVRControllerStates, 0, sizeof(QVROpenVRControllerStates));
    QVROpenVRHmdToEye[0] = QVROpenVRConvertMatrix(QVROpenVRSystem->GetEyeToHeadTransform(vr::Eye_Left));
    QVROpenVRHmdToEye[1] = QVROpenVRConvertMatrix(QVROpenVRSystem->GetEyeToHeadTransform(vr::Eye_Right));
    int controllerIndex = 0;
    for (unsigned int i = 0; i < vr::k_unMaxTrackedDeviceCount; i++) {
        if (QVROpenVRSystem->GetTrackedDeviceClass(i) == vr::TrackedDeviceClass_Controller) {
            QVROpenVRControllerIndices[controllerIndex] = i;
            QVR_DEBUG("OpenVR: controller %d has device index %u", controllerIndex, i);
            controllerIndex++;
            if (controllerIndex >= 2)
                break;
        }
    }

    // Get render models for the two controllers
    QVector<char> buffer;
    int bufSize;
    for (int i = 0; i < 2; i++) {
        int deviceIndex = QVROpenVRControllerIndices[i];
        buffer.clear();
        buffer.resize(vr::k_unMaxPropertyStringSize);
        QVROpenVRSystem->GetStringTrackedDeviceProperty(deviceIndex, vr::Prop_RenderModelName_String, buffer.data(), vr::k_unMaxPropertyStringSize, 0);
        QString renderModelName(buffer.data());
        if (renderModelName.isEmpty()) {
            QVR_DEBUG("OpenVR controller %d has no render model", i);
        } else {
            QVR_DEBUG("OpenVR controller %d has render model %s", i, qPrintable(renderModelName));
            int m = vr::VRRenderModels()->GetComponentCount(qPrintable(renderModelName));
            QVR_DEBUG("    %d components", m);
            // reserve the space to avoid repeated reallocation in QVRUpdateOpenVR()
            QVROpenVRControllerModelPositions[i].reserve(m);
            QVROpenVRControllerModelOrientations[i].reserve(m);
            QVROpenVRControllerModelVertexDataIndices[i].reserve(m);
            QVROpenVRControllerModelTextureIndices[i].reserve(m);
            for (int j = 0; j < m; j++) {
                bufSize = vr::VRRenderModels()->GetComponentName(qPrintable(renderModelName), j, 0, 0);
                buffer.clear();
                buffer.resize(qMin(1, bufSize));
                vr::VRRenderModels()->GetComponentName(qPrintable(renderModelName), j, buffer.data(), bufSize);
                QString componentName(buffer.data());
                QVR_DEBUG("    component %d: name %s", j, qPrintable(componentName));
                bufSize = vr::VRRenderModels()->GetComponentRenderModelName(qPrintable(renderModelName), qPrintable(componentName), 0, 0);
                buffer.clear();
                buffer.resize(qMin(1, bufSize));
                vr::VRRenderModels()->GetComponentRenderModelName(qPrintable(renderModelName), qPrintable(componentName), buffer.data(), bufSize);
                QString componentRenderModelName(buffer.data());
                if (componentRenderModelName.isEmpty()) {
                    QVR_DEBUG("        component has no render model");
                } else if (QVROpenVRNameToVertexDataIndexMap.contains(componentRenderModelName)) {
                    QVR_DEBUG("        render model %s already loaded", qPrintable(componentRenderModelName));
                    QVROpenVRControllerModelVertexDataIndices[i].append(QVROpenVRNameToVertexDataIndexMap.value(componentRenderModelName));
                    QVROpenVRControllerModelTextureIndices[i].append(QVROpenVRNameToTextureIndexMap.value(componentRenderModelName));
                } else {
                    QVR_DEBUG("        loading render model %s...", qPrintable(componentRenderModelName));
                    vr::RenderModel_t* renderModel;
                    vr::EVRRenderModelError renderModelError;
                    while ((renderModelError = vr::VRRenderModels()->LoadRenderModel_Async(
                                    qPrintable(componentRenderModelName), &renderModel))
                            == vr::VRRenderModelError_Loading) {
                    }
                    if (renderModelError != vr::VRRenderModelError_None) {
                        QVR_DEBUG("        ...failed: %s",
                                vr::VRRenderModels()->GetRenderModelErrorNameFromEnum(renderModelError));
                        continue;
                    }
                    QVR_DEBUG("        ...done");
                    int texId = renderModel->diffuseTextureId;
                    if (QVROpenVRTexIdToIndexMap.contains(texId)) {
                        QVR_DEBUG("            texture already loaded");
                        QVROpenVRNameToTextureIndexMap.insert(componentRenderModelName, QVROpenVRTexIdToIndexMap.value(texId));
                    } else {
                        QVR_DEBUG("            loading texture...");
                        vr::RenderModel_TextureMap_t* textureMap;
                        while ((renderModelError = vr::VRRenderModels()->LoadTexture_Async(texId, &textureMap))
                                == vr::VRRenderModelError_Loading) {
                        }
                        if (renderModelError != vr::VRRenderModelError_None) {
                            QVR_WARNING("cannot load OpenVR controller %d render model component %s texture id %d: %s", i,
                                    qPrintable(componentRenderModelName), texId,
                                    vr::VRRenderModels()->GetRenderModelErrorNameFromEnum(renderModelError));
                            vr::VRRenderModels()->FreeRenderModel(renderModel);
                            continue;
                        }
                        QImage textureRaw(textureMap->rubTextureMapData,
                                textureMap->unWidth, textureMap->unHeight,
                                QImage::Format_RGBA8888);
                        QImage texture = textureRaw.copy(textureRaw.rect());
                        vr::VRRenderModels()->FreeTexture(textureMap);
                        QVRDeviceModelTextures.append(texture);
                        QVROpenVRNameToTextureIndexMap.insert(componentRenderModelName, QVRDeviceModelTextures.size() - 1);
                    }
                    int vertexCount = renderModel->unVertexCount;
                    QVector<float> vertexPositions(3 * vertexCount);
                    QVector<float> vertexNormals(3 * vertexCount);
                    QVector<float> vertexTexCoords(2 * vertexCount);
                    for (int k = 0; k < vertexCount; k++) {
                        vertexPositions[3 * k + 0] = renderModel->rVertexData[k].vPosition.v[0];
                        vertexPositions[3 * k + 1] = renderModel->rVertexData[k].vPosition.v[1];
                        vertexPositions[3 * k + 2] = renderModel->rVertexData[k].vPosition.v[2];
                        vertexNormals[3 * k + 0] = renderModel->rVertexData[k].vNormal.v[0];
                        vertexNormals[3 * k + 1] = renderModel->rVertexData[k].vNormal.v[1];
                        vertexNormals[3 * k + 2] = renderModel->rVertexData[k].vNormal.v[2];
                        vertexTexCoords[2 * k + 0] = renderModel->rVertexData[k].rfTextureCoord[0];
                        vertexTexCoords[2 * k + 1] = renderModel->rVertexData[k].rfTextureCoord[1];
                    }
                    int indexCount = 3 * renderModel->unTriangleCount;
                    QVector<unsigned short> vertexIndices(indexCount);
                    for (int k = 0; k < indexCount; k++) {
                        vertexIndices[k] = renderModel->rIndexData[k];
                    }
                    QVRDeviceModelVertexPositions.append(vertexPositions);
                    QVRDeviceModelVertexNormals.append(vertexNormals);
                    QVRDeviceModelVertexTexCoords.append(vertexTexCoords);
                    QVRDeviceModelVertexIndices.append(vertexIndices);
                    QVROpenVRNameToVertexDataIndexMap.insert(componentRenderModelName, QVRDeviceModelVertexPositions.size() - 1);
                    vr::VRRenderModels()->FreeRenderModel(renderModel);
                }
            }
        }
    }
}
void QVRUpdateOpenVR()
{
    vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount];
    vr::VRCompositor()->WaitGetPoses(poses, vr::k_unMaxTrackedDeviceCount, NULL, 0);
    // head:
    QMatrix4x4 headMatrix = QVROpenVRConvertMatrix(poses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking);
    QVRMatrixToPose(headMatrix, &(QVROpenVRTrackedOrientations[0]), &(QVROpenVRTrackedPositions[0]));
    QVROpenVRHaveTrackedVelocities[0] = true;
    QVROpenVRTrackedVelocities[0] = QVector3D(poses[vr::k_unTrackedDeviceIndex_Hmd].vVelocity.v[0],
                                              poses[vr::k_unTrackedDeviceIndex_Hmd].vVelocity.v[1],
                                              poses[vr::k_unTrackedDeviceIndex_Hmd].vVelocity.v[2]);
    QVROpenVRTrackedAngularVelocities[0] = QVector3D(poses[vr::k_unTrackedDeviceIndex_Hmd].vAngularVelocity.v[0],
                                                     poses[vr::k_unTrackedDeviceIndex_Hmd].vAngularVelocity.v[1],
                                                     poses[vr::k_unTrackedDeviceIndex_Hmd].vAngularVelocity.v[2]);
    // left eye:
    QMatrix4x4 leftEyeMatrix = headMatrix * QVROpenVRHmdToEye[0];
    QVRMatrixToPose(leftEyeMatrix, &(QVROpenVRTrackedOrientations[1]), &(QVROpenVRTrackedPositions[1]));
    QVROpenVRHaveTrackedVelocities[1] = false;
    // right eye:
    QMatrix4x4 rightEyeMatrix = headMatrix * QVROpenVRHmdToEye[1];
    QVRMatrixToPose(rightEyeMatrix, &(QVROpenVRTrackedOrientations[2]), &(QVROpenVRTrackedPositions[2]));
    QVROpenVRHaveTrackedVelocities[2] = false;
    // controller 0:
    QVRMatrixToPose(QVROpenVRConvertMatrix(poses[QVROpenVRControllerIndices[0]].mDeviceToAbsoluteTracking),
            &(QVROpenVRTrackedOrientations[3]), &(QVROpenVRTrackedPositions[3]));
    QVROpenVRHaveTrackedVelocities[3] = true;
    QVROpenVRTrackedVelocities[3] = QVector3D(poses[QVROpenVRControllerIndices[0]].vVelocity.v[0],
                                              poses[QVROpenVRControllerIndices[0]].vVelocity.v[1],
                                              poses[QVROpenVRControllerIndices[0]].vVelocity.v[2]);
    QVROpenVRTrackedAngularVelocities[3] = QVector3D(poses[QVROpenVRControllerIndices[0]].vAngularVelocity.v[0],
                                                     poses[QVROpenVRControllerIndices[0]].vAngularVelocity.v[1],
                                                     poses[QVROpenVRControllerIndices[0]].vAngularVelocity.v[2]);
    // controller 1:
    QVRMatrixToPose(QVROpenVRConvertMatrix(poses[QVROpenVRControllerIndices[1]].mDeviceToAbsoluteTracking),
            &(QVROpenVRTrackedOrientations[4]), &(QVROpenVRTrackedPositions[4]));
    QVROpenVRHaveTrackedVelocities[4] = true;
    QVROpenVRTrackedVelocities[4] = QVector3D(poses[QVROpenVRControllerIndices[1]].vVelocity.v[0],
                                              poses[QVROpenVRControllerIndices[1]].vVelocity.v[1],
                                              poses[QVROpenVRControllerIndices[1]].vVelocity.v[2]);
    QVROpenVRTrackedAngularVelocities[4] = QVector3D(poses[QVROpenVRControllerIndices[1]].vAngularVelocity.v[0],
                                                     poses[QVROpenVRControllerIndices[1]].vAngularVelocity.v[1],
                                                     poses[QVROpenVRControllerIndices[1]].vAngularVelocity.v[2]);
    // controller states:
    QVROpenVRSystem->GetControllerState(QVROpenVRControllerIndices[0], &(QVROpenVRControllerStates[0]),
            sizeof(QVROpenVRControllerStates[0]));
    QVROpenVRSystem->GetControllerState(QVROpenVRControllerIndices[1], &(QVROpenVRControllerStates[1]),
            sizeof(QVROpenVRControllerStates[1]));

    // update render models for the two controllers
    QVector<char> buffer;
    int bufSize;
    for (int i = 0; i < 2; i++) {
        QVROpenVRControllerModelPositions[i].clear();
        QVROpenVRControllerModelOrientations[i].clear();
        QVROpenVRControllerModelVertexDataIndices[i].clear();
        QVROpenVRControllerModelTextureIndices[i].clear();
        int deviceIndex = QVROpenVRControllerIndices[i];
        buffer.clear();
        buffer.resize(vr::k_unMaxPropertyStringSize);
        QVROpenVRSystem->GetStringTrackedDeviceProperty(deviceIndex, vr::Prop_RenderModelName_String, buffer.data(), vr::k_unMaxPropertyStringSize, 0);
        QString renderModelName(buffer.data());
        if (!renderModelName.isEmpty()) {
            int m = vr::VRRenderModels()->GetComponentCount(qPrintable(renderModelName));
            for (int j = 0; j < m; j++) {
                bufSize = vr::VRRenderModels()->GetComponentName(qPrintable(renderModelName), j, 0, 0);
                buffer.clear();
                buffer.resize(qMin(1, bufSize));
                vr::VRRenderModels()->GetComponentName(qPrintable(renderModelName), j, buffer.data(), bufSize);
                QString componentName(buffer.data());
                bufSize = vr::VRRenderModels()->GetComponentRenderModelName(qPrintable(renderModelName), qPrintable(componentName), 0, 0);
                buffer.clear();
                buffer.resize(qMin(1, bufSize));
                vr::VRRenderModels()->GetComponentRenderModelName(qPrintable(renderModelName), qPrintable(componentName), buffer.data(), bufSize);
                QString componentRenderModelName(buffer.data());
                if (!componentRenderModelName.isEmpty()) {
                    vr::RenderModel_ControllerMode_State_t state;
                    state.bScrollWheelVisible = false;
                    vr::RenderModel_ComponentState_t componentState;
                    vr::VRRenderModels()->GetComponentState(qPrintable(renderModelName), qPrintable(componentName),
                            &(QVROpenVRControllerStates[i]), &state, &componentState);
                    if (componentState.uProperties & vr::VRComponentProperty_IsVisible) {
                        QMatrix4x4 M = QVROpenVRConvertMatrix(componentState.mTrackingToComponentRenderModel);
                        QQuaternion q;
                        QVector3D p;
                        QVRMatrixToPose(M, &q, &p);
                        QVROpenVRControllerModelPositions[i].append(p);
                        QVROpenVRControllerModelOrientations[i].append(q);
                        QVROpenVRControllerModelVertexDataIndices[i].append(QVROpenVRNameToVertexDataIndexMap.value(componentRenderModelName));
                        QVROpenVRControllerModelTextureIndices[i].append(QVROpenVRNameToTextureIndexMap.value(componentRenderModelName));
                    }
                }
            }
        }
    }
}
#endif

/* Global functions and variables for Google VR support */
#ifdef ANDROID
#include <QtAndroid>
#include <QAndroidJniObject>
gvr_context* QVRGoogleVR = NULL;
gvr_controller_context* QVRGoogleVRController = NULL;
gvr_buffer_viewport_list* QVRGoogleVRViewportList = NULL;
gvr_swap_chain* QVRGoogleVRSwapChain = NULL;
float QVRGoogleVRResolutionFactor = 0.0f; // must be set by the window
QSize QVRGoogleVRTexSize;
QRectF QVRGoogleVRRelativeViewports[2]; // viewports inside buffer for each eye
float QVRGoogleVRlrbt[2][4]; // frustum l, r, b, t for each eye, at n=1
gvr_mat4f QVRGoogleVRHeadMatrix;
QVector3D QVRGoogleVRPositions[4];      // 0 = left eye, 1 = right eye, 2 = head, 3 = daydream controller
QQuaternion QVRGoogleVROrientations[4]; // 0 = left eye, 1 = right eye, 2 = head, 3 = daydream controller
QAtomicInt QVRGoogleVRTouchEvent;
bool QVRGoogleVRButtons[3] = { false, false, false };
float QVRGoogleVRAxes[2] = { 0.0f, 0.0f };
QAtomicInt QVRGoogleVRSync; // 0 = new frame, 1 = render to GVR, 2 = submit to GVR and swap
unsigned int QVRGoogleVRTextures[2] = { 0, 0 };
static gvr_controller_state* QVRGoogleVRControllerState = NULL;
static float QVRGoogleVRDisplayFPS;
static QVector3D QVRGoogleVREyeFromHeadPositions[2];
static QQuaternion QVRGoogleVREyeFromHeadOrientations[2];
static unsigned int QVRGoogleVRVao;
static unsigned int QVRGoogleVRPrg;
void QVRAttemptGoogleVRInitialization()
{
    QVR_INFO("GoogleVR: version: %s", gvr_get_version_string());
    QAndroidJniObject activity = QtAndroid::androidActivity();
    jlong nativeGvrContext = activity.callMethod<jlong>("getNativeGvrContext");
    QVRGoogleVR = reinterpret_cast<gvr_context*>(nativeGvrContext);
    if (!QVRGoogleVR) {
        QVR_WARNING("GoogleVR: cannot get native context");
        return;
    }
    int32_t viewerType = gvr_get_viewer_type(QVRGoogleVR);
    QVR_INFO("GoogleVR: VR flavor is %s",
              viewerType == GVR_VIEWER_TYPE_CARDBOARD ? "cardboard"
            : viewerType == GVR_VIEWER_TYPE_DAYDREAM ? "daydream"
            : "unknown");
    if (viewerType == GVR_VIEWER_TYPE_DAYDREAM) {
        int32_t options = gvr_controller_get_default_options();
        options |= GVR_CONTROLLER_ENABLE_ORIENTATION | GVR_CONTROLLER_ENABLE_POSITION
            | GVR_CONTROLLER_ENABLE_TOUCH;
        QVRGoogleVRController = gvr_controller_create_and_init(options, QVRGoogleVR);
        if (QVRGoogleVRController) {
            QVRGoogleVRControllerState = gvr_controller_state_create();
            gvr_controller_resume(QVRGoogleVRController);
        }
    }
    QVRGoogleVRDisplayFPS = QGuiApplication::primaryScreen()->refreshRate();
    QVR_INFO("GoogleVR: display refresh rate is %g Hz", QVRGoogleVRDisplayFPS);
}
void QVRUpdateGoogleVR()
{
    gvr_clock_time_point t = gvr_get_time_point_now();
    // TODO: calculate time of next vsync
    // There should really be a simple function in the Google VR NDK for this.
    // My first attempt was to just guess based on screen refresh rate:
    //     t.monotonic_system_time_nanos += 1e9f / QVRGoogleVRDisplayFPS;
    // But this caused jumpy head tracking. So now I just do what all the
    // Google sample programs do: simply add 50ms. This works, for no obvious reason.
    t.monotonic_system_time_nanos += 50000000;
    QVRGoogleVRHeadMatrix = gvr_get_head_space_from_start_space_rotation(QVRGoogleVR, t);
    QVRMatrixToPose(QMatrix4x4(reinterpret_cast<const float*>(QVRGoogleVRHeadMatrix.m)),
            &(QVRGoogleVROrientations[2]), &(QVRGoogleVRPositions[2]));
    QVRGoogleVROrientations[2] = QVRGoogleVROrientations[2].inverted();
    QVRGoogleVRPositions[2] = -QVRGoogleVRPositions[2];
    // This Y offset moves the user's head and eyes from 0 to a default standing height in the virtual world:
    QVRGoogleVRPositions[2].setY(QVRGoogleVRPositions[2].y() + QVRObserverConfig::defaultEyeHeight);
    for (int i = 0; i < 2; i++) {
        QVRGoogleVRPositions[i] = QVRGoogleVRPositions[2] + QVRGoogleVROrientations[2] * QVRGoogleVREyeFromHeadPositions[i];
        QVRGoogleVROrientations[i] = QVRGoogleVROrientations[2] * QVRGoogleVREyeFromHeadOrientations[i];
    }
    if (QVRGoogleVRController) {
        gvr_controller_state_update(QVRGoogleVRController, 0, QVRGoogleVRControllerState);
        if (gvr_controller_state_get_api_status(QVRGoogleVRControllerState) == GVR_CONTROLLER_API_OK) {
            gvr_quatf rot = gvr_controller_state_get_orientation(QVRGoogleVRControllerState);
            QVRGoogleVROrientations[3] = QQuaternion(rot.qw, rot.qx, rot.qy, rot.qz);
            gvr_vec3f pos = gvr_controller_state_get_position(QVRGoogleVRControllerState);
            QVRGoogleVRPositions[3] = QVector3D(pos.x, pos.y, pos.z);
            QVRGoogleVRButtons[0] = gvr_controller_state_get_button_state(QVRGoogleVRControllerState, GVR_CONTROLLER_BUTTON_CLICK);
            QVRGoogleVRButtons[1] = gvr_controller_state_get_button_state(QVRGoogleVRControllerState, GVR_CONTROLLER_BUTTON_HOME);
            QVRGoogleVRButtons[2] = gvr_controller_state_get_button_state(QVRGoogleVRControllerState, GVR_CONTROLLER_BUTTON_APP);
            QVRGoogleVRAxes[0] = 0.0f;
            QVRGoogleVRAxes[1] = 0.0f;
            if (gvr_controller_state_is_touching(QVRGoogleVRControllerState)) {
                gvr_vec2f touchPos = gvr_controller_state_get_touch_pos(QVRGoogleVRControllerState);
                QVRGoogleVRAxes[0] = -(touchPos.y * 2.0f - 1.0f);
                QVRGoogleVRAxes[1] = touchPos.x * 2.0f - 1.0f;
            }
        }
    }
}
extern "C" JNIEXPORT void JNICALL Java_de_uni_1siegen_libqvr_QVRActivity_nativeOnSurfaceCreated()
{
    // This is called from the Android thread.
    gvr_initialize_gl(QVRGoogleVR);
    QVR_DEBUG("Google VR: initialized GL");
    QVRGoogleVRViewportList = gvr_buffer_viewport_list_create(QVRGoogleVR);
    gvr_get_recommended_buffer_viewports(QVRGoogleVR, QVRGoogleVRViewportList);
    gvr_buffer_viewport* leftEyeVP = gvr_buffer_viewport_create(QVRGoogleVR);
    gvr_buffer_viewport* rightEyeVP = gvr_buffer_viewport_create(QVRGoogleVR);
    gvr_buffer_viewport_list_get_item(QVRGoogleVRViewportList, 0, leftEyeVP);
    gvr_buffer_viewport_list_get_item(QVRGoogleVRViewportList, 1, rightEyeVP);
    gvr_rectf leftEyeVPUV = gvr_buffer_viewport_get_source_uv(leftEyeVP);
    QVRGoogleVRRelativeViewports[0] = QRectF(leftEyeVPUV.left, leftEyeVPUV.bottom,
            leftEyeVPUV.right - leftEyeVPUV.left, leftEyeVPUV.top - leftEyeVPUV.bottom);
    QVR_DEBUG("Google VR: left eye viewport x=%g y=%g w=%g h=%g",
            QVRGoogleVRRelativeViewports[0].x(), QVRGoogleVRRelativeViewports[0].y(),
            QVRGoogleVRRelativeViewports[0].width(), QVRGoogleVRRelativeViewports[0].height());
    gvr_rectf rightEyeVPUV = gvr_buffer_viewport_get_source_uv(rightEyeVP);
    QVRGoogleVRRelativeViewports[1] = QRectF(rightEyeVPUV.left, rightEyeVPUV.bottom,
            rightEyeVPUV.right - rightEyeVPUV.left, rightEyeVPUV.top - rightEyeVPUV.bottom);
    QVR_DEBUG("Google VR: right eye viewport: x=%g y=%g w=%g h=%g",
            QVRGoogleVRRelativeViewports[1].x(), QVRGoogleVRRelativeViewports[1].y(),
            QVRGoogleVRRelativeViewports[1].width(), QVRGoogleVRRelativeViewports[1].height());
    gvr_sizei renderTargetSize = gvr_get_maximum_effective_render_target_size(QVRGoogleVR);
    QVRGoogleVRTexSize = QSize(
            QVRGoogleVRRelativeViewports[0].width() * renderTargetSize.width,
            QVRGoogleVRRelativeViewports[0].height() * renderTargetSize.height);
    QVR_DEBUG("Google VR: recommended texture size for each eye: %dx%d",
            QVRGoogleVRTexSize.width(), QVRGoogleVRTexSize.height());
    QVRGoogleVRTexSize = QVRGoogleVRTexSize * QVRGoogleVRResolutionFactor;
    QVR_DEBUG("Google VR: actual texture size for each eye: %dx%d",
            QVRGoogleVRTexSize.width(), QVRGoogleVRTexSize.height());
    // Note that the swap chain is created for the recommended texture size, even if the actual
    // texture size is smaller due to QVRGoogleVRResolutionFactor. This avoids ugly artefacts.
    gvr_buffer_spec* bufferSpec = gvr_buffer_spec_create(QVRGoogleVR);
    gvr_buffer_spec_set_size(bufferSpec, renderTargetSize);
    gvr_buffer_spec_set_samples(bufferSpec, 1);
    gvr_buffer_spec_set_color_format(bufferSpec, GVR_COLOR_FORMAT_RGBA_8888);
    gvr_buffer_spec_set_depth_stencil_format(bufferSpec, GVR_DEPTH_STENCIL_FORMAT_NONE);
    QVRGoogleVRSwapChain = gvr_swap_chain_create(QVRGoogleVR, const_cast<const gvr_buffer_spec**>(&bufferSpec), 1);
    QVR_DEBUG("Google VR: created swap chain for %dx%d buffers",
            renderTargetSize.width, renderTargetSize.height);
    // Compute frustum l,r,b,t for each eye here since these values will not change
    for (int i = 0; i < 2; i++) {
        gvr_rectf fov = gvr_buffer_viewport_get_source_fov(i == 0 ? leftEyeVP : rightEyeVP);
        QVRGoogleVRlrbt[i][0] = -std::tan(qDegreesToRadians(fov.left));
        QVRGoogleVRlrbt[i][1] =  std::tan(qDegreesToRadians(fov.right));
        QVRGoogleVRlrbt[i][2] = -std::tan(qDegreesToRadians(fov.bottom));
        QVRGoogleVRlrbt[i][3] =  std::tan(qDegreesToRadians(fov.top));
        QVR_DEBUG("Google VR: %s eye frustum for near=1: l=%g r=%g b=%g t=%g",
                i == 0 ? "left" : "right",
                QVRGoogleVRlrbt[i][0], QVRGoogleVRlrbt[i][1], QVRGoogleVRlrbt[i][2], QVRGoogleVRlrbt[i][3]);
    }
    // Get transformation between eye and head since these values will not change
    for (int i = 0; i < 2; i++) {
        gvr_mat4f eyeMatrix = gvr_get_eye_from_head_matrix(QVRGoogleVR, i);
        QVRMatrixToPose(QMatrix4x4(reinterpret_cast<const float*>(eyeMatrix.m)),
                &(QVRGoogleVREyeFromHeadOrientations[i]), &(QVRGoogleVREyeFromHeadPositions[i]));
        QVRGoogleVREyeFromHeadOrientations[i] = QVRGoogleVREyeFromHeadOrientations[i].inverted();
        QVRGoogleVREyeFromHeadPositions[i] = -QVRGoogleVREyeFromHeadPositions[i];
        QVector3D axis;
        float angle;
        QVRGoogleVREyeFromHeadOrientations[i].getAxisAndAngle(&axis, &angle);
        QVR_DEBUG("Google VR: %s eye relative to head: position %g %g %g, orientation %g degrees around %g %g %g",
                i == 0 ? "left" : "right",
                QVRGoogleVREyeFromHeadPositions[i].x(), QVRGoogleVREyeFromHeadPositions[i].y(), QVRGoogleVREyeFromHeadPositions[i].z(),
                angle, axis.x(), axis.y(), axis.z());
    }
    // Initialize our own output code
    static const char* vertexShaderSource = "#version 300 es\n"
        "layout(location = 0) in vec4 position;\n"
        "layout(location = 1) in vec2 texcoord;\n"
        "smooth out vec2 vtexcoord;\n"
        "void main(void)\n"
        "{\n"
        "    vtexcoord = texcoord;\n"
        "    gl_Position = position;\n"
        "}\n";
    static const char* fragmentShaderSource = "#version 300 es\n"
        "uniform sampler2D tex;\n"
        "smooth in vec2 vtexcoord;\n"
        "layout(location = 0) out vec4 fcolor;\n"
        "void main(void)\n"
        "{\n"
        "    fcolor = vec4(texture(tex, vtexcoord).rgb, 1.0);\n"
        "}\n";
    static const float quadPositions[] = {
        -1.0f, -1.0f, 0.0f,  +1.0f, -1.0f, 0.0f,
        +1.0f, +1.0f, 0.0f,  -1.0f, +1.0f, 0.0f
    };
    static const float quadTexcoords[] = {
        0.0f, 0.0f,  1.0f, 0.0f,
        1.0f, 1.0f,  0.0f, 1.0f
    };
    static unsigned int quadIndices[] = {
        0, 1, 3, 1, 2, 3
    };
    glGenVertexArrays(1, &QVRGoogleVRVao);
    glBindVertexArray(QVRGoogleVRVao);
    GLuint positionBuffer;
    glGenBuffers(1, &positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
    glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(QVector3D), quadPositions, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    GLuint texcoordBuffer;
    glGenBuffers(1, &texcoordBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, texcoordBuffer);
    glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(QVector2D), quadTexcoords, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);
    GLuint indexBuffer;
    glGenBuffers(1, &indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), quadIndices, GL_STATIC_DRAW);
    glBindVertexArray(0);
    QVR_DEBUG("Google VR: created output quad vao");
    QVRGoogleVRPrg = glCreateProgram();
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    glAttachShader(QVRGoogleVRPrg, vertexShader);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glAttachShader(QVRGoogleVRPrg, fragmentShader);
    QVR_DEBUG("Google VR: created output program");
    glLinkProgram(QVRGoogleVRPrg);
    QVR_DEBUG("Google VR: linked output program");
}
extern "C" JNIEXPORT void JNICALL Java_de_uni_1siegen_libqvr_QVRActivity_nativeOnDrawFrame()
{
    // This is called from the Android thread.
    while (QVRGoogleVRSync.load() != 1)
        QThread::yieldCurrentThread();

    gvr_frame* frame = gvr_swap_chain_acquire_frame(QVRGoogleVRSwapChain);
    gvr_sizei frameSize = gvr_frame_get_buffer_size(frame, 0);
    gvr_frame_bind_buffer(frame, 0);

    glBindVertexArray(QVRGoogleVRVao);
    glUseProgram(QVRGoogleVRPrg);
    glUniform1i(glGetUniformLocation(QVRGoogleVRPrg, "tex"), 0);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);

    glViewport(0, 0, frameSize.width, frameSize.height);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindTexture(GL_TEXTURE_2D, QVRGoogleVRTextures[0]);
    glViewport(
            QVRGoogleVRRelativeViewports[0].x() * frameSize.width,
            QVRGoogleVRRelativeViewports[0].y() * frameSize.height,
            QVRGoogleVRRelativeViewports[0].width() * frameSize.width,
            QVRGoogleVRRelativeViewports[0].height() * frameSize.height);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindTexture(GL_TEXTURE_2D, QVRGoogleVRTextures[1]);
    glViewport(
            QVRGoogleVRRelativeViewports[1].x() * frameSize.width,
            QVRGoogleVRRelativeViewports[1].y() * frameSize.height,
            QVRGoogleVRRelativeViewports[1].width() * frameSize.width,
            QVRGoogleVRRelativeViewports[1].height() * frameSize.height);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    gvr_frame_unbind(frame);
    gvr_frame_submit(&frame, QVRGoogleVRViewportList, QVRGoogleVRHeadMatrix);

    QVRGoogleVRSync.store(2);
}
extern "C" JNIEXPORT void JNICALL Java_de_uni_1siegen_libqvr_QVRActivity_nativeOnTriggerEvent()
{
    // This is called from the Android thread. The event is consumed by a QVRDevice (see there).
    QVRGoogleVRTouchEvent.store(1);
}
#endif
