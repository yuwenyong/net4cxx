
add_executable(tcpclient tcpclient.cpp)
add_dependencies(tcpclient net4cxx)
target_link_libraries(tcpclient net4cxx)