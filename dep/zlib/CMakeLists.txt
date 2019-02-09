find_package(ZLIB REQUIRED)

message(STATUS "ZLIB_LIBRARIES: ${ZLIB_LIBRARIES}")
message(STATUS "ZLIB_INCLUDE_DIRS: ${ZLIB_INCLUDE_DIRS}")

add_library(zlib INTERFACE)

target_link_libraries(zlib
        INTERFACE
        ${ZLIB_LIBRARIES})

target_include_directories(zlib
        INTERFACE
        ${ZLIB_INCLUDE_DIRS})


