# Use CMake instead of this qmake .pro file to build libqvr!
#
# This file is only intended to be used for Android apps.
#
# Use it as follows:
# - Adjust GOOGLEVRNDK_DIR and LIBQVR_DIR to match your needs.
# - Run the following commands (with suitable paths):
#   $ export ANDROID_NDK_ROOT=/path/to/ndk-bundle
#   $ /path/to/qmake
#   $ make install_lib install_headers
# You now have libqvr for Android installed in LIBQVR_DIR.
# Now set up your app to build and link against this library.
# See qvr-example-opengl for an example.

# Directory that contains the Google VR NDK
GOOGLEVRNDK_DIR = /var/tmp/googlevr-ndk
# Directory where libqvr will be installed (required for the app .pro file)
LIBQVR_DIR = /var/tmp/libqvr-android


QT += gui network gamepad androidextras

TARGET = qvr

TEMPLATE = lib

CONFIG += dll c++11

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

INCLUDEPATH += $$GOOGLEVRNDK_DIR/libraries/headers

LIBS += \
	$$GOOGLEVRNDK_DIR/libraries/jni/armeabi-v7a/libgvr.so \
	$$GOOGLEVRNDK_DIR/libraries/jni/armeabi-v7a/libgvr_audio.so

lib.path = $$LIBQVR_DIR/lib
lib.files = $$OUT_PWD/libqvr.so
INSTALLS += lib
headers.path = $$LIBQVR_DIR/include/qvr
headers.files = app.hpp manager.hpp config.hpp device.hpp observer.hpp window.hpp process.hpp rendercontext.hpp outputplugin.hpp frustum.hpp
INSTALLS += headers
