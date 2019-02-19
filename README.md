# QVR: a library for Virtual Reality applications

## Introduction

QVR is a library that makes writing Virtual Reality (VR) applications very easy.
It is based on Qt and requires no other libraries.

QVR manages all graphics views required for a VR application, based on a
configuration for the target VR environment, from multi-node multi-GPU render
clusters to head-mounted displays such as the Oculus Rift. A simple change
of the configuration file makes the same application run on a completely
different setup.

The application only needs to implement a single interface with just a few
easily understandable functions. All rendering happens in a single OpenGL
context so that there are no context switching and sharing issues. Furthermore
the application does not need to care about threads, but is still free to
implement any threading model.

QVR allows writing VR applications in pure OpenGL as well as using arbitrary
external rendering engines. Examples are provided.

## Supported platforms and hardware

The library should work on every platform that has Qt (i.e. all the platforms).
Currently tested are Linux, Windows, and Android.

Supported VR hardware includes:

- Custom large-scale VR labs with multiple GPUs and/or render clusters

- HTC Vive and SteamVR with [OpenVR](https://github.com/ValveSoftware/openvr)

- Oculus Rift with [Oculus SDK](https://www.oculus.com/)

- Google Cardboard and Daydream with [Google VR NDK](https://developers.google.com/vr/android/ndk/gvr-ndk-overview)

- Almost all tracking and interaction hardware with [VRPN](http://vrpn.org/)

## The library

Build and install the QVR library `libqvr` first. After that, you can build
and try the example applications.

See the [library documentation](https://marlam.de/qvr/html/).

There is also an [introductory presentation](https://marlam.de/qvr/qvr-slides.pdf)
explaining the concepts of libqvr and how to use it, and a
[short paper](https://marlam.de/qvr/lambers2016qvr.pdf) from the Eurographics
2016 Education track explaining the motivation behind it.

## Example applications

- `qvr-example-opengl-minimal`:
  a minimal OpenGL-based example application that displays a rotating box.

- `qvr-example-opengl`:
  a simple demo scene based on OpenGL.

- `qvr-example-openscenegraph`:
  a full-featured [OpenSceneGraph](http://www.openscenegraph.com) viewer.

- `qvr-example-vtk`:
  a simple example of a [VTK](http://www.vtk.org) visualization pipeline within
  a QVR application.

- `qvr-sceneviewer`:
  a viewer for 3D models and scenes in many file formats.

- `qvr-videoplayer`:
  a video screen in the virtual world, for 2D and 3D videos.

- `qvr-vncviewer`: a [VNC](https://en.wikipedia.org/wiki/Virtual_Network_Computing)
  viewer. Put a remote desktop in your virtual world!

- `qvr-identify-displays`:
  a small utility to check the configuration and left/right channel separation.
