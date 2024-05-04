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

project(qvr-sceneviewer)

find_package(Qt6 6.2.0 COMPONENTS OpenGL)
find_package(QVR REQUIRED)
find_package(assimp REQUIRED)

include_directories(${ASSIMP_INCLUDE_DIRS} ${QVR_INCLUDE_DIRS})
link_directories(${ASSIMP_LIBRARY_DIRS} ${QVR_LIBRARY_DIRS})
qt6_add_resources(RESOURCES resources.qrc)
add_executable(qvr-sceneviewer
    qvr-sceneviewer.cpp qvr-sceneviewer.hpp
    sceneviewer.cpp sceneviewer.hpp
    ${RESOURCES})
set_target_properties(qvr-sceneviewer PROPERTIES WIN32_EXECUTABLE TRUE)
target_link_libraries(qvr-sceneviewer ${ASSIMP_LIBRARIES} ${QVR_LIBRARIES} Qt6::OpenGL)
install(TARGETS qvr-sceneviewer RUNTIME DESTINATION bin)
