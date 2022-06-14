cmake_minimum_required(VERSION 3.10...3.23)

# ---------------------------------------------------------------------------------------
# Start project
# ---------------------------------------------------------------------------------------
include(cmake/utils.cmake)

project(muduo LANGUAGES CXX)

# ---------------------------------------------------------------------------------------
# Set default build to Debug
# ---------------------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose Release or Debug" FORCE)
endif()
message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})

# ---------------------------------------------------------------------------------------
# Compiler config
# ---------------------------------------------------------------------------------------
if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# ---------------------------------------------------------------------------------------
# Static library
# ---------------------------------------------------------------------------------------
add_library(mini_muduo STATIC     
    src/TcpServer.cc
    src/Channel.cc
    src/Acceptor.cc
    src/TcpConnection.cc
    src/EpollPoller.cc
    src/EventLoop.cc
    src/Buffer.cc
    src/Timer.cc
    src/TimerQueue.cc
)
muduo_enable_warnings(mini_muduo)
target_include_directories(mini_muduo PUBLIC ${PROJECT_SOURCE_DIR})

# ---------------------------------------------------------------------------------------
# Build binaries
# ---------------------------------------------------------------------------------------
message(STATUS "Generating tests")
add_subdirectory(tests)

