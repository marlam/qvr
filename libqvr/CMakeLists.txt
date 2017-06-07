# Copyright (C) 2016, 2017
# Computer Graphics Group, University of Siegen
# Written by Martin Lambers <martin.lambers@uni-siegen.de>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice and this
# notice are preserved. This file is offered as-is, without any warranty.

cmake_minimum_required(VERSION 3.4)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR} ${CMAKE_MODULE_PATH})
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Project
project(libqvr)
set(QVR_VERSION 1.1.0)
set(QVR_LIBVERSION 2.0.0)
set(QVR_SOVERSION 2)

# Build options
option(QVR_BUILD_DOCUMENTATION "Build API reference documentation (requires Doxygen)" OFF)

# Compiler and system
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall -Wextra")
endif()
if(ANDROID)
    # Work around missing defines in QOpenGLExtraFunctions
    add_definitions(-DGL_TEXTURE_WIDTH=0x1000)
    add_definitions(-DGL_TEXTURE_HEIGHT=0x1001)
    add_definitions(-DGL_SRGB8_ALPHA8=0x8C43)
    add_definitions(-DGL_RGBA8=0x8058)
endif()

# Required libraries
find_package(Qt5Widgets 5.6.0 REQUIRED)
find_package(Qt5Network 5.6.0 REQUIRED)

# Optional libraries
find_package(Qt5Gamepad 5.7.0)
find_package(VRPN)
find_package(Oculus)
find_package(OpenVR)
find_package(OSVRCore)
find_package(OSVRRenderManager)

# The QVR library
qt5_add_resources(QVRRESOURCES qvr.qrc)
add_library(libqvr SHARED
    manager.hpp manager.cpp
    config.hpp config.cpp
    device.hpp device.cpp
    observer.hpp observer.cpp
    window.hpp window.cpp
    process.hpp process.cpp
    ipc.hpp ipc.cpp
    internalglobals.hpp internalglobals.cpp
    logging.hpp logging.cpp
    event.hpp event.cpp
    rendercontext.hpp rendercontext.cpp
    frustum.hpp frustum.cpp
    ${QVRRESOURCES})
set_target_properties(libqvr PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
set_target_properties(libqvr PROPERTIES OUTPUT_NAME qvr)
set_target_properties(libqvr PROPERTIES VERSION ${QVR_LIBVERSION})
set_target_properties(libqvr PROPERTIES SOVERSION ${QVR_SOVERSION})
target_link_libraries(libqvr Qt5::Widgets Qt5::Network)
if(Qt5Gamepad_FOUND)
    add_definitions(-DHAVE_QGAMEPAD)
    target_link_libraries(libqvr Qt5::Gamepad)
endif()
if(VRPN_FOUND)
    add_definitions(-DHAVE_VRPN)
    include_directories(${VRPN_INCLUDE_DIRS})
    target_link_libraries(libqvr ${VRPN_LIBRARIES})
endif()
if(OCULUS_FOUND)
    add_definitions(-DHAVE_OCULUS)
    include_directories(${OCULUS_INCLUDE_DIRS})
    target_link_libraries(libqvr ${OCULUS_LIBRARIES})
endif()
if(OPENVR_FOUND)
    add_definitions(-DHAVE_OPENVR)
    include_directories(${OPENVR_INCLUDE_DIRS})
    target_link_libraries(libqvr ${OPENVR_LIBRARIES})
endif()
if(OSVRCORE_FOUND)
    if (OSVRRENDERMANAGER_FOUND)
        add_definitions(-DHAVE_OSVR)
        include_directories(${OSVRCORE_INCLUDE_DIRS} ${OSVRRENDERMANAGER_INCLUDE_DIRS})
        target_link_libraries(libqvr ${OSVRCORE_LIBRARIES} ${OSVRRENDERMANAGER_LIBRARIES})
    endif()
endif()
if(ANDROID)
    # Work around undefined references to GLES functions in QOpenGLFunctions
    target_link_libraries(libqvr -lGLESv3)
    # Link against the system logging library
    target_link_libraries(libqvr -llog)
endif()
install(TARGETS libqvr
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib${LIB_SUFFIX}
    ARCHIVE DESTINATION lib${LIB_SUFFIX}
)
install(FILES
    app.hpp
    manager.hpp
    config.hpp
    device.hpp
    observer.hpp
    window.hpp
    process.hpp
    rendercontext.hpp
    outputplugin.hpp
    frustum.hpp
    DESTINATION include/qvr)
include(CMakePackageConfigHelpers)
set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/include)
set(LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX})
configure_package_config_file(
    QVRConfig.cmake.in ${CMAKE_BINARY_DIR}/QVRConfig.cmake
    INSTALL_DESTINATION lib${LIB_SUFFIX}/cmake/QVR-{QVR_VERSION}
    PATH_VARS INCLUDE_INSTALL_DIR LIB_INSTALL_DIR
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)
write_basic_package_version_file(
    ${CMAKE_BINARY_DIR}/QVRConfigVersion.cmake
    VERSION ${QVR_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_BINARY_DIR}/QVRConfig.cmake
    ${CMAKE_BINARY_DIR}/QVRConfigVersion.cmake
    DESTINATION ${LIB_INSTALL_DIR}/cmake/QVR-${QVR_VERSION}
)

# Optional target: reference documentation
if(QVR_BUILD_DOCUMENTATION)
  find_package(Doxygen REQUIRED)
  configure_file("${CMAKE_SOURCE_DIR}/Doxyfile.in" "${CMAKE_BINARY_DIR}/Doxyfile" @ONLY)
  file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/html")
  add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/html/index.html"
    COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_BINARY_DIR}/Doxyfile"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    DEPENDS "${CMAKE_SOURCE_DIR}/Doxyfile.in"
            "${CMAKE_SOURCE_DIR}/app.hpp"
	    "${CMAKE_SOURCE_DIR}/manager.hpp"
	    "${CMAKE_SOURCE_DIR}/config.hpp"
	    "${CMAKE_SOURCE_DIR}/device.hpp"
	    "${CMAKE_SOURCE_DIR}/observer.hpp"
	    "${CMAKE_SOURCE_DIR}/window.hpp"
	    "${CMAKE_SOURCE_DIR}/process.hpp"
	    "${CMAKE_SOURCE_DIR}/rendercontext.hpp"
	    "${CMAKE_SOURCE_DIR}/outputplugin.hpp"
            "${CMAKE_SOURCE_DIR}/frustum.hpp"
    COMMENT "Generating API documentation with Doxygen" VERBATIM
  )
  add_custom_target(doc ALL DEPENDS "${CMAKE_BINARY_DIR}/html/index.html")
  install(DIRECTORY "${CMAKE_BINARY_DIR}/html" DESTINATION share/doc/libqvr)
endif()
