# Copyright (C) 2016
# Computer Graphics Group, University of Siegen
# Written by Martin Lambers <martin.lambers@uni-siegen.de>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice and this
# notice are preserved. This file is offered as-is, without any warranty.

FIND_PATH(OSVRCORE_INCLUDE_DIR NAMES osvr/ClientKit/DisplayC.h)

FIND_LIBRARY(OSVRCORE_LIBRARY NAMES osvrClientKit)

MARK_AS_ADVANCED(OSVRCORE_INCLUDE_DIR OSVRCORE_LIBRARY)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OSVRCORE
    REQUIRED_VARS OSVRCORE_LIBRARY OSVRCORE_INCLUDE_DIR
)

IF(OSVRCORE_FOUND)
    SET(OSVRCORE_LIBRARIES ${OSVRCORE_LIBRARY})
    SET(OSVRCORE_INCLUDE_DIRS ${OSVRCORE_INCLUDE_DIR})
ENDIF()
