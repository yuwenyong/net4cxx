SET(Boost_USE_STATIC_LIBS OFF)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_C_COMPILER MATCHES "gcc" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
        if (CMAKE_SYSTEM_NAME MATCHES "Windows")
            find_package(Boost 1.66 REQUIRED COMPONENTS system filesystem thread program_options iostreams regex log_setup
                         log date_time stacktrace_windbg)
        else()
            find_package(Boost 1.66 REQUIRED COMPONENTS system filesystem thread program_options iostreams regex log_setup
                    log date_time stacktrace_backtrace)
        endif()
    else()
        find_package(Boost 1.66 REQUIRED COMPONENTS system filesystem thread program_options iostreams regex log_setup
                     log date_time stacktrace_basic)
    endif()
else()
    find_package(Boost 1.66 REQUIRED COMPONENTS system filesystem thread program_options iostreams regex log_setup
                 log date_time stacktrace_noop)
endif()

message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")

add_library(boost INTERFACE)

target_link_libraries(boost INTERFACE ${Boost_LIBRARIES})

target_include_directories(boost INTERFACE ${Boost_INCLUDE_DIRS})
