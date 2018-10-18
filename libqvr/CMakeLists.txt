# Copyright (C) 2016, 2017, 2018
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
set(QVR_VERSION 3.0.0)
set(QVR_LIBVERSION 3.0.0)
set(QVR_SOVERSION 3)

# Build options
option(QVR_BUILD_DOCUMENTATION "Build API reference documentation (requires Doxygen)" OFF)

# Required libraries
find_package(Qt5 5.6.0 COMPONENTS Gui Network OPTIONAL_COMPONENTS Gamepad)

# Optional libraries
find_package(VRPN QUIET)
find_package(Oculus QUIET)
find_package(OpenVR QUIET)

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
target_link_libraries(libqvr Qt5::Gui Qt5::Network)
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
