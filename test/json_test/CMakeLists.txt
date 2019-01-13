add_executable(json_test json_test.cpp)
add_dependencies(json_test net4cxx)
target_link_libraries(json_test net4cxx)