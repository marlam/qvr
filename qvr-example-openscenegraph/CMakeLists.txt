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

project(qvr-example-openscenegraph)

find_package(Qt5 5.6.0 COMPONENTS Gui)
find_package(QVR REQUIRED)
set(OpenSceneGraph_MARK_AS_ADVANCED ON)
find_package(OpenSceneGraph REQUIRED COMPONENTS osgViewer osgDB)

include_directories(${OPENSCENEGRAPH_INCLUDE_DIRS} ${QVR_INCLUDE_DIRS})
link_directories(${OPENSCENEGRAPH_LIBRARY_DIRS} ${QVR_LIBRARY_DIRS})
add_executable(qvr-example-openscenegraph qvr-example-openscenegraph.cpp qvr-example-openscenegraph.hpp)
set_target_properties(qvr-example-openscenegraph PROPERTIES WIN32_EXECUTABLE TRUE)
target_link_libraries(qvr-example-openscenegraph ${OPENSCENEGRAPH_LIBRARIES} ${QVR_LIBRARIES} Qt5::Gui)
install(TARGETS qvr-example-openscenegraph RUNTIME DESTINATION bin)
