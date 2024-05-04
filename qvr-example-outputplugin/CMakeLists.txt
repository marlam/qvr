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

project(qvr-example-outputplugin)

find_package(Qt6 6.2.0 COMPONENTS OpenGL)
find_package(QVR REQUIRED)

include_directories(${QVR_INCLUDE_DIRS})
link_directories(${QVR_LIBRARY_DIRS})
qt6_add_resources(RESOURCES qvr-example-outputplugin.qrc)
add_library(libqvr-example-outputplugin SHARED
    qvr-example-outputplugin.cpp qvr-example-outputplugin.hpp ${RESOURCES})
set_target_properties(libqvr-example-outputplugin
    PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
set_target_properties(libqvr-example-outputplugin
    PROPERTIES OUTPUT_NAME qvr-example-outputplugin)
target_link_libraries(libqvr-example-outputplugin
    ${QVR_LIBRARIES} Qt6::OpenGL)
install(TARGETS libqvr-example-outputplugin
    LIBRARY DESTINATION "lib${LIB_SUFFIX}"
    ARCHIVE DESTINATION "lib${LIB_SUFFIX}"
)
