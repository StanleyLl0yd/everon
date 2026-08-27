cmake_minimum_required(VERSION 3.21)

project(Everon VERSION 2.7.0 LANGUAGES CXX RC)

if(NOT WIN32)
    message(FATAL_ERROR "Everon is a native Windows application and can only be built on Windows.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(EVERON_SOURCES
    src/main.cpp
    src/App.cpp
    src/AboutDialog.cpp
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
    target_compile_options(Everon PRIVATE /W4 /permissive- /utf-8 /sdl /guard:cf)
    target_link_options(Everon PRIVATE
        /guard:cf
        /CETCOMPAT
        /MANIFEST:EMBED
        "/MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/src/app.manifest"
    )
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
        target_compile_options(TimerModeTests PRIVATE /W4 /permissive- /utf-8 /sdl /guard:cf)
        target_link_options(TimerModeTests PRIVATE /guard:cf /CETCOMPAT)
    endif()

    add_test(NAME TimerModeTests COMMAND TimerModeTests)

    add_executable(HotkeyManagerTests
        tests/HotkeyManagerTests.cpp
        src/HotkeyManager.cpp
        src/Utils.cpp
    )
    target_include_directories(HotkeyManagerTests PRIVATE src)
    target_compile_definitions(HotkeyManagerTests PRIVATE UNICODE _UNICODE)
    if(MSVC)
        target_compile_options(HotkeyManagerTests PRIVATE /W4 /permissive- /utf-8 /sdl /guard:cf)
        target_link_options(HotkeyManagerTests PRIVATE /guard:cf /CETCOMPAT)
    endif()
    add_test(NAME HotkeyManagerTests COMMAND HotkeyManagerTests)

    add_executable(SettingsTests
        tests/SettingsTests.cpp
        src/Settings.cpp
        src/Localization.cpp
        src/HotkeyManager.cpp
        src/TimerMode.cpp
        src/Utils.cpp
    )
    target_include_directories(SettingsTests PRIVATE src)
    target_compile_definitions(SettingsTests PRIVATE UNICODE _UNICODE)
    target_link_libraries(SettingsTests PRIVATE advapi32)
    if(MSVC)
        target_compile_options(SettingsTests PRIVATE /W4 /permissive- /utf-8 /sdl /guard:cf)
        target_link_options(SettingsTests PRIVATE /guard:cf /CETCOMPAT)
    endif()
    add_test(NAME SettingsTests COMMAND SettingsTests)

    add_executable(PowerManagerTests
        tests/PowerManagerTests.cpp
        src/PowerManager.cpp
        src/Utils.cpp
    )
    target_include_directories(PowerManagerTests PRIVATE src)
    target_compile_definitions(PowerManagerTests PRIVATE UNICODE _UNICODE)
    if(MSVC)
        target_compile_options(PowerManagerTests PRIVATE /W4 /permissive- /utf-8 /sdl /guard:cf)
        target_link_options(PowerManagerTests PRIVATE /guard:cf /CETCOMPAT)
    endif()
    add_test(NAME PowerManagerTests COMMAND PowerManagerTests)
endif()
