# Use CMake instead of this qmake .pro file to build libqvr!
#
# This file is only intended to be used for Android apps.
#
# Use it as follows:
# - Build libqvr for Android. See libqvr.pro.
# - Adjust GOOGLEVRNDK_DIR and LIBQVR_DIR below to match the values
#   you used in libqvr.pro.
# - Use Qt Creator to open this app project file.
# - You should now be able to build and run this app.


# Directory that contains the Google VR NDK. Same as in libqvr.pro.
GOOGLEVRNDK_DIR = /var/tmp/googlevr-ndk
# Directory where libqvr will be installed (required for the app .pro file)
LIBQVR_DIR = /var/tmp/libqvr-android


QT += gui network gamepad androidextras

TARGET = qvr-example-opengl

TEMPLATE = app

CONFIG += mobility c++11

DEFINES += QT_DEPRECATED_WARNINGS \
	GL_RGBA8=0x8058 \
	GL_DEPTH_COMPONENT24=0x81A6

SOURCES += geometries.cpp qvr-example-opengl.cpp

HEADERS += geometries.hpp qvr-example-opengl.hpp

RESOURCES += resources.qrc

INCLUDEPATH += $$LIBQVR_DIR/include
LIBS += -L$$LIBQVR_DIR/lib -lqvr \
	-L$$GOOGLEVRNDK_DIR/libraries/jni/armeabi-v7a -lgvr -lgvr_audio

MOBILITY = 

DISTFILES += \
    android/AndroidManifest.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew \
    android/gradlew.bat

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

ANDROID_EXTRA_LIBS += \
    $$LIBQVR_DIR/lib/libqvr.so \
    $$GOOGLEVRNDK_DIR/libraries/jni/armeabi-v7a/libgvr.so \
    $$GOOGLEVRNDK_DIR/libraries/jni/armeabi-v7a/libgvr_audio.so
