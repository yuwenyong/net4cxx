
#if(CMAKE_C_COMPILER MATCHES "gcc" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
#    if (NOT CMAKE_SYSTEM_NAME MATCHES "Windows")
#        add_subdirectory(backtrace)
#    endif()
#endif()

add_subdirectory(zlib)
add_subdirectory(openssl)
add_subdirectory(boost)