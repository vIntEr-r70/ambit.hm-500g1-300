cmake_minimum_required(VERSION 3.20)

project(hm-500g1-300)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (CMAKE_TOOLCHAIN_FILE)
    add_definitions(-DBUILDROOT)
    message(STATUS "IN TOOLCHAIN MODE")
else()
    message(STATUS "IN NATIVE MODE")
endif()

include_directories(PRIVATE src/common)

add_subdirectory(src/libs/libaem)
# add_subdirectory(src/libs/libwe)
add_subdirectory(src/libs/libeng)
add_subdirectory(src/libs/ambit-gui-core)
add_subdirectory(src/gui)
add_subdirectory(src/cnc)
add_subdirectory(src/core)
add_subdirectory(src/ethercat)
add_subdirectory(src/modbus-tcp-client)
add_subdirectory(src/modbus-rtu-master)
