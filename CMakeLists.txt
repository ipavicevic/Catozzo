cmake_minimum_required(VERSION 3.21)
project(Catozzo VERSION 0.1.11 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2)

qt_add_executable(Catozzo
    src/main.cpp
    src/ProjectModel.cpp
    src/ProjectModel.h
    src/ClipScanner.cpp
    src/ClipScanner.h
    src/FfmpegRunner.cpp
    src/FfmpegRunner.h
)

qt_add_qml_module(Catozzo
    URI Catozzo
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/components/Toolbar.qml
        qml/components/ClipList.qml
        qml/components/ClipRow.qml
    RESOURCE_PREFIX "/qt/qml"
)

target_compile_definitions(Catozzo PRIVATE APP_VERSION="${CMAKE_PROJECT_VERSION}")

target_link_libraries(Catozzo PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
)

set_target_properties(Catozzo PROPERTIES
    WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>
    MACOSX_BUNDLE TRUE
)
