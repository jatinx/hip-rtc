cmake_minimum_required(VERSION 3.16)
project(hip_rtc)

option(ENABLE_TESTING OFF)

if(NOT DEFINED ROCM_PATH)
    set(ROCM_PATH "/opt/rocm")
endif()
message(STATUS "ROCM_PATH: ${ROCM_PATH}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")

include_directories(${PROJECT_SOURCE_DIR}/include)
add_subdirectory(source)

if(ENABLE_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()