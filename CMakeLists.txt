cmake_minimum_required(VERSION 3.10)
project(maze LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c++11")

add_executable(maze
  src/maze.cpp
  src/main.cpp
  src/utils.cpp
)
