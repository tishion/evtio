cmake_minimum_required(VERSION 3.16)
project(evtio)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(WINDOWS TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(LINUX TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Android")
    set(ANDROID TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOSX TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "iOS")
    set(IOS TRUE)
    if(${CMAKE_VERSION} VERSION_LESS "3.14.0") 
        message(FATAL_ERROR "CMake Version is too low to build for iOS platform, 3.14+ is required.")
    endif()
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}/include
)

add_subdirectory(example)
