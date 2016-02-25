# QVR: a library for Virtual Reality applications

## Introduction

QVR is a library that makes writing Virtual Reality (VR) applications very easy.
It is based on Qt and requires no other libraries.

QVR manages all graphics views required for a VR application, based on a
configuration for the target VR environment, from multi-node multi-GPU render
clusters to head-mounted displays such as the Oculus Rift. A simple change
of the configuration file make the same application run on a completely
different setup.

The application only needs to implement a single interface with just a few
easily understandable functions. All rendering happens in a single OpenGL
context from a single thread. There are no context switching and sharing
issues, and no threading problems!

QVR allows writing VR applications in pure OpenGL as well as using arbitrary
external rendering engines. Examples are provided.

## The library

Build and install the QVR library `libqvr` first. After that, you can build
and try the example applications.

See the [library documentation](https://marlam.github.io/qvr/html/).

## Example applications

- `qvr-helloworld`:
  a simple OpenGL-based demo with WASD-style navigation.

- `qvr-osgviewer`:
  a full-featured [OpenSceneGraph](http://www.openscenegraph.com)
  viewer with WASD-style navigation.

- `qvr-vtk-example`:
  a simple example of a [VTK](http://www.vtk.org) visualization pipeline within
  a QVR application.

- `qvr-vncviewer`: a [VNC](https://en.wikipedia.org/wiki/Virtual_Network_Computing)
  viewer. Put a remote desktop in your virtual world!
