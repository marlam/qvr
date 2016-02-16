# Copyright (C) 2016
# Computer Graphics Group, University of Siegen
# Written by Martin Lambers <martin.lambers@uni-siegen.de>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice and this
# notice are preserved. This file is offered as-is, without any warranty.

FIND_PATH(QVR_INCLUDE_DIR NAMES qvr/app.hpp)

FIND_LIBRARY(QVR_LIBRARY NAMES qvr)

MARK_AS_ADVANCED(QVR_INCLUDE_DIR QVR_LIBRARY)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(QVR
    REQUIRED_VARS QVR_LIBRARY QVR_INCLUDE_DIR
)

IF(QVR_FOUND)
    SET(QVR_LIBRARIES ${QVR_LIBRARY})
    SET(QVR_INCLUDE_DIRS ${QVR_INCLUDE_DIR})
ENDIF()
