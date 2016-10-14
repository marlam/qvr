# Copyright (C) 2016
# Computer Graphics Group, University of Siegen
# Written by Martin Lambers <martin.lambers@uni-siegen.de>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice and this
# notice are preserved. This file is offered as-is, without any warranty.

FIND_PATH(OSVRRENDERMANAGER_INCLUDE_DIR NAMES osvr/RenderKit/RenderManagerOpenGLC.h)

FIND_LIBRARY(OSVRRENDERMANAGER_LIBRARY NAMES osvrRenderManager)

MARK_AS_ADVANCED(OSVRRENDERMANAGER_INCLUDE_DIR OSVRRENDERMANAGER_LIBRARY)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OSVRRENDERMANAGER
    REQUIRED_VARS OSVRRENDERMANAGER_LIBRARY OSVRRENDERMANAGER_INCLUDE_DIR
)

IF(OSVRRENDERMANAGER_FOUND)
    SET(OSVRRENDERMANAGER_LIBRARIES ${OSVRRENDERMANAGER_LIBRARY})
    SET(OSVRRENDERMANAGER_INCLUDE_DIRS ${OSVRRENDERMANAGER_INCLUDE_DIR})
ENDIF()
