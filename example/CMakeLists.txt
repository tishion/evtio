cmake_minimum_required(VERSION 3.16)
project(evtio)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/bin")

# generate debug symbols
add_compile_options(
    "-g"
)

add_executable(echo-udp-server 
    echo_udp_server.cpp
)

add_executable(echo-udp-client 
    echo_udp_client.cpp
)

add_executable(echo-tcp-server 
    echo_tcp_server.cpp
)

add_executable(echo-tcp-client 
    echo_tcp_client.cpp
)
