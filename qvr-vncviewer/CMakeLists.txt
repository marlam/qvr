# Copyright (C) 2016, 2017, 2018, 2019, 2020, 2021, 2022
# Computer Graphics Group, University of Siegen
# Written by Martin Lambers <martin.lambers@uni-siegen.de>
# Copyright (C) 2024
# Martin Lambers <marlam@marlam.de>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice and this
# notice are preserved. This file is offered as-is, without any warranty.

cmake_minimum_required(VERSION 3.20)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR} ${CMAKE_MODULE_PATH})
set(CMAKE_FIND_PACKAGE_SORT_ORDER NATURAL)
set(CMAKE_FIND_PACKAGE_SORT_DIRECTION DEC)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

project(qvr-vncviewer)

find_package(Qt6 6.2.0 COMPONENTS OpenGL)
find_package(QVR REQUIRED)
find_package(PkgConfig)
pkg_check_modules(LIBVNCCLIENT REQUIRED libvncclient)

include(CheckStructHasMember)
check_struct_has_member("rfbClient" listen6Port rfb/rfbclient.h HAVE_RFBCLIENT_LISTEN6PORT)
if(HAVE_RFBCLIENT_LISTEN6PORT)
  add_definitions(-DHAVE_RFBCLIENT_LISTEN6PORT=1)
endif()

include_directories(${QVR_INCLUDE_DIRS} ${LIBVNCCLIENT_INCLUDE_DIRS})
link_directories(${QVR_LIBRARY_DIRS} ${LIBVNCCLIENT_LIBRARY_DIRS})
qt6_add_resources(RESOURCES resources.qrc)
add_executable(qvr-vncviewer qvr-vncviewer.cpp qvr-vncviewer.hpp ${RESOURCES})
set_target_properties(qvr-vncviewer PROPERTIES WIN32_EXECUTABLE TRUE)
target_link_libraries(qvr-vncviewer ${QVR_LIBRARIES} Qt6::OpenGL ${LIBVNCCLIENT_LIBRARIES})
install(TARGETS qvr-vncviewer RUNTIME DESTINATION bin)
