cmake_minimum_required(VERSION 3.15)
project(lastBit)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")

# Locate source files
file(GLOB SOURCES "src/*.cc")

# Create executable from sources
add_executable(lastBit ${SOURCES})
