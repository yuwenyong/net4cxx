
find_package(ZLIB REQUIRED)
if (CMAKE_SYSTEM_NAME MATCHES "Windows")
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

