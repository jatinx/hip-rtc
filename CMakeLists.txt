cmake_minimum_required(VERSION 3.16)
project(hip_rtc)

option(ENABLE_TESTING OFF)

if(NOT DEFINED ROCM_PATH)
    set(ROCM_PATH "/opt/rocm")
endif()
message(STATUS "ROCM_PATH: ${ROCM_PATH}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

# Version
set(HIPRTC_MAJOR_VERSION 0)
set(HIPRTC_MINOR_VERSION 2)
add_compile_definitions(HIPRTC_MAJOR_VERSION=${HIPRTC_MAJOR_VERSION})
add_compile_definitions(HIPRTC_MINOR_VERSION=${HIPRTC_MINOR_VERSION})

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/headers)

add_subdirectory(source)

if(ENABLE_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()