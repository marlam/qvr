# Use CMake instead of this qmake .pro file to build libqvr!
#
# This file is only intended to be used for Android apps.
#
# Use it as follows:
# - Edit lib.path and headers.path at the bottom of this file
# - Run the following commands (with suitable paths):
#   $ export ANDROID_NDK_ROOT=/path/to/ndk-bundle
#   $ /path/to/qmake
#   $ make install_lib install_headers
# You now have a static libqvr for Android installed in a directory of your
# choice. Now set up your app to build and link against this library.
# See qvr-example-opengl for an example.

QT += widgets network gamepad

TARGET = qvr

TEMPLATE = lib

CONFIG += staticlib c++11

DEFINES += QT_DEPRECATED_WARNINGS \
	HAVE_QGAMEPAD \
	GL_SRGB8_ALPHA8=0x8C43   \
	GL_RGBA8=0x8058

SOURCES += \
	manager.cpp \
	config.cpp \
	device.cpp \
	observer.cpp \
	window.cpp \
	process.cpp \
	ipc.cpp \
	internalglobals.cpp \
	logging.cpp \
	event.cpp \
	rendercontext.cpp \
	frustum.cpp

HEADERS += \
	manager.hpp \
	config.hpp \
	device.hpp \
	observer.hpp \
	window.hpp \
	process.hpp \
	ipc.hpp \
	internalglobals.hpp \
	logging.hpp \
	event.hpp \
	rendercontext.hpp \
	frustum.hpp

RESOURCES += qvr.qrc

lib.path = /var/tmp/libqvr-android/lib
lib.files = $$OUT_PWD/libqvr.a
headers.path = /var/tmp/libqvr-android/include/qvr
headers.files = app.hpp manager.hpp config.hpp device.hpp observer.hpp window.hpp process.hpp rendercontext.hpp outputplugin.hpp frustum.hpp
INSTALLS += lib headers
