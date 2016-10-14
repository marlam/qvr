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

#ifndef QVR_INTERNALS_HPP
#define QVR_INTERNALS_HPP

#ifdef HAVE_OCULUS
# define isnan std::isnan
# include <OVR.h>
# include <OVR_CAPI_GL.h>
extern ovrHmd QVROculus;
extern ovrEyeRenderDesc QVROculusEyeRenderDesc[2];
extern ovrPosef QVROculusRenderPoses[2];
extern ovrTrackingState QVROculusTrackingState;
void QVRAttemptOculusInitialization();
#endif

#ifdef HAVE_OSVR
# include <osvr/ClientKit/DisplayC.h>
# include <osvr/ClientKit/ContextC.h>
# include <osvr/RenderKit/RenderManagerOpenGLC.h>
extern OSVR_ClientContext QVROsvrClientContext;
extern OSVR_DisplayConfig QVROsvrDisplayConfig;
extern OSVR_RenderManager QVROsvrRenderManager;
extern OSVR_RenderManagerOpenGL QVROsvrRenderManagerOpenGL;
void QVRAttemptOSVRInitialization();
#endif

#endif
