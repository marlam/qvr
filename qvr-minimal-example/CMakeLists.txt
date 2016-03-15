# Copyright (C) 2016
# Computer Graphics Group, University of Siegen
# Written by Martin Lambers <martin.lambers@uni-siegen.de>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice and this
# notice are preserved. This file is offered as-is, without any warranty.

cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR} ${CMAKE_MODULE_PATH})
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall -Wextra")
endif()

find_package(Qt5Widgets REQUIRED)
find_package(QVR REQUIRED)

qt5_add_resources(RESOURCES resources.qrc)
add_executable(qvr-minimal-example
    qvr-minimal-example.cpp qvr-minimal-example.hpp
    ${RESOURCES})
include_directories(${QVR_INCLUDE_DIRS})
target_link_libraries(qvr-minimal-example ${QVR_LIBRARIES} Qt5::Widgets)
install(TARGETS qvr-minimal-example RUNTIME DESTINATION bin)
