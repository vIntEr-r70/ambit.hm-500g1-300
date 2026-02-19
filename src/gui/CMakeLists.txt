cmake_minimum_required(VERSION 3.20)

project(gui)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt5 COMPONENTS PrintSupport REQUIRED)
find_package(PkgConfig REQUIRED)

add_definitions(${Qt5Widgets_DEFINITIONS})

set(CMAKE_CXX_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

add_executable(${PROJECT_NAME}
    main.cpp

    gui/MainFrame.cpp
    gui/LockMessageBox.cpp

    gui/ManualCtrlWindow/axis-ctl-widget/AxisCtlWidget.cpp
    gui/ManualCtrlWindow/axis-ctl-widget/AxisCtlItemWidget.cpp
    gui/ManualCtrlWindow/axis-ctl-widget/manual-engine-set-dlg/manual-engine-set-dlg.cpp
    gui/ManualCtrlWindow/axis-ctl-widget/manual-engine-set-dlg/axis-ctl-widget.cpp
    gui/ManualCtrlWindow/ManualCtrlWindow.cpp
    gui/ManualCtrlWindow/SprayerCtrlWidget.cpp
    gui/ManualCtrlWindow/CenteringCfgWidget.cpp
    gui/ManualCtrlWindow/CenteringCtrlDlg.cpp

    gui/auto-ctl-window/auto-ctl-window.cpp
    gui/auto-ctl-window/main-page/main-page.cpp
    gui/auto-ctl-window/main-page/main-page-header-widget.cpp
    gui/auto-ctl-window/main-page/programs-list-widget.cpp
    gui/auto-ctl-window/main-page/program-list-model.cpp
    gui/auto-ctl-window/auto-ctl-page/auto-ctl-page.cpp
    gui/auto-ctl-window/auto-ctl-page/problem-list-widget.cpp
    gui/auto-ctl-window/auto-ctl-page/program-model-mode.cpp
    gui/auto-ctl-window/editor-page/editor-page.cpp
    gui/auto-ctl-window/editor-page/editor-page-header-widget.cpp
    gui/auto-ctl-window/editor-page/AutoParamKeyboard.cpp
    gui/auto-ctl-window/editor-page/EditorMessageBox.cpp
    gui/auto-ctl-window/editor-page/program-model-editor.cpp
    gui/auto-ctl-window/common/program-model.cpp
    gui/auto-ctl-window/common/program-widget.cpp
    gui/auto-ctl-window/common/ProgramModelHeader.cpp

    gui/SysCfgWindow/SysCfgWindow.cpp
    gui/SysCfgWindow/SysSetsWidget.cpp
    gui/SysCfgWindow/BkiCfgWidget.cpp

    gui/ArcsWindow/BaseArcsLoader.cpp
    gui/ArcsWindow/ArcsWindow.cpp
    gui/ArcsWindow/ArcDataViewWidget.cpp
    gui/ArcsWindow/ArcsListWidget.cpp
    gui/ArcsWindow/ArcsListModel.cpp
    gui/ArcsWindow/ArcData.cpp
    gui/ArcsWindow/ArcInfo.cpp
    gui/ArcsWindow/ArcDataView.cpp
    gui/ArcsWindow/DiagramView.cpp

    gui/LiquidSystem/LiquidSystemWindow.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${Qt5Widgets_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${Qt5Widgets_LIBRARIES}
    ${Qt5PrintSupport_LIBRARIES}
    aem::aem
    gui-core::gui-core
    -lfmt
    stdc++exp
)

target_compile_features(${PROJECT_NAME} PRIVATE
    cxx_std_26
)

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wno-deprecated-declarations
    -Wno-template-id-cdtor
)

