cmake_minimum_required(VERSION 3.20)

project(worker VERSION 0.1.0)

add_executable(${PROJECT_NAME}
	main.cpp

    worker.cpp
    hw.cpp
    cfg.cpp

    actors/a_centering.cpp

    modes/auto/auto-mode.cpp
    modes/auto/vm.cpp

    modes/bki-reset-ctrl.cpp
    modes/bki-allow-ctrl.cpp
    modes/manual/manual-mode.cpp
    modes/manual/base-ctrl.cpp
    modes/manual/rcu/rcu-ctrl.cpp
    modes/manual/panel/panel-mode.cpp
    modes/manual/panel/panel-ctrl.cpp
    modes/manual/panel/move-to-ctrl.cpp
    modes/manual/panel/centering-ctrl.cpp
    modes/manual/panel/calibrate-ctrl.cpp

    modes/unit-on-off-ctl.cpp
    modes/sprayer-on-off-ctl.cpp
    modes/dmg-led-ctl.cpp
    modes/fc-led-ctl.cpp

    ctls/drain-ctl.cpp
    ctls/spump-ctl.cpp
    ctls/barrel-ctl.cpp
    ctls/pr-barrel-ctl.cpp
    ctls/drainage-ctl.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    aem::aem
    we::we
    -lfmt
)

target_compile_features(${PROJECT_NAME} PRIVATE
    cxx_std_26
)

