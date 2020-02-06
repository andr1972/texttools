cmake_minimum_required(VERSION 3.15)
project(eolconv)

set(CMAKE_CXX_STANDARD 14)

add_executable(eolconv main.cpp)
target_link_libraries(eolconv
        boost_system
        boost_filesystem
        boost_regex)