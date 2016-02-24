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

/* This file contains Doxygen documentation pages that do not fit into other files. */

/*!
 * \mainpage QVR: A library to build Virtual Reality applications
 *
 * \section Overview
 *
 * QVR provides the base class \a QVRApp for Virtual Reality applications.
 * An application implements the subset of \a QVRApp functions that it needs.
 * See \a QVRApp for function descriptions.
 *
 * To run a QVR application, the main() function creates and initializes a
 * \a QVRManager instances and then calls QApplication::exec() as usual. See
 * \a QVRManager for a minimal code example.
 *
 * The \a QVRManager manages three basic types of objects: observers, processes,
 * and windows:
 * - Observers view the virtual scene. Often there is only one observer.
 *   Observers are configured via \a QVRObserverConfig and implemented as \a QVRObserver.
 * - Processes all run the same application binary. The first process initially
 *   run by the user is the master process. Slave processes are automatically
 *   launched by QVRManager as needed. Each process is connected to one display
 *   which can have multiple screens attached. Processes can run on the same host
 *   or across a network. Often there is only the master process.
 *   Processes are configured via \a QVRProcessConfig and implemented as \a QVRProcess.
 * - Windows belong to processes and appear on the display that their process is
 *   connected to. They can have different positions and sizes on different screens
 *   attached to that display. Each window shows a view for exactly one observer
 *   (typically that observer views multiple windows).
 *   Windows are configured via \a QVRWindowConfig and implemented as \a QVRWindow.
 *
 * All QVR applications support a set of command line options. The most important
 * option lets the user choose a configuration file. See QVRManager::QVRManager().
 * The same application binary can run on different Virtual Reality display setups
 * by specifying different configuration files.
 *
 * To get started, see the main descriptions of \a QVRManager and \a QVRApp, and
 * then look at the source code of an example application to understand how it is
 * implemented.
 *
 * Note: your application must not write to stdout and must not read from stdin.
 * These streams are used by the QVR library for inter-process communication.
 * Of course, writing messages to stderr is still possible.
 *
 * \section conffile Configuration files
 *
 * A configuration file defines the observers, processes, and windows that the
 * \a QVRManager will manage.
 *
 * Each observer, process, and window definition is mapped directly to the
 * \a QVRObserverConfig, \a QVRProcessConfig, or \a QVRWindowConfig classes.
 *
 * A configuration file starts with a list of observers and their properties.
 *
 * After that, the list of processes starts. There is always at least one process:
 * the master process. The master process is connected to the display that Qt initially
 * uses (this can be changed with the -display option or DISPLAY environment
 * variable). Slave processes can connect to different displays. Optionally, a
 * launcher command can be specified, e.g. to run a slave process on a different
 * host using ssh.
 *
 * Each process definition contains a list of windows for that process.
 * Since windows provide views into the virtual world, the geometry of the
 * screen wall represented by a window must be known. This geometry is either
 * given by screen wall center coordinates, or by coordinates for three of its
 * corners.
 *
 * Please see the configuration file examples distributed with QVR to understand
 * how typical configurations look like.
 *
 * Observer definition (see \a QVRObserverConfig):
 * - `observer <id>`<br>
 *   Start a new observer definition with the given unique id.
 * - `type <static|wasdqe|vrpn|oculus|custom>`<br>
 *   Set the observer type.
 * - `parameters ...`<br>
 *   Set parameters for specific observer types.
 * - `eye_distance <meters>`<br>
 *   Set the interpupillary distance.
 * - `position <x> <y> <z>`<br>
 *   Set the initial position.
 * - `forward <x> <y> <z>`<br>
 *   Set the initial forward (or viewing) direction.
 * - `up <x> <y> <z>`<br>
 *   Set the initial up direction.
 *
 * Process definition (see \a QVRProcessConfig):
 * - `process <id>`<br>
 *   Start a new process definition with the given unique id.
 * - `display <name>`<br>
 *   Display that this process is connected to.
 * - `launcher <prg-and-args>`<br>
 *   Launcher commando used to start this process.
 *
 * Window definition (see \a QVRWindowConfig):
 * - `window <id>`<br>
 *   Start a new window definition with the given unique id, within the current process definition.
 * - `observer <id>`<br>
 *   Set the observer that this window provides a view for.
 * - `output <center|left|right|stereo> <gl|red_cyan|green_magenta|amber_blue|oculus|plugin>`<br>
 *   Set the output mode. Center, left, and right are monoscopic views, stereo is
 *   a stereoscopic view with one of the builtin types gl, red_cyan, green_magenta,
 *   amber_blue, oculus. Both monoscopic and stereoscopic views can also be handled
 *   by an output plugin. See \a QVROutputPlugin().
 * - `display_screen <screen>`<br>
 *   Select the Qt screen index on the Qt display that this process is connected to.<br>
 * - `fullscreen <true|false>`
 *   Whether to show the window in fullscreen mode.<br>
 * - `position <x> <y>`<br>
 *   Set the window position on the Qt screen in pixels.
 * - `size <w> <h>`<br>
 *   Set the window size on the Qt screen in pixels.
 * - `screen_is_fixed_to_observer <true|false>`<br>
 *   Whether the screen wall represented by this window is fixed to the observer,
 *   like a head-mounted display, or it is fixed in virtual world space.
 * - `screen_is_given_by_center <true|false>`<br>
 *   Whether the screen wall represented by this window is given by its center
 *   or by three of its corners.
 * - `screen_center <x> <y> <z>`<br>
 *   Set screen wall center coordinates.
 * - `screen_wall <blx> <bly> <blz> <brx> <bry> <brz> <tlx> <tly> <tlz>`<br>
 *   Set the screen wall geometry defined by three points: bottom left corner, bottom
 *   right corner, and top left corner.
 * - `render_resolution_factor <factor>`<br>
 *   Set the render resolution factor.
 *
 * \section Implementation
 *
 * The \a QVRManager creates an invisible main OpenGL context for each process. This
 * context is used for all calls to \a QVRApp application functions: the application
 * only has to deal with this one context. The rendering results are directed into a
 * set of textures that cover all the windows of the process.
 *
 * When all window textures have been rendered, they are put on screen by the
 * actual visible windows who all have access to the textures from the main
 * window. The windows themselves render in separate rendering threads. On buffer
 * swap, only these rendering threads block. The main thread fills the waiting
 * time with CPU work such as processing events and calling the \a QVRApp::update()
 * function of the application.
 *
 * The process initially started by the user is the master process. \a QVRManager
 * launch slave processes as required by the configuration file, and it will
 * handle all necessary synchronization and data exchange with these slave
 * processes.
 *
 * \section Notes
 *
 * - Oculus Rift: Currently the DK2 was tested on Linux with SDK 0.5. It should
 *   be configured as a separate X11 screen, and the orientation of that screen
 *   should be fixed with `xrandr -display :0.1 -o left`.
 * - Multiple screens on one Qt display: On Linux/X11, Qt can only show windows
 *   on additional display screens if those screens are part of the same virtual
 *   screen (the X11 display has one virtual screen that contains the actual
 *   screens; NVIDIA calls this TwinView). If you configured separate X11 screens
 *   (e.g. :0.0, :0.1, :0.2), then Qt cannot properly show windows on them.
 *   A workaround is to use multiple QVR processes, one for each screen.
 */
