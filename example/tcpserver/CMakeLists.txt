
add_executable(tcpserver tcpserver.cpp)
add_dependencies(tcpserver net4cxx)
target_link_libraries(tcpserver net4cxx)