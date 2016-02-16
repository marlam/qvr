# QVR: a library for Virtual Reality applications

## Principle

QVR provides a base class for VR applications called QVRApp. An application
derived from QVRApp implements a set of functions (see below). It then creates
and initializes a QVRManager object and finally calls QApplication::exec() as
usual.

The QVRManager manages three basic types of objects: observers, processes, and
windows.

- Observers are the viewers of the virtual scene. Often there is only one.
- Processes all run the same application binary. They are automatically
  launched by QVRManager as needed. Different processes talk to different
  displays; if there is only one display, then no additional processes are
  needed. Processes can run on different nodes of a cluster.
- Windows are OpenGL windows (derived from QWindow with an OpenGL surface) that
  show the actual content on screen. They can be placed on different screens,
  with varying positions and sizes.

Each process talks to one display. All the windows of a process appear on that
display, but can appear on different screens. Each window has exactly one
observer. The content shown in a window shows the scene from the viewpoint
of that observer.

## Implementing QVRApp functions

You must implement `render()`. This function always requires you to render into
a texture, so the first thing you need to do is to set up a framebuffer
object. The `render()` function may be called more than once for each window,
e.g. for the left and right views of a stereo-3D window.

The other functions are all optional and are listed below, with decreasing
importance:
- `update()`  
  This function is called once for every frame. Use it to update your scene
  state, e.g. for animations etc.
  Optionally, this function allows to update custom tracked observers using
  tracking systems of your choice.
- `wantExit()`  
  This function is called once per frame to check if your application wants to
  quit.
- `keyPressEvent()`, `mousePressEvent()`, `mouseMoveEvent()` etc.  
  These functions handle Qt events. Events from all windows of all processes
  will be sent to your application. You can access information about the
  originating window and process.
- `serializeDynamicData()`, `deserializeDynamicData()`  
  These functions are only required if you want your application to support
  configurations with multiple processes. Use them to serialize your
  application scene state.
- `serializeStaticData()`, `deserializeStaticData()`  
  These functions offer an optimization for serialization. You can serialize
  just the static parts of your application state with them (i.e. the parts that
  do not change after initialization). This avoids sending those parts to
  additional processes with each new frame.
- `preRenderWindow()`, `postRenderWindow()`  
  These functions are called once per frame for every window, before rendering
  the frame and after rendering it. You can do per-window per-frame
  initializations and cleanups here, or screen-space post-processing.
- `preRenderProcess()`, `postRenderProcess()`  
  These functions are called once per frame for every process, before rendering
  the frame and after rendering it. You can do per-process per-frame
  initializations and cleanups here.
- `initWindow()`, `exitWindow()`  
  These functions are called once per window, before rendering the first frame
  and after rendering the last. You can do per-window initializations and
  cleanups here.
- `initProcess()`, `exitProcess()`  
  These functions are called once per process, before rendering the first
  frame and after rendering the last. You can do per-process initializations
  and cleanups here.
- `getNearFar()`  
  Use this to tell QVR which near and far plane values you like to have
  in the frustum passed to render().

Note: the application must not write to `stdout` and must not read from `stdin`.
These streams are used by the QVR library for inter-process communication.
Of course, writing messages to `stderr` is still possible.

## Launching QVR applications

Each QVR application supports the following command line options:
- `--qvr-log-level=fatal|warning|info|debug|firehose`  
  Set the log level.
- `--qvr-config=<configfile>`  
  Set the configuration file to use. If none is given, a simple default
  configuration will be created. If an Oculus Rift is detected, that default
  configuration will use it.
- `--qvr-fps=<milliseconds>`  
  Print an FPS measurement every x milliseconds.

## Configuration files

The configuration file always starts with a list of observers and their
properties. Currently three types of observers are implemented: stationary
observers (which do not move), custom observers (which are updated by the
application) and observers tracked by the Oculus Rift.

After that, the list of processes starts. There is always at least one master
process. The master process always uses the display it was started with (i.e.
the one in the DISPLAY environment variable or the -display command line
option), it cannot be configured to use a different display.

Each process contains a list of windows. Windows provide views into the virtual
world. The geometry of these windows must be known so that correct frusta and
viewing matrices can be computed. The geometry can be given either by
specifying the screen center location relative to the observer and deriving
the rest of the geometry automatically from the known screen dimensions, or by
explicitly defining the screen geometry in the virtual world space.

Observer properties:
- `type <static|oculus|custom>`  
  Static observers stay put, Oculus observers are updated automatically,
  custom observers use a tracking system of your choice.
- `eye_distance <meters>`  
  Interpupillary distance.
- `position <x> <y> <z>`  
  Initial position of the observer.
- `forward <x> <y> <z>`  
  Initial forward (or viewing) direction of the observer.
- `up <x> <y> <z>`  
  Initial up direction of the observer.

Process properties:
- `display <name>`  
  Display that this process talks to. For Linux/X11, this is usually something
  like :0, :1, or :0.2.
- `launcher <prg-and-args>`  
  Launcher process used to start this process. Only required when processes
  need to run on remote computers. Example for ssh (passwordless login to
  remotehost must be set up):
  launcher ssh remotehost
  Example for ssh if you use libraries in non-standard locations:
  `launcher ssh remotehost env LD_LIBRARY_PATH=/path/to/libs`

Window properties:
- `observer <id>`  
  The observer that this window provides a view into the virtual scene for.
  Must be one of the observers defined at the beginning of the configuration
  file.
- `stereo_mode <none|gl|left|right|red_cyan|green_magenta|amber_blue|oculus>`  
  Stereo-3D mode to use. None means no stereo (the observer only has a single
  eye); left and right only show the left eye / right eye view in this window,
  the color modes are for the corresponding anaglyph glasses, and oculus is
  for the Oculus Rift. Driver-supported OpenGL stereo (gl) is currently not
  implemented.
- `display_screen <screen>`  
  The display of the process may contain multiple screens. This option chooses
  one. The default screen is specified with -1.
- `fullscreen <true|false>`  
  Whether or not to show the window in fullscreen mode.
- `position <x> <y>`  
  Window position on screen, in pixels. The default is -1 -1, which means the
  window manager automatically places the window.
  This property is ignored for fullscreen windows and windows with `stereo_mode`
  oculus.
- `size <w> <h>`  
  Window size on screen, in pixels.
  This property is ignored for fullscreen windows and windows with `stereo_mode`
  oculus.
- `screen_is_fixed_to_observer <true|false>`  
  Whether this screen is fixed to the observer, like a head-mounted display,
  or it is fixed in virtual world space.
- `screen_wall <blx> <bly> <blz> <brx> <bry> <brz> <tlx> <tly> <tlz>`  
  The screen geometry defined by three points: bottom left corner, bottom
  right corner, and top left corner.
- `screen_is_given_by_center <true|false>`  
  Whether this screen is given only by its center (see next option). In this
  case, the full geometry is derived from querying the screen dimensions
  automatically.
- `screen_center <x> <y> <z>`  
  Center of the screen, usually given relative to the observer (i.e.
  `screen_is_fixed_to_observer` is true).
- `render_resolution_factor <factor>`  
  Optionally, the window can show scenes that were rendered at a different
  resolution than the actual window size. This is useful for example if a
  control window does not need to look good and can render at lower
  resolution. The window size will be multiplied by this factor to get the
  resolution of the rendering textures. Example: a 800x600 window with
  `render_resolution_factor` 0.5 will cause the application to render into a
  400x300 texture which is then upscaled to 800x600 for display.

## Implementation

The QVRManager creates an invisible main OpenGL context for each process. This
context is used for all calls to application functions: all application-side
rendering happens here. The rendering results are directed into a set of
textures that cover all the windows of the process.

When all window textures have been rendered, they are put on screen by the
actual visible windows who all have access to the textures from the main
window. The windows themselves render in separate rendering threads. On buffer
swap, only these rendering threads block. The main thread fills the waiting
time with CPU work such as processing events and calling the `update()` function
of the application.

The process initially started by the user is the master process. It will
launch slave processes as required by the configuration file, and it will
handle all necessary synchronization and data exchange with these slave
processes.

For each main object that QVRManager handles, there is both a configuration
class and an object class:
- QVRObserverConfig and QVRObserver
- QVRWindowConfig and QVRWindow
- QVRProcessConfig and QVRProcess
The properties of the configuration classes correspond 1:1 to the properties
that can be set in the configuration file.
In some situations, only the configuration of an object is available, but not
the object itself. An example is an event that was sent from a window of a
different process: on the master process, we only know the configuration of
the other process and its window, but not the objects themselves.

## Current limitations

- QVR is not very well tested, and only on Linux. However, there are no system
  dependencies in the code, so porting should be trivial.
- On Linux/X11, the Oculus Rift should be configured as a separate X11 screen,
  and the orientation of that screen should be fixed with
  `xrandr -display :0.1 -o left`
  The Oculus SDK 0.5 was tested (the latest version available on Linux).
- On Linux/X11, Qt can only show windows on additional display screens if
  those screens are part of the same virtual screen (the X11 display has
  one virtual screen that contains the actual screens; NVIDIA calls this
  configuration TwinView). If you configured separate X11 screens (e.g.
  :0.0, :0.1, :0.2), then Qt cannot properly show windows on them.
  A workaround is to use multiple QVR processes, one for each screen.
