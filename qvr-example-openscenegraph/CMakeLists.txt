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

project(qvr-example-openscenegraph)

find_package(Qt6 6.2.0 COMPONENTS OpenGL)
find_package(QVR REQUIRED)
set(OpenSceneGraph_MARK_AS_ADVANCED ON)
find_package(OpenSceneGraph REQUIRED COMPONENTS osgViewer osgDB)

include_directories(${OPENSCENEGRAPH_INCLUDE_DIRS} ${QVR_INCLUDE_DIRS})
link_directories(${OPENSCENEGRAPH_LIBRARY_DIRS} ${QVR_LIBRARY_DIRS})
add_executable(qvr-example-openscenegraph qvr-example-openscenegraph.cpp qvr-example-openscenegraph.hpp)
set_target_properties(qvr-example-openscenegraph PROPERTIES WIN32_EXECUTABLE TRUE)
target_link_libraries(qvr-example-openscenegraph ${OPENSCENEGRAPH_LIBRARIES} ${QVR_LIBRARIES} Qt6::OpenGL)
install(TARGETS qvr-example-openscenegraph RUNTIME DESTINATION bin)
