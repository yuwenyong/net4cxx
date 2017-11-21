
add_executable(tcpserver tcpserver.cpp)
add_dependencies(tcpserver net4cxx)
target_link_libraries(tcpserver net4cxx)
add_custom_command(TARGET tcpserver
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/test.crt ${CMAKE_CURRENT_BINARY_DIR}/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/test.key ${CMAKE_CURRENT_BINARY_DIR}/
        )