# Copyright (C) 2017, 2018, 2019, 2020, 2021, 2022
# Computer Graphics Group, University of Siegen
# Written by Martin Lambers <martin.lambers@uni-siegen.de>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice and this
# notice are preserved. This file is offered as-is, without any warranty.

cmake_minimum_required(VERSION 3.4)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR} ${CMAKE_MODULE_PATH})
set(CMAKE_FIND_PACKAGE_SORT_ORDER NATURAL)
set(CMAKE_FIND_PACKAGE_SORT_DIRECTION DEC)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

project(qvr-videoplayer)

find_package(Qt6 6.2.0 COMPONENTS OpenGL Multimedia)
find_package(QVR REQUIRED)

include_directories(${QVR_INCLUDE_DIRS})
link_directories(${QVR_LIBRARY_DIRS})
qt6_add_resources(RESOURCES resources.qrc)
add_executable(qvr-videoplayer
    qvr-videoplayer.cpp qvr-videoplayer.hpp
    screen.cpp screen.hpp
    tiny_obj_loader.cc tiny_obj_loader.h
    ${RESOURCES})
set_target_properties(qvr-videoplayer PROPERTIES WIN32_EXECUTABLE TRUE)
target_link_libraries(qvr-videoplayer ${QVR_LIBRARIES} Qt6::OpenGL Qt6::Multimedia)
install(TARGETS qvr-videoplayer RUNTIME DESTINATION bin)
