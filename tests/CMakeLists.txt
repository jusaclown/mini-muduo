add_executable(echo_server echo_server.cc)
target_link_libraries(echo_server PRIVATE mini_muduo)
muduo_enable_warnings(echo_server)
muduo_enable_sanitizer(echo_server)

add_executable(test_timer test_timer.cc)
target_link_libraries(test_timer PRIVATE mini_muduo)

add_executable(http_server http_server.cc)
target_link_libraries(http_server PRIVATE mini_muduo)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    muduo_enable_warnings(http_server)
    muduo_enable_sanitizer(http_server)
endif()
