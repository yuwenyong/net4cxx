
CollectSourceFiles(${CMAKE_CURRENT_SOURCE_DIR} PRIVATE_SOURCES)

add_definitions(-DNET4CXX_API_EXPORT_COMMON)

if (BUILD_NET4CXX_STATIC_LIBS)
    add_library(net4cxx STATIC ${PRIVATE_SOURCES})
else()
    add_library(net4cxx SHARED ${PRIVATE_SOURCES})
endif()

target_include_directories(net4cxx PUBLIC "${CMAKE_SOURCE_DIR}/src/")

target_link_libraries(net4cxx PUBLIC boost openssl zlib)

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    target_link_libraries(net4cxx PUBLIC ws2_32 wsock32)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_link_libraries(net4cxx PUBLIC ole32 dbgeng)
    endif()
else()
    target_link_libraries(net4cxx PUBLIC pthread dl)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        if(CMAKE_C_COMPILER MATCHES "gcc" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
            target_link_libraries(net4cxx PUBLIC backtrace)
        endif()
    endif()
endif()
