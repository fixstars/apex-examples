# Copyright (c) 2026 Fixstars Corporation
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.10)
project(PoseAlignment VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    add_compile_options(-mavx2 -mfma)
endif()

add_subdirectory(src)
add_subdirectory(test)

message(STATUS "CXX compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "System processor: ${CMAKE_SYSTEM_PROCESSOR}")