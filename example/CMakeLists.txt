cmake_minimum_required(VERSION 3.16)
project(evtio)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/bin")

# generate debug symbols

if (WINDOWS)
    add_compile_options(
        /W4
        -D_CRT_SECURE_NO_WARNINGS
    )
else()
    add_compile_options(
        -g
    )
endif()

file(GLOB_RECURSE SRC_common
    common/*
)

add_executable(echo_udp_server 
    echo_udp_server.cpp
    ${SRC_common}
)

add_executable(echo_udp_client 
    echo_udp_client.cpp
    ${SRC_common}
)

add_executable(echo_tcp_server 
    echo_tcp_server.cpp
    ${SRC_common}
)

add_executable(echo_tcp_client 
    echo_tcp_client.cpp
    ${SRC_common}
)
