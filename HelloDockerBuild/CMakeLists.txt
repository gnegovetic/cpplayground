cmake_minimum_required(VERSION 3.0)

set(CMAKE_C_COMPILER clang-8)
set(CMAKE_CXX_COMPILER clang++-8)

project(HelloWorld)

#set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_BUILD_TYPE Debug)

add_executable(helloWorld HelloWorld.cpp)
