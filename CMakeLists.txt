cmake_minimum_required(VERSION 3.16)

project(jokgu VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
    main.cpp
    MainWindow.cpp
    MainWindow.h
    MainWindow.ui
    TeamMaker.cpp
    TeamMaker.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(jokgu
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(jokgu
        ${PROJECT_SOURCES}
    )
endif()

target_link_libraries(jokgu PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(jokgu PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS jokgu
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_finalize_executable(jokgu)
endif()
