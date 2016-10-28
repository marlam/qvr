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

#include <cstring>

#include "internalglobals.hpp"
#include "logging.hpp"

#ifdef HAVE_OSVR
# include <osvr/ClientKit/ServerAutoStartC.h>
#endif


#ifdef HAVE_OCULUS
# if (OVR_PRODUCT_VERSION >= 1)
ovrSession QVROculus = NULL;
ovrGraphicsLuid QVROculusLuid;
ovrTextureSwapChain QVROculusTextureSwapChainL = 0;
ovrTextureSwapChain QVROculusTextureSwapChainR = 0;
long long QVROculusFrameIndex = 0;
ovrLayerEyeFov QVROculusLayer;
ovrVector3f QVROculusHmdToEyeViewOffset[2];
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
# else
#  define oculusHmdDesc (*QVROculus)
# endif
    QVR_DEBUG("Oculus: product name: %s", oculusHmdDesc.ProductName);
    QVR_DEBUG("Oculus: resolution: %dx%d", oculusHmdDesc.Resolution.w, oculusHmdDesc.Resolution.h);
# if (OVR_PRODUCT_VERSION < 1)
    // this is done automatically with newer SDKs
    ovrHmd_ConfigureTracking(QVROculus,
            ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection | ovrTrackingCap_Position,
            ovrTrackingCap_Orientation | ovrTrackingCap_Position);
# endif
}
#endif

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
static OSVR_OpenGLToolkitFunctions osvrToolkit = {
    .size = sizeof(OSVR_OpenGLToolkitFunctions),
    .data = NULL,
    .create = osvrToolkitCreate,
    .destroy = osvrToolkitDestroy,
    .addOpenGLContext = osvrToolkitAddOpenGLContext,
    .removeOpenGLContexts = osvrToolkitRemoveOpenGLContexts,
    .makeCurrent = osvrToolkitMakeCurrent,
    .swapBuffers = osvrToolkitSwapBuffers,
    .setVerticalSync = osvrToolkitSetVerticalSync,
    .handleEvents = osvrToolkitHandleEvents,
    .getDisplayFrameBuffer = osvrToolkitGetDisplayFrameBuffer,
    .getDisplaySizeOverride = osvrToolkitGetDisplaySizeOverride
};
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
