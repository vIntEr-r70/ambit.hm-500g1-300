cmake_minimum_required(VERSION 3.20)

project(ethercat)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/ethercat.cpp

    src/slaves/servo-motor.cpp
    src/slaves/servo-motors/lc10e.cpp
    src/slaves/servo-motors/sd700.cpp

    src/functions/axis-motion-node.cpp
)

# if (NOT CMAKE_TOOLCHAIN_FILE)
#     target_include_directories(${PROJECT_NAME} PRIVATE
#         /home/alex/projects/ethercat/include
#     )
#     target_link_directories(${PROJECT_NAME} PRIVATE
#         /home/alex/projects/ethercat/lib/.libs
#     )
# endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    eng::eng
)

if (CMAKE_TOOLCHAIN_FILE AND NOT WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        -lethercat
    )
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)


if(WIN32)
    target_compile_options(${PROJECT_NAME} PRIVATE /std:c++latest)
else()
    target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_26)
endif()

