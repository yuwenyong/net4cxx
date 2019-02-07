
if(APPLE)
    set(_OPENSSL_ROOT_PATHS "/usr/local/ssl")
    find_path(OPENSSL_ROOT_DIR
            NAMES
            include/openssl/ssl.h
            PATHS
            ${_OPENSSL_ROOT_PATHS}
            )
endif()

set(OPENSSL_USE_STATIC_LIBS FALSE)

find_package(OpenSSL REQUIRED)

#if(MINGW)
#    foreach(_OPENSSL_LIB_PATH ${OPENSSL_LIBRARIES})
#        string(REPLACE ".dll.a" ".a" _FIXED_OPENSSL_LIB_PATH ${_OPENSSL_LIB_PATH})
#        list(APPEND _FIXED_OPENSSL_LIBRARIES ${_FIXED_OPENSSL_LIB_PATH})
#    endforeach()
#    if(_FIXED_OPENSSL_LIBRARIES)
#        set(OPENSSL_LIBRARIES ${_FIXED_OPENSSL_LIBRARIES})
#    endif()
#endif()

message(STATUS "OPENSSL_INCLUDE_DIR: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OPENSSL_LIBRARIES: ${OPENSSL_LIBRARIES}")

add_library(openssl INTERFACE)

target_link_libraries(openssl
        INTERFACE
        ${OPENSSL_LIBRARIES})

target_include_directories(openssl
        INTERFACE
        ${OPENSSL_INCLUDE_DIR})


