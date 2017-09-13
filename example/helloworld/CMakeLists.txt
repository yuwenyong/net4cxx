
add_executable(helloworld helloworld.cpp)
add_dependencies(helloworld net4cxx)
target_link_libraries(helloworld net4cxx)