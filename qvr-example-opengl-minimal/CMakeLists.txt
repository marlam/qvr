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

project(qvr-example-opengl-minimal)

find_package(Qt5 5.6.0 COMPONENTS Gui)
find_package(QVR REQUIRED)

include_directories(${QVR_INCLUDE_DIRS})
link_directories(${QVR_LIBRARY_DIRS})
qt5_add_resources(RESOURCES resources.qrc)
add_executable(qvr-example-opengl-minimal
    qvr-example-opengl-minimal.cpp qvr-example-opengl-minimal.hpp
    ${RESOURCES})
set_target_properties(qvr-example-opengl-minimal PROPERTIES WIN32_EXECUTABLE TRUE)
target_link_libraries(qvr-example-opengl-minimal ${QVR_LIBRARIES} Qt5::Gui)
install(TARGETS qvr-example-opengl-minimal RUNTIME DESTINATION bin)
