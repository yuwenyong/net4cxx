
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

message(STATUS "OPENSSL_INCLUDE_DIR: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OPENSSL_LIBRARIES: ${OPENSSL_LIBRARIES}")

add_library(openssl INTERFACE)

target_link_libraries(openssl
        INTERFACE
        ${OPENSSL_LIBRARIES})

target_include_directories(openssl
        INTERFACE
        ${OPENSSL_INCLUDE_DIR})


