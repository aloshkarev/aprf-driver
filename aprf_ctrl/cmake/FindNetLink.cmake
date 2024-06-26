IF (NL_LIBRARIES AND NL_INCLUDE_DIRS)
    # in cache already
    SET(NL_FOUND TRUE)
ELSE (NL_LIBRARIES AND NL_INCLUDE_DIRS)
    SET(SEARCHPATHS
            /opt/local
            /sw
            /usr
            /usr/local
            )
    FIND_PATH(NL_INCLUDE_DIR
            PATH_SUFFIXES
            include/libnl3
            NAMES
            netlink/version.h
            PATHS
            $(SEARCHPATHS)
            )
    # NL version >= 2
    IF (NL_INCLUDE_DIR)
        FIND_LIBRARY(NL_LIBRARY
                NAMES
                nl-3 nl
                PATH_SUFFIXES
                lib64 lib
                PATHS
                $(SEARCHPATHS)
                )
        FIND_LIBRARY(NLGENL_LIBRARY
                NAMES
                nl-genl-3 nl-genl
                PATH_SUFFIXES
                lib64 lib
                PATHS
                $(SEARCHPATHS)
                )
        #
        # If we don't have all of those libraries, we can't use libnl.
        #
        IF (NOT NLGENL_LIBRARY)
            SET(NL_LIBRARY NOTFOUND)
        ENDIF (NOT NLGENL_LIBRARY)
        IF (NL_LIBRARY)
            STRING(REGEX REPLACE ".*nl-([^.,;]*).*" "\\1" NLSUFFIX ${NL_LIBRARY})
            IF (NLSUFFIX)
                SET(HAVE_LIBNL3 1)
            ELSE (NLSUFFIX)
                SET(HAVE_LIBNL2 1)
            ENDIF (NLSUFFIX)
            SET(HAVE_LIBNL 1)
        ENDIF (NL_LIBRARY)
    ELSE (NL_INCLUDE_DIR)
        # NL version 1 ?
        FIND_PATH(NL_INCLUDE_DIR
                NAMES
                netlink/netlink.h
                PATHS
                $(SEARCHPATHS)
                )
        FIND_LIBRARY(NL_LIBRARY
                NAMES
                nl
                PATH_SUFFIXES
                lib64 lib
                PATHS
                $(SEARCHPATHS)
                )
        if (NL_INCLUDE_DIR)
            SET(HAVE_LIBNL1 1)
        ENDIF (NL_INCLUDE_DIR)
    ENDIF (NL_INCLUDE_DIR)
ENDIF (NL_LIBRARIES AND NL_INCLUDE_DIRS)
# MESSAGE(STATUS "LIB Found: ${NL_LIBRARY}, Suffix: ${NLSUFFIX}\n  1:${HAVE_LIBNL1}, 2:${HAVE_LIBNL2}, 3:${HAVE_LIBNL3}.")

# handle the QUIETLY and REQUIRED arguments and set NL_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(NL DEFAULT_MSG NL_LIBRARY NL_INCLUDE_DIR)

IF (NL_FOUND)
    SET(NL_LIBRARIES ${NLGENL_LIBRARY} ${NL_LIBRARY})
    SET(NL_INCLUDE_DIRS ${NL_INCLUDE_DIR})
ELSE ()
    SET(NL_LIBRARIES)
    SET(NL_INCLUDE_DIRS)
ENDIF ()

MARK_AS_ADVANCED(NL_LIBRARIES NL_INCLUDE_DIRS)