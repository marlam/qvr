# Copyright (C) 2016
# Computer Graphics Group, University of Siegen
# Written by Martin Lambers <martin.lambers@uni-siegen.de>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice and this
# notice are preserved. This file is offered as-is, without any warranty.

FIND_PATH(OCULUS_INCLUDE_DIR NAMES OVR_CAPI.h)

FIND_LIBRARY(OCULUS_LIBRARY NAMES OVR)

MARK_AS_ADVANCED(OCULUS_INCLUDE_DIR OCULUS_LIBRARY)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OCULUS
    REQUIRED_VARS OCULUS_LIBRARY OCULUS_INCLUDE_DIR
)

IF(OCULUS_FOUND)
    SET(OCULUS_LIBRARIES ${OCULUS_LIBRARY})
    IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        list(APPEND OCULUS_LIBRARIES -ldl)
    ENDIF()
    SET(OCULUS_INCLUDE_DIRS ${OCULUS_INCLUDE_DIR})
ENDIF()
