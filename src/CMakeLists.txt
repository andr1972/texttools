cmake_minimum_required(VERSION 3.10)
project(eolconv)

set(CMAKE_CXX_STANDARD 14)

add_executable(eolconv eolconv.cpp common.cpp common.h)
target_link_libraries(eolconv
        boost_system
        boost_filesystem
        boost_regex)

add_executable(utf8bom utf8bom.cpp common.cpp common.h)
target_link_libraries(utf8bom
        boost_system
        boost_filesystem
        boost_regex)