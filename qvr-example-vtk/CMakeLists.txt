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

project(qvr-example-vtk)

find_package(Qt5 5.6.0 COMPONENTS Gui)
find_package(QVR REQUIRED)
find_package(VTK 6.2 REQUIRED COMPONENTS vtkRenderingExternal NO_MODULE)
include(${VTK_USE_FILE})

include_directories(${VTK_INCLUDE_DIRS} ${QVR_INCLUDE_DIRS})
link_directories(${VTK_LIBRARY_DIRS} ${QVR_LIBRARY_DIRS})
add_executable(qvr-example-vtk qvr-example-vtk.cpp qvr-example-vtk.hpp)
set_target_properties(qvr-example-vtk PROPERTIES WIN32_EXECUTABLE TRUE)
target_link_libraries(qvr-example-vtk ${VTK_LIBRARIES} ${QVR_LIBRARIES} Qt5::Gui)
install(TARGETS qvr-example-vtk RUNTIME DESTINATION bin)
