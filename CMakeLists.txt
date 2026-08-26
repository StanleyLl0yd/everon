cmake_minimum_required(VERSION 3.21)

project(Everon VERSION 2.5.0 LANGUAGES CXX RC)

if(NOT WIN32)
    message(FATAL_ERROR "Everon is a native Windows application and can only be built on Windows.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(EVERON_SOURCES
    src/main.cpp
    src/App.cpp
    src/HotkeyManager.cpp
    src/Localization.cpp
    src/PowerManager.cpp
    src/Settings.cpp
    src/SettingsDialog.cpp
    src/TimerMode.cpp
    src/TrayIcon.cpp
    src/Utils.cpp
    src/app.rc
)

add_executable(Everon WIN32 ${EVERON_SOURCES})
target_include_directories(Everon PRIVATE src)
target_compile_definitions(Everon PRIVATE UNICODE _UNICODE)
target_link_libraries(Everon PRIVATE comctl32 shell32 advapi32)

if(MSVC)
    target_compile_options(Everon PRIVATE /W4 /permissive- /utf-8)
endif()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Everon)

include(CTest)

if(BUILD_TESTING)
    add_executable(TimerModeTests
        tests/TimerModeTests.cpp
        src/TimerMode.cpp
    )
    target_include_directories(TimerModeTests PRIVATE src)
    target_compile_definitions(TimerModeTests PRIVATE UNICODE _UNICODE)

    if(MSVC)
        target_compile_options(TimerModeTests PRIVATE /W4 /permissive- /utf-8)
    endif()

    add_test(NAME TimerModeTests COMMAND TimerModeTests)
endif()
