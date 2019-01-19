
CollectSourceFiles(${CMAKE_CURRENT_SOURCE_DIR} PRIVATE_SOURCES)

add_definitions(-DNET4CXX_API_EXPORT_COMMON)

add_library(net4cxx SHARED ${PRIVATE_SOURCES})

target_include_directories(net4cxx PUBLIC "${CMAKE_SOURCE_DIR}/src/")

target_link_libraries(net4cxx PUBLIC pthread openssl boost zlib dl)

if(CMAKE_C_COMPILER MATCHES "gcc" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(net4cxx PUBLIC backtrace)
endif()