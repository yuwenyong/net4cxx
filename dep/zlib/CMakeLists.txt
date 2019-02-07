if (MINGW)
    set(_zlib_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
    if(MINGW)
        set(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
    else()
        set(CMAKE_FIND_LIBRARY_SUFFIXES .a )
    endif()
endif()

find_package(ZLIB REQUIRED)

if(MINGW)
    foreach(_ZLIB_LIB_PATH ${ZLIB_LIBRARIES})
        string(REPLACE ".dll.a" ".a" _FIXED_ZLIB_LIB_PATH ${_ZLIB_LIB_PATH})
        list(APPEND _FIXED_ZLIB_LIBRARIES ${_FIXED_ZLIB_LIB_PATH})
    endforeach()
    if(_FIXED_ZLIB_LIBRARIES)
        set(ZLIB_LIBRARIES ${_FIXED_ZLIB_LIBRARIES})
    endif()
endif()

message(STATUS "ZLIB_LIBRARIES: ${ZLIB_LIBRARIES}")
message(STATUS "ZLIB_INCLUDE_DIRS: ${ZLIB_INCLUDE_DIRS}")

if(MINGW)
    add_library(zlib STATIC IMPORTED GLOBAL)
else()
    add_library(zlib SHARED IMPORTED GLOBAL)
endif()

set_target_properties(zlib
        PROPERTIES
        IMPORTED_LOCATION
        "${ZLIB_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES
        "${ZLIB_INCLUDE_DIRS}")

if(MINGW)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${_zlib_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})
endif()

