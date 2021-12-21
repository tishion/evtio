cmake_minimum_required(VERSION 3.16)
project(evtio)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/bin")

macro(create_example example_name example_soure_folder)
    file(GLOB_RECURSE _source
        ${CMAKE_CURRENT_SOURCE_DIR}/${example_soure_folder}/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/${example_soure_folder}/*.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/${example_soure_folder}/*.cpp
    )

    source_group(
        TREE ${CMAKE_CURRENT_SOURCE_DIR}/${example_soure_folder}
        FILES ${_source}
    )

    add_executable(${example_name} 
        ${_source}
    )

    set_target_properties(${example_name} 
        PROPERTIES FOLDER "example"
    )

    if (QN_BUILD_STATIC)
        target_compile_options(${example_name} 
            PRIVATE
                -DQUICKNET_STATIC_LIB
        )
    endif()

    target_link_libraries(${example_name}
        PUBLIC 
            quicknet
            ${ARGN}
    )

    target_include_directories(${example_name}
        PUBLIC
        ../
        ${CMAKE_CURRENT_SOURCE_DIR}/${example_soure_folder}/
        ${CMAKE_CURRENT_SOURCE_DIR}/common/
    )

    install(TARGETS ${example_name}
        CONFIGURATIONS Release 
            DESTINATION example
    )
    install(TARGETS ${example_name}
        CONFIGURATIONS Debug 
            DESTINATION example/debug
    )
endmacro(create_example)


# generate debug symbols

if (WINDOWS)
    add_compile_options(
        /W4
        -D_CRT_SECURE_NO_WARNINGS
    )
    set(link_libs
        ws2_32
    )
else()
    add_compile_options(
        -g
    )
endif()

file(GLOB_RECURSE SRC_common
    common/*
)

macro(add_example target_name)
    add_executable(${target_name} 
        ${ARGN}
        ${SRC_common}
    )
    target_link_libraries(${target_name}
        ${link_libs}
    )
endmacro(add_example)

add_example(echo_udp_server 
    echo_udp_server.cpp
)

add_example(echo_udp_client 
    echo_udp_client.cpp
)

add_example(echo_tcp_server 
    echo_tcp_server.cpp
)

add_example(echo_tcp_client 
    echo_tcp_client.cpp
)
