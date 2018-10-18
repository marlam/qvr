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

project(qvr-example-outputplugin)

find_package(Qt5 5.6.0 COMPONENTS Widgets)
find_package(QVR REQUIRED)

include_directories(${QVR_INCLUDE_DIRS})
link_directories(${QVR_LIBRARY_DIRS})
qt5_add_resources(RESOURCES qvr-example-outputplugin.qrc)
add_library(libqvr-example-outputplugin SHARED
    qvr-example-outputplugin.cpp qvr-example-outputplugin.hpp ${RESOURCES})
set_target_properties(libqvr-example-outputplugin
    PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
set_target_properties(libqvr-example-outputplugin
    PROPERTIES OUTPUT_NAME qvr-example-outputplugin)
target_link_libraries(libqvr-example-outputplugin
    ${QVR_LIBRARIES} Qt5::Widgets)
install(TARGETS libqvr-example-outputplugin
    LIBRARY DESTINATION "lib${LIB_SUFFIX}"
    ARCHIVE DESTINATION "lib${LIB_SUFFIX}"
)
