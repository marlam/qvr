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

# Required libraries
find_package(Qt5Widgets REQUIRED)

# Optional libraries
find_package(VRPN)
find_package(Oculus)

# The library
qt5_add_resources(QVRRESOURCES qvr.qrc)
add_library(libqvr SHARED
    manager.hpp manager.cpp
    config.hpp config.cpp
    observer.hpp observer.cpp
    window.hpp window.cpp
    process.hpp process.cpp
    logging.hpp logging.cpp
    event.hpp event.cpp
    ${QVRRESOURCES})
set_target_properties(libqvr PROPERTIES OUTPUT_NAME qvr)
set_target_properties(libqvr PROPERTIES VERSION "0.0.0")  
set_target_properties(libqvr PROPERTIES SOVERSION "0")  
target_link_libraries(libqvr Qt5::Widgets)
if(VRPN_FOUND)
    add_definitions(-DHAVE_VRPN)
    include_directories(${VRPN_INCLUDE_DIRS})
    target_link_libraries(libqvr ${VRPN_LIBRARIES})
endif()
if(OCULUS_FOUND)
    add_definitions(-DHAVE_OCULUS)
    include_directories(${OCULUS_INCLUDE_DIRS})
    target_link_libraries(libqvr ${OCULUS_LIBRARIES})
endif()
install(TARGETS libqvr
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION "lib${LIB_SUFFIX}"
    ARCHIVE DESTINATION "lib${LIB_SUFFIX}"
)
install(FILES
    app.hpp
    manager.hpp
    config.hpp
    observer.hpp
    window.hpp
    process.hpp
    outputplugin.hpp
    DESTINATION include/qvr)
