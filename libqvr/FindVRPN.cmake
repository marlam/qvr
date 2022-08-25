# Copyright (C) 2016, 2022
# Computer Graphics Group, University of Siegen
# Written by Martin Lambers <martin.lambers@uni-siegen.de>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice and this
# notice are preserved. This file is offered as-is, without any warranty.

FIND_PATH(VRPN_INCLUDE_DIR NAMES vrpn_Tracker.h)

FIND_LIBRARY(VRPN_LIBRARY NAMES vrpn)
FIND_LIBRARY(VRPN_QUAT_LIBRARY NAMES quat)

MARK_AS_ADVANCED(VRPN_INCLUDE_DIR VRPN_LIBRARY VRPN_QUAT_LIBRARY)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(VRPN
    REQUIRED_VARS VRPN_LIBRARY VRPN_QUAT_LIBRARY VRPN_INCLUDE_DIR
)

IF(VRPN_FOUND)
    SET(VRPN_LIBRARIES ${VRPN_LIBRARY} ${VRPN_QUAT_LIBRARY})
    SET(VRPN_INCLUDE_DIRS ${VRPN_INCLUDE_DIR})
ENDIF()
