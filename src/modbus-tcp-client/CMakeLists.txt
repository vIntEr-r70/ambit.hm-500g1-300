cmake_minimum_required(VERSION 3.28)

project(modbus-tcp-client)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/units-interaction.cpp
    # src/unit-conf-loader.cpp
    # src/unit-node.cpp

    src/modbus-unit.cpp
    src/units/PR205-A1.cpp
    src/units/PR205-A2.cpp
    src/units/PR205-BN.cpp
    src/units/PR205-A14.cpp
    src/units/PLC110.cpp
)

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

