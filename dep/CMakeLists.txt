
if(CMAKE_C_COMPILER MATCHES "gcc" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_subdirectory(backtrace)
endif()

add_subdirectory(boost)
add_subdirectory(openssl)
add_subdirectory(zlib)