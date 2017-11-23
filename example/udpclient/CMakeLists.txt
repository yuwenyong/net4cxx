
add_executable(udpclient udpclient.cpp)
add_dependencies(udpclient net4cxx)
target_link_libraries(udpclient net4cxx)