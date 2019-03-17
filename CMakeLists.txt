cmake_minimum_required(VERSION 3.9)
project(smc32 CXX)

file(GLOB_RECURSE SMC32_SOURCES "src/*.cpp")
file(GLOB_RECURSE SMC32_HEADERS "src/*.h", "src/*.hpp")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")

add_executable(smc32 ${SMC32_SOURCES})
