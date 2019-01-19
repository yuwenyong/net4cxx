
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_C_COMPILER MATCHES "gcc" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
        find_package(Boost 1.66 REQUIRED COMPONENTS system filesystem thread program_options iostreams regex log_setup
                     log date_time stacktrace_backtrace)
    else()
        find_package(Boost 1.66 REQUIRED COMPONENTS system filesystem thread program_options iostreams regex log_setup
                     log date_time stacktrace_basic)
    endif()
else()
    find_package(Boost 1.66 REQUIRED COMPONENTS system filesystem thread program_options iostreams regex log_setup
                 log date_time stacktrace_noop)
endif()

add_library(boost INTERFACE)

target_link_libraries(boost INTERFACE ${Boost_LIBRARIES})

target_include_directories(boost INTERFACE ${Boost_INCLUDE_DIRS})