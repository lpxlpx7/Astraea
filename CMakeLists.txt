cmake_minimum_required(VERSION 3.21)

project(Astraea VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

option(ASTRAEA_FETCH_QLEMENTINE "Fetch Qlementine when it is available" OFF)
option(ASTRAEA_DEPLOY_QT "Deploy Qt runtime beside the executable on Windows" ON)
if(ASTRAEA_FETCH_QLEMENTINE)
    include(FetchContent)
    FetchContent_Declare(qlementine
        GIT_REPOSITORY https://github.com/oclero/qlementine.git
        GIT_TAG main)
    FetchContent_MakeAvailable(qlementine)
endif()

add_library(astraea_plugin_api INTERFACE)
target_include_directories(astraea_plugin_api INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(APP_SOURCES
    src/main.cpp
    src/app/application.cpp
    src/app/application.h
    src/app/mainwindow.cpp
    src/app/mainwindow.h
    src/app/pluginmanager.cpp
    src/app/pluginmanager.h
    src/app/theme.cpp
    src/app/theme.h
    src/plugins/plugininterface.h
    src/resources/resources.qrc
)

if(WIN32)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/img/logo-bright.png
        ${CMAKE_CURRENT_BINARY_DIR}/astraea-logo-source.png COPYONLY)
    add_executable(astraea ${APP_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/astraea.rc)
else()
    add_executable(astraea ${APP_SOURCES})
endif()
target_link_libraries(astraea PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network astraea_plugin_api)
if(TARGET qlementine)
    target_link_libraries(astraea PRIVATE qlementine)
    target_compile_definitions(astraea PRIVATE ASTRAEA_HAS_QLEMENTINE=1)
endif()

set_target_properties(astraea PROPERTIES
    WIN32_EXECUTABLE TRUE
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


if(WIN32 AND ASTRAEA_DEPLOY_QT)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt
        HINTS "${CMAKE_PREFIX_PATH}/bin" "$ENV{QTDIR}/bin")
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET astraea POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --release "$<TARGET_FILE:astraea>"
            COMMENT "Deploying Qt runtime with windeployqt")
    else()
        message(WARNING "windeployqt was not found; Qt runtime will not be copied automatically")
    endif()
endif()
