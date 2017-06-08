# Use CMake instead of this qmake .pro file to build libqvr!
#
# This file is only intended to be used for Android apps.
#
# Use it as follows:
# - First build a static libqvr for Android and install it in a directory of
#   your choice. See ../libqvr/libqvr.pro for instructions.
# - Edit INCLUDEPATH and LIBS below to match that directory.
# - Use Qt Creator to open this project file.
# - You should now be able to build and run this app.

QT += widgets network gamepad

TARGET = qvr-example-opengl

TEMPLATE = app

CONFIG += mobility c++11

DEFINES += QT_DEPRECATED_WARNINGS \
	GL_TEXTURE_WIDTH=0x1000  \
	GL_TEXTURE_HEIGHT=0x1001 \
	GL_RGBA8=0x8058

SOURCES += geometries.cpp qvr-example-opengl.cpp

HEADERS += geometries.hpp qvr-example-opengl.hpp

RESOURCES += resources.qrc

INCLUDEPATH += /var/tmp/libqvr-android/include
LIBS += -L/var/tmp/libqvr-android/lib -lqvr

MOBILITY = 
