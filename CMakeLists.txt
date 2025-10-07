cmake_minimum_required(VERSION 3.20)

project(hm-500g1-300)

#-----------------------------------------------------------------------

include(FetchContent)

FetchContent_Declare(
    aem
    GIT_REPOSITORY git@gitlab.com:alexander.kvashnin/libaem.git
    GIT_TAG        master
)

FetchContent_Declare(
    we
    GIT_REPOSITORY git@gitlab.com:alexander.kvashnin/libwe.git
    GIT_TAG        master
)

FetchContent_MakeAvailable(aem we)

#-----------------------------------------------------------------------

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (CMAKE_TOOLCHAIN_FILE)
    add_definitions(-DBUILDROOT)
    message(STATUS "IN TOOLCHAIN MODE")
else()
    message(STATUS "IN NATIVE MODE")
endif()

add_subdirectory(src/gui)
add_subdirectory(src/worker)
