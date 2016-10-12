# Copyright (C) 2016
# Computer Graphics Group, University of Siegen
# Written by Martin Lambers <martin.lambers@uni-siegen.de>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice and this
# notice are preserved. This file is offered as-is, without any warranty.

FIND_PATH(OSVR_INCLUDE_DIR NAMES osvr/ClientKit/DisplayC.h)

FIND_LIBRARY(OSVR_LIBRARY NAMES osvrClientKit)

MARK_AS_ADVANCED(OSVR_INCLUDE_DIR OSVR_LIBRARY)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OSVR
    REQUIRED_VARS OSVR_LIBRARY OSVR_INCLUDE_DIR
)

IF(OSVR_FOUND)
    SET(OSVR_LIBRARIES ${OSVR_LIBRARY})
    SET(OSVR_INCLUDE_DIRS ${OSVR_INCLUDE_DIR})
ENDIF()
