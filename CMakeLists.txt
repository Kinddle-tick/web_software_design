cmake_minimum_required(VERSION 3.19)
project(web_software_design_course_design)

set(CMAKE_CXX_STANDARD 14)

#add_executable(S_client S_client.cpp)
#add_executable(S_server S_server.cpp)
add_executable(New_client src/client_main.cpp src/client_source.cpp)
add_executable(New_server src/server_source.cpp src/server_main.cpp)