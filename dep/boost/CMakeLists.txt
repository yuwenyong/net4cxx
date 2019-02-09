SET(Boost_USE_STATIC_LIBS OFF)

find_package(Boost 1.66 REQUIRED COMPONENTS system filesystem thread program_options iostreams regex log_setup log date_time)

message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")

add_library(boost INTERFACE)

target_link_libraries(boost INTERFACE ${Boost_LIBRARIES})

target_include_directories(boost INTERFACE ${Boost_INCLUDE_DIRS})
