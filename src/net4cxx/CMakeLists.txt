
CollectSourceFiles(${CMAKE_CURRENT_SOURCE_DIR} PRIVATE_SOURCES)
add_library(net4cxx SHARED ${PRIVATE_SOURCES})
target_include_directories(net4cxx PUBLIC "${CMAKE_SOURCE_DIR}/src/")
if(CMAKE_C_COMPILER MATCHES "gcc" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(net4cxx PUBLIC pthread openssl boost zlib dl backtrace)
else()
    target_link_libraries(net4cxx PUBLIC pthread openssl boost zlib dl)
endif()