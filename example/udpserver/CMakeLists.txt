
add_executable(udpserver udpserver.cpp)
add_dependencies(udpserver net4cxx)
target_link_libraries(udpserver net4cxx)