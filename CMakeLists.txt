cmake_minimum_required(VERSION 3.6)
project(minimail)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

aux_source_directory("." SOURCE_FILES)

add_executable(minimail ${SOURCE_FILES})
target_link_libraries(minimail "pthread")
target_link_libraries(minimail "sqlite3")