cmake_minimum_required(VERSION 3.28)

project(modbus-rtu-master)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/modbus-rtu-master.cpp
    src/slaves/frequency-converter.cpp
    src/slaves/PR200.cpp
    src/modbus-unit.cpp
)

message(${PROJECT_SOURCE_DIR})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    eng::eng
    stdc++exp
)

target_compile_features(${PROJECT_NAME} PRIVATE
    cxx_std_26
)

