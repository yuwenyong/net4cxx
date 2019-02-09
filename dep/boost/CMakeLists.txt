SET(Boost_USE_STATIC_LIBS OFF)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        find_package(Boost 1.66 REQUIRED COMPONENTS system filesystem thread program_options iostreams regex log_setup
                log date_time stacktrace_basic)
    else()
        find_package(Boost 1.66 REQUIRED COMPONENTS system filesystem thread program_options iostreams regex log_setup
                log date_time stacktrace_noop)
    endif()
else()
    find_package(Boost 1.66 REQUIRED COMPONENTS system filesystem thread program_options iostreams regex log_setup log
            date_time)
endif()


message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")

add_library(boost INTERFACE)

target_link_libraries(boost INTERFACE ${Boost_LIBRARIES})

target_include_directories(boost INTERFACE ${Boost_INCLUDE_DIRS})

