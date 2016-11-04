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
context from a single thread. There are no context switching and sharing
issues, and no threading problems!

QVR allows writing VR applications in pure OpenGL as well as using arbitrary
external rendering engines. Examples are provided.

## Supported platforms and hardware

The library should work on every platform that has Qt (i.e. all the platforms).
Currently tested are Linux and Windows.

Supported VR hardware includes:

- Custom room-scale VR labs with multiple GPUs and/or render clusters

- Oculus Rift with [Oculus SDK](https://www.oculus.com/)

- HTC Vive and SteamVR with [OpenVR](https://github.com/ValveSoftware/openvr)

- HDK and lots of other head-mounted displays with [OSVR](http://osvr.org/)

- Almost all tracking and interaction hardware with [VRPN](http://vrpn.org/)

- Desktop-based fake-VR for development and testing purposes

## The library

Build and install the QVR library `libqvr` first. After that, you can build
and try the example applications.

See the [library documentation](https://marlam.github.io/qvr/html/).

There is also an [introductory presentation](https://marlam.github.io/qvr/qvr-slides.pdf)
explaining the concepts of libqvr and how to use it, and a
[short paper](https://marlam.github.io/qvr/lambers2016qvr.pdf) from the Eurographics
2016 Education track explaining the motivation behind it.

## Example applications

- `qvr-minimal-example`:
  a minimal example application that displays a rotating box.

- `qvr-helloworld`:
  a simple demo scene based on OpenGL.

- `qvr-sceneviewer`:
  a viewer for 3D models and scenes in many file formats.

- `qvr-osgviewer`:
  a full-featured [OpenSceneGraph](http://www.openscenegraph.com) viewer.

- `qvr-vtk-example`:
  a simple example of a [VTK](http://www.vtk.org) visualization pipeline within
  a QVR application.

- `qvr-vncviewer`: a [VNC](https://en.wikipedia.org/wiki/Virtual_Network_Computing)
  viewer. Put a remote desktop in your virtual world!
