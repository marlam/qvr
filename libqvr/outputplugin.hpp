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

#ifndef QVR_OUTPUT_PLUGIN_HPP
#define QVR_OUTPUT_PLUGIN_HPP

class QStringList;
class QVRWindow;
class QVRRenderContext;

/*!
 * \brief Initialize an output plugin.
 * @param window        The window that provides the output
 * @param args          Plugin argument list from the configuration file
 *
 * See also \a QVROutputPluginExit() and \a QVROutputPlugin().
 */
extern "C" bool QVROutputPluginInit(QVRWindow* window, const QStringList& args);

/*!
 * \brief Cleanup an output plugin.
 * @param window        The window that provides the output
 *
 * See also \a QVROutputPluginInit() and \a QVROutputPlugin().
 */
extern "C" void QVROutputPluginExit(QVRWindow* window);

/*!
 * \brief Output the rendered content.
 * @param window        The window that provides the output
 * @param context       The render context of the output
 * @param tex0          The output of view pass 0
 * @param tex1          The output of view pass 1 (if any)
 *
 * This is the main function of an output plugin. It takes the rendered output
 * from the textures \a tex0 and \a tex1 and displays it to the output device,
 * applying arbitrary postprocessing.
 *
 * The \a context provides all information available about the output. Note that
 * \a tex1 is only valid if the output mode of \a window has two view passes.
 * This can be checked with \a QVRRenderContex::viewPasses().
 *
 * See also \a QVROutputPluginInit() and \a QVROutputPluginExit().
 */
extern "C" void QVROutputPlugin(QVRWindow* window,
        const QVRRenderContext& context,
        unsigned int tex0, unsigned int tex1);

#endif
