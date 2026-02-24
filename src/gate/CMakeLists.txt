cmake_minimum_required(VERSION 3.20)

project(gate VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Release)

add_executable(gate
    src/main.cpp
)

target_link_libraries(gate PRIVATE
    aem::aem
    -lblkid
    -lfmt
)

