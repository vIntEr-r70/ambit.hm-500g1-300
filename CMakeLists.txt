cmake_minimum_required(VERSION 3.20)

project(hm-500g1-300)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (CMAKE_TOOLCHAIN_FILE)
    add_definitions(-DBUILDROOT)
    message(STATUS "IN TOOLCHAIN MODE")
else()
    message(STATUS "IN NATIVE MODE")
endif()

add_subdirectory(src/libs/libaem)
add_subdirectory(src/libs/libwe)
add_subdirectory(src/libs/ambit-gui-core)
add_subdirectory(src/gui)
add_subdirectory(src/worker)
