cmake_minimum_required(VERSION 3.25)

project(NTechNVisionNative VERSION 0.1.0 LANGUAGES CXX)

option(NVISION_BUILD_TESTS "Build native NVision tests" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NVISION_BUILD_TESTS)
    enable_testing()
endif()

add_subdirectory(native)
