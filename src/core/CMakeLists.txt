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

    src/current-conductors.cpp

    src/working-mode-selector.cpp

    src/drainage-ctl.cpp
    src/barrel-ctl.cpp
    src/barrel-lvl-ctl.cpp

    src/sprayer-ctl.cpp

    src/error-mask.cpp

    src/bki-ctl.cpp
    src/emg-ctl.cpp

    src/diverter-valve-ctl.cpp

    src/multi-mode-panel-btns.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    eng::eng
    stdc++exp
)

target_compile_features(${PROJECT_NAME} PRIVATE
    cxx_std_26
)


