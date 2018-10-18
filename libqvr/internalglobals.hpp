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

#ifndef QVR_INTERNALS_HPP
#define QVR_INTERNALS_HPP

#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector3D>
#include <QQueue>
#include <QElapsedTimer>
#include <QList>
#include <QVector>
#include <QImage>
#include <QRect>
#include <QSizeF>

#include "event.hpp"
class QVRManager;


/* Global manager instance (singleton) */
extern QVRManager* QVRManagerInstance;

/* Global screen info */
extern int QVRScreenCount;
extern int QVRPrimaryScreen;
extern QVector<QRect> QVRScreenGeometries;
extern QVector<QSizeF> QVRScreenSizes;
void QVRGetScreenInfo();

/* Global helper functions */
void QVRMatrixToPose(const QMatrix4x4& matrix, QQuaternion* orientation, QVector3D* position);

/* Global event queue */
extern QQueue<QVREvent>* QVREventQueue;

/* Global timer */
extern QElapsedTimer QVRTimer;

/* Global renderable device model data */
extern QList<QVector<float>> QVRDeviceModelVertexPositions;
extern QList<QVector<float>> QVRDeviceModelVertexNormals;
extern QList<QVector<float>> QVRDeviceModelVertexTexCoords;
extern QList<QVector<unsigned short>> QVRDeviceModelVertexIndices;
extern QList<QImage> QVRDeviceModelTextures;

/* Global list of gamepads */
#ifdef HAVE_QGAMEPAD
extern QList<int> QVRGamepads;
void QVRDetectGamepads();
#endif

/* Global variables and functions for Oculus Rift support */
#ifdef HAVE_OCULUS
# include <OVR_Version.h>
# if (OVR_PRODUCT_VERSION < 1)
#  define isnan std::isnan
# endif
# include <OVR_CAPI.h>
# if (OVR_PRODUCT_VERSION < 1)
#  undef isnan
# endif
# if (OVR_PRODUCT_VERSION >= 1)
extern ovrSession QVROculus;
extern ovrGraphicsLuid QVROculusLuid;
extern ovrTextureSwapChain QVROculusTextureSwapChainL;
extern ovrTextureSwapChain QVROculusTextureSwapChainR;
extern long long QVROculusFrameIndex;
extern ovrLayerEyeFov QVROculusLayer;
extern ovrPosef QVROculusHmdToEyeViewPose[2];
extern ovrInputState QVROculusInputState;
# else
extern ovrHmd QVROculus;
# endif
extern ovrTrackingState QVROculusTrackingState;
extern ovrPosef QVROculusRenderPoses[2];
extern ovrEyeRenderDesc QVROculusEyeRenderDesc[2];
extern int QVROculusControllers; // 0 = none, 1 = xbox, 2 = left touch, 3 = right touch, 4 = both touch
void QVRAttemptOculusInitialization();
void QVRUpdateOculus();
#endif

/* Global variables and functions for OpenVR (HTC Vive) support */
#ifdef HAVE_OPENVR
# include <openvr.h>
extern vr::IVRSystem* QVROpenVRSystem;
extern unsigned int QVROpenVRControllerIndices[2];
extern vr::VRControllerState_t QVROpenVRControllerStates[2];
extern QQuaternion QVROpenVRTrackedOrientations[5];   // head, left eye, right eye, controller0, controller1
extern QVector3D QVROpenVRTrackedPositions[5];        // head, left eye, right eye, controller0, controller1
extern bool QVROpenVRHaveTrackedVelocities[5];        // head, left eye, right eye, controller0, controller1
extern QVector3D QVROpenVRTrackedVelocities[5];       // head, left eye, right eye, controller0, controller1
extern QVector3D QVROpenVRTrackedAngularVelocities[5];// head, left eye, right eye, controller0, controller1
extern QVector<QVector3D> QVROpenVRControllerModelPositions[2];
extern QVector<QQuaternion> QVROpenVRControllerModelOrientations[2];
extern QVector<int> QVROpenVRControllerModelVertexDataIndices[2];
extern QVector<int> QVROpenVRControllerModelTextureIndices[2];
void QVRAttemptOpenVRInitialization();
void QVRUpdateOpenVR();
#endif

/* Global variables and functions for Google VR support */
#ifdef ANDROID
# include <QAtomicInt>
# include <vr/gvr/capi/include/gvr.h>
# include <vr/gvr/capi/include/gvr_controller.h>
extern gvr_context* QVRGoogleVR;
extern gvr_controller_context* QVRGoogleVRController;
extern gvr_buffer_viewport_list* QVRGoogleVRViewportList;
extern gvr_swap_chain* QVRGoogleVRSwapChain;
extern float QVRGoogleVRResolutionFactor; // must be set by the window
extern QSize QVRGoogleVRTexSize;
extern QRectF QVRGoogleVRRelativeViewports[2]; // viewports inside buffer for each eye
extern float QVRGoogleVRlrbt[2][4]; // frustum l, r, b, t for each eye, at n=1
extern gvr_mat4f QVRGoogleVRHeadMatrix;
extern QVector3D QVRGoogleVRPositions[4];      // 0 = left eye, 1 = right eye, 2 = head, 3 = daydream controller
extern QQuaternion QVRGoogleVROrientations[4]; // 0 = left eye, 1 = right eye, 2 = head, 3 = daydream controller
extern QAtomicInt QVRGoogleVRTouchEvent;
extern bool QVRGoogleVRButtons[3];
extern float QVRGoogleVRAxes[2];
extern QAtomicInt QVRGoogleVRSync; // 0 = new frame, 1 = render to GVR, 2 = submit to GVR and swap
extern unsigned int QVRGoogleVRTextures[2];
void QVRAttemptGoogleVRInitialization();
void QVRUpdateGoogleVR();
#endif

#endif
