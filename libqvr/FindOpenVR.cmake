# Copyright (C) 2016
# Computer Graphics Group, University of Siegen
# Written by Martin Lambers <martin.lambers@uni-siegen.de>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice and this
# notice are preserved. This file is offered as-is, without any warranty.

FIND_PATH(OPENVR_INCLUDE_DIR NAMES openvr.h)

FIND_LIBRARY(OPENVR_LIBRARY NAMES openvr_api)

MARK_AS_ADVANCED(OPENVR_INCLUDE_DIR OPENVR_LIBRARY)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OPENVR
    REQUIRED_VARS OPENVR_LIBRARY OPENVR_INCLUDE_DIR
)

IF(OPENVR_FOUND)
    SET(OPENVR_LIBRARIES ${OPENVR_LIBRARY})
    SET(OPENVR_INCLUDE_DIRS ${OPENVR_INCLUDE_DIR})
ENDIF()
