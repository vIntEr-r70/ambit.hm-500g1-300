cmake_minimum_required(VERSION 3.20)

project(gui)

#-----------------------------------------------------------------------

include(FetchContent)

FetchContent_Declare(
    gui-core
    GIT_REPOSITORY git@gitlab.com:alexander.kvashnin/ambit-gui-core.git
    GIT_TAG        master
)

FetchContent_MakeAvailable(gui-core)

#-----------------------------------------------------------------------

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
    gui/BkiLockMessage.cpp
    gui/EmgStopMessage.cpp

    gui/ManualCtrlWindow/ManualCtrlWindow.cpp
    gui/ManualCtrlWindow/AxisCtrlWidget.cpp
    gui/ManualCtrlWindow/SprayerCtrlWidget.cpp
    gui/ManualCtrlWindow/ManualEngineSetDlg.cpp
    gui/ManualCtrlWindow/CenteringCfgWidget.cpp
    gui/ManualCtrlWindow/CenteringCtrlDlg.cpp

    gui/AutoCtrlWindow/AutoCtrlWindow.cpp
    gui/AutoCtrlWindow/AutoCtrlWidget.cpp
    gui/AutoCtrlWindow/AutoEditWidget.cpp
    gui/AutoCtrlWindow/ProgramModel.cpp
    gui/AutoCtrlWindow/ProgramModelHeader.cpp
    gui/AutoCtrlWindow/ProgramStateCtrlWidget.cpp
    gui/AutoCtrlWindow/ProgramOpFactory.cpp
    gui/AutoCtrlWindow/ProgramListDlg.cpp
    gui/AutoCtrlWindow/AutoParamKeyboard.cpp
    gui/AutoCtrlWindow/EditorMessageBox.cpp

    gui/SysCfgWindow/SysCfgWindow.cpp
    gui/SysCfgWindow/SysSetsWidget.cpp
    gui/SysCfgWindow/EngineSignalsWidget.cpp
    gui/SysCfgWindow/BkiCfgWidget.cpp

    gui/MnemonicWindow/MnemonicWindow.cpp
    gui/MnemonicWindow/MnemonicWidget.cpp
    gui/MnemonicWindow/LegendWidget.cpp

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

    ../common/Program.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${Qt5Widgets_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${Qt5Widgets_LIBRARIES}
    ${Qt5PrintSupport_LIBRARIES}
    aem::aem
    we::we
    gui-core::gui-core
    -lfmt
)

target_compile_features(${PROJECT_NAME} PRIVATE
    cxx_std_26
)

