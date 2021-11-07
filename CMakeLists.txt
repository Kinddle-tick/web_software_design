cmake_minimum_required(VERSION 3.19)
project(test2)

set(CMAKE_CXX_STANDARD 14)

add_executable(S_client S_client.cpp)
add_executable(S_server S_server.cpp)
