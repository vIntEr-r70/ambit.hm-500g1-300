cmake_minimum_required(VERSION 3.20)

project(core)

add_executable(${PROJECT_NAME}
    main.cpp

    src/intervals.cpp

    src/auto-mode/auto-mode.cpp
    src/auto-mode/vm.cpp

    src/stuff-ctl.cpp
    src/speed-node.cpp
    src/panel-button-ctl.cpp
    src/panel-switch-2-ctl.cpp

    src/current-conductors.cpp

    src/working-mode-selector.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    eng::eng
    stdc++exp
)

target_compile_features(${PROJECT_NAME} PRIVATE
    cxx_std_26
)


