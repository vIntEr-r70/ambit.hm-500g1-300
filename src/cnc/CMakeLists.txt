cmake_minimum_required(VERSION 3.20)

project(cnc)

add_executable(${PROJECT_NAME}
    main.cpp
    src/axis-panel-ctl.cpp
    src/rcu-ctl.cpp
    src/tooth-center-search.cpp
    src/multi-axis-ctl.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    eng::eng
    stdc++exp
)

if(WIN32)
    target_compile_options(${PROJECT_NAME} PRIVATE /std:c++latest)
else()
    target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_26)
endif()


