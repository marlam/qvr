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

project(qvr-vncviewer)

find_package(Qt5 5.6.0 COMPONENTS Gui)
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
qt5_add_resources(RESOURCES resources.qrc)
add_executable(qvr-vncviewer qvr-vncviewer.cpp qvr-vncviewer.hpp ${RESOURCES})
set_target_properties(qvr-vncviewer PROPERTIES WIN32_EXECUTABLE TRUE)
target_link_libraries(qvr-vncviewer ${QVR_LIBRARIES} Qt5::Gui ${LIBVNCCLIENT_LIBRARIES})
install(TARGETS qvr-vncviewer RUNTIME DESTINATION bin)
