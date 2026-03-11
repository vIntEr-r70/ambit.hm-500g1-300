cmake_minimum_required(VERSION 3.20)

project(sys-emu)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(PkgConfig REQUIRED)

add_definitions(${Qt5Widgets_DEFINITIONS})

set(CMAKE_CXX_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(${PROJECT_NAME}
    main.cpp
    main-frame.cpp

    listeners/syslink.cpp
    listeners/base-device.cpp
    listeners/modbus-tcp-server.cpp

    listeners/devices/PLC110.cpp
    listeners/devices/FC.cpp
    listeners/devices/PR200.cpp
    listeners/devices/PR205-A1.cpp
    listeners/devices/PR205-A2.cpp
    listeners/devices/PR205-A14.cpp
    listeners/devices/PR205-B2.cpp
    listeners/devices/PR205-B6.cpp
    listeners/devices/PR205-B10.cpp

    subsys/panel-page.cpp
    subsys/sens-fc-page.cpp
    subsys/sens-dt-page.cpp
    subsys/sens-dp-page.cpp
    subsys/frequency-converter-page.cpp

    widgets/pair-buttons.cpp
    widgets/moving-buttons.cpp
    widgets/select-buttons.cpp
    widgets/titled-led.cpp
    widgets/sens-value-set.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${Qt5Widgets_LIBRARIES}
    gui-core::gui-core
)

if(MSVC)

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:gui>
            $<TARGET_FILE_DIR:gui>
        COMMAND_EXPAND_LISTS
    )

    target_compile_options(${PROJECT_NAME} PRIVATE /std:c++latest /EHsc)

else()

    target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_26)

    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wno-deprecated-declarations
        -Wno-template-id-cdtor
    )

endif()


