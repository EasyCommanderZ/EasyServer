cmake_minimum_required(VERSION 3.12)

project(EasyServer VERSION 0.1 DESCRIPTION "Easy Web Server" LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(ThreadPool)

file(GLOB srcs main.cpp)

add_executable(main ${srcs})

target_link_libraries(main PUBLIC ThreadPool)