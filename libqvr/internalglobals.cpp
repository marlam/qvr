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

#include <cstring>

#include "internalglobals.hpp"
#include "logging.hpp"

#ifdef HAVE_OSVR
# include <osvr/ClientKit/ServerAutoStartC.h>
#endif

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
    QVRGamepads = QGamepadManager::instance()->connectedGamepads();
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
ovrVector3f QVROculusHmdToEyeViewOffset[2];
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
    QVROculusHmdToEyeViewOffset[0] = QVROculusEyeRenderDesc[0].HmdToEyeOffset;
    QVROculusHmdToEyeViewOffset[1] = QVROculusEyeRenderDesc[1].HmdToEyeOffset;
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
static void QVROpenVRConvertPose(const QMatrix4x4& matrix, QQuaternion* orientation, QVector3D* position)
{
    *orientation = QQuaternion::fromRotationMatrix(matrix.toGenericMatrix<3, 3>());
    *position = QVector3D(matrix(0, 3), matrix(1, 3), matrix(2, 3));
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
    QVROpenVRSystem->CaptureInputFocus();
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
    QVROpenVRConvertPose(headMatrix, &(QVROpenVRTrackedOrientations[0]), &(QVROpenVRTrackedPositions[0]));
    QVROpenVRHaveTrackedVelocities[0] = true;
    QVROpenVRTrackedVelocities[0] = QVector3D(poses[vr::k_unTrackedDeviceIndex_Hmd].vVelocity.v[0],
                                              poses[vr::k_unTrackedDeviceIndex_Hmd].vVelocity.v[1],
                                              poses[vr::k_unTrackedDeviceIndex_Hmd].vVelocity.v[2]);
    QVROpenVRTrackedAngularVelocities[0] = QVector3D(poses[vr::k_unTrackedDeviceIndex_Hmd].vAngularVelocity.v[0],
                                                     poses[vr::k_unTrackedDeviceIndex_Hmd].vAngularVelocity.v[1],
                                                     poses[vr::k_unTrackedDeviceIndex_Hmd].vAngularVelocity.v[2]);
    // left eye:
    QMatrix4x4 leftEyeMatrix = headMatrix * QVROpenVRHmdToEye[0];
    QVROpenVRConvertPose(leftEyeMatrix, &(QVROpenVRTrackedOrientations[1]), &(QVROpenVRTrackedPositions[1]));
    QVROpenVRHaveTrackedVelocities[1] = false;
    // right eye:
    QMatrix4x4 rightEyeMatrix = headMatrix * QVROpenVRHmdToEye[1];
    QVROpenVRConvertPose(rightEyeMatrix, &(QVROpenVRTrackedOrientations[2]), &(QVROpenVRTrackedPositions[2]));
    QVROpenVRHaveTrackedVelocities[2] = false;
    // controller 0:
    QVROpenVRConvertPose(QVROpenVRConvertMatrix(poses[QVROpenVRControllerIndices[0]].mDeviceToAbsoluteTracking),
            &(QVROpenVRTrackedOrientations[3]), &(QVROpenVRTrackedPositions[3]));
    QVROpenVRHaveTrackedVelocities[3] = true;
    QVROpenVRTrackedVelocities[3] = QVector3D(poses[QVROpenVRControllerIndices[0]].vVelocity.v[0],
                                              poses[QVROpenVRControllerIndices[0]].vVelocity.v[1],
                                              poses[QVROpenVRControllerIndices[0]].vVelocity.v[2]);
    QVROpenVRTrackedAngularVelocities[3] = QVector3D(poses[QVROpenVRControllerIndices[0]].vAngularVelocity.v[0],
                                                     poses[QVROpenVRControllerIndices[0]].vAngularVelocity.v[1],
                                                     poses[QVROpenVRControllerIndices[0]].vAngularVelocity.v[2]);
    // controller 1:
    QVROpenVRConvertPose(QVROpenVRConvertMatrix(poses[QVROpenVRControllerIndices[1]].mDeviceToAbsoluteTracking),
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
                        QVROpenVRConvertPose(M, &q, &p);
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

/* Global variables and functions for OSVR support */
#ifdef HAVE_OSVR
OSVR_ClientContext QVROsvrClientContext = NULL;
OSVR_DisplayConfig QVROsvrDisplayConfig = NULL;
OSVR_RenderManager QVROsvrRenderManager = NULL;
OSVR_RenderManagerOpenGL QVROsvrRenderManagerOpenGL = NULL;
// to prevent the OSVR Render Manager from getting in our way, we have to make
// it use a dummy toolkit that does absolutely nothing.
static void osvrToolkitCreate(void*) {}
static void osvrToolkitDestroy(void*) {}
static OSVR_CBool osvrToolkitAddOpenGLContext(void*, const OSVR_OpenGLContextParams*) { return true; }
static OSVR_CBool osvrToolkitRemoveOpenGLContexts(void*) { return true; }
static OSVR_CBool osvrToolkitMakeCurrent(void*, size_t) { return true; }
static OSVR_CBool osvrToolkitSwapBuffers(void*, size_t) { return true; }
static OSVR_CBool osvrToolkitSetVerticalSync(void*, OSVR_CBool) { return true; }
static OSVR_CBool osvrToolkitHandleEvents(void*) { return true; }
static OSVR_CBool osvrToolkitGetDisplayFrameBuffer(void*, size_t, GLuint* fb) { *fb = 0; return true; }
static OSVR_CBool osvrToolkitGetDisplaySizeOverride(void*, size_t, int*, int*) { return false; }
static OSVR_OpenGLToolkitFunctions osvrToolkit;
void QVRAttemptOSVRInitialization()
{
    bool osvr = false;
    osvrClientAttemptServerAutoStart();
    QVROsvrClientContext = osvrClientInit("de.uni-siegen.informatik.cg.qvr");
    if (osvrClientGetDisplay(QVROsvrClientContext, &QVROsvrDisplayConfig) == OSVR_RETURN_SUCCESS) {
        QVR_INFO("OSVR: got display config");
        OSVR_DisplayInputCount numDisplayInputs;
        osvrClientGetNumDisplayInputs(QVROsvrDisplayConfig, &numDisplayInputs);
        if (numDisplayInputs != 1) {
            QVR_INFO("OSVR: needs more than one display inputs; QVR currently does not handle this");
        } else {
            OSVR_DisplayDimension w, h;
            osvrClientGetDisplayDimensions(QVROsvrDisplayConfig, 0, &w, &h);
            QVR_INFO("OSVR: display dimensions %dx%d", static_cast<int>(w), static_cast<int>(h));
            OSVR_ViewerCount viewers;
            osvrClientGetNumViewers(QVROsvrDisplayConfig, &viewers);
            if (viewers != 1) {
                QVR_INFO("OSVR: requires more than one viewer; QVR currently does not handle this");
            } else {
                OSVR_EyeCount eyes;
                osvrClientGetNumEyesForViewer(QVROsvrDisplayConfig, 0, &eyes);
                if (eyes != 2) {
                    QVR_INFO("OSVR: viewer has more than 2 eyes; QVR currently does not handle this");
                } else {
                    OSVR_SurfaceCount surfaces0, surfaces1 = 1;
                    osvrClientGetNumSurfacesForViewerEye(QVROsvrDisplayConfig, 0, 0, &surfaces0);
                    if (eyes > 1)
                        osvrClientGetNumSurfacesForViewerEye(QVROsvrDisplayConfig, 0, 1, &surfaces1);
                    if (surfaces0 != 1 || surfaces1 != 1) {
                        QVR_INFO("OSVR: more than one surface per eye; QVR currently does not handle this");
                    } else {
                        QVR_INFO("OSVR: display config is usable");
                        OSVR_GraphicsLibraryOpenGL library;
                        osvrToolkit.size = sizeof(OSVR_OpenGLToolkitFunctions);
                        osvrToolkit.data = NULL;
                        osvrToolkit.create = osvrToolkitCreate;
                        osvrToolkit.destroy = osvrToolkitDestroy;
                        osvrToolkit.addOpenGLContext = osvrToolkitAddOpenGLContext;
                        osvrToolkit.removeOpenGLContexts = osvrToolkitRemoveOpenGLContexts;
                        osvrToolkit.makeCurrent = osvrToolkitMakeCurrent;
                        osvrToolkit.swapBuffers = osvrToolkitSwapBuffers;
                        osvrToolkit.setVerticalSync = osvrToolkitSetVerticalSync;
                        osvrToolkit.handleEvents = osvrToolkitHandleEvents;
                        osvrToolkit.getDisplayFrameBuffer = osvrToolkitGetDisplayFrameBuffer;
                        osvrToolkit.getDisplaySizeOverride = osvrToolkitGetDisplaySizeOverride;
                        library.toolkit = &osvrToolkit;
                        if (osvrCreateRenderManagerOpenGL(QVROsvrClientContext, "OpenGL", library,
                                    &QVROsvrRenderManager, &QVROsvrRenderManagerOpenGL) != OSVR_RETURN_SUCCESS) {
                            QVR_INFO("OSVR: cannot create render manager");
                        } else if (osvrRenderManagerGetDoingOkay(QVROsvrRenderManager) != OSVR_RETURN_SUCCESS) {
                            QVR_INFO("OSVR: created render manager does not work");
                            osvrDestroyRenderManager(QVROsvrRenderManager);
                            QVROsvrRenderManager = NULL;
                        } else {
                            QVR_INFO("OSVR: render manager works");
                            osvr = true;
                        }
                    }
                }
            }
        }
    } else {
        QVR_INFO("OSVR: cannot get display config; server probably not running correctly");
    }
    if (osvr) {
        QVR_INFO("OSVR: waiting for context to become ready ... ");
        while (osvrClientCheckStatus(QVROsvrClientContext) != OSVR_RETURN_SUCCESS) {
            osvrClientUpdate(QVROsvrClientContext);
        }
        QVR_INFO("OSVR: ... context is ready");
        QVR_INFO("OSVR: waiting for display to become ready ... ");
        while (osvrClientCheckDisplayStartup(QVROsvrDisplayConfig) != OSVR_RETURN_SUCCESS) {
            osvrClientUpdate(QVROsvrClientContext);
        }
        QVR_INFO("OSVR: ... display is ready");
    } else {
        osvrClientShutdown(QVROsvrClientContext);
        QVROsvrDisplayConfig = NULL;
        QVROsvrClientContext = NULL;
    }
}
#endif
