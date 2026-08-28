cmake_minimum_required(VERSION 3.21)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/src/Version.h" EVERON_VERSION_DEFINES
    REGEX "^#define VER_(MAJOR|MINOR|PATCH) [0-9]+$")
foreach(line IN LISTS EVERON_VERSION_DEFINES)
    if(line MATCHES "^#define VER_MAJOR ([0-9]+)$")
        set(EVERON_VERSION_MAJOR "${CMAKE_MATCH_1}")
    elseif(line MATCHES "^#define VER_MINOR ([0-9]+)$")
        set(EVERON_VERSION_MINOR "${CMAKE_MATCH_1}")
    elseif(line MATCHES "^#define VER_PATCH ([0-9]+)$")
        set(EVERON_VERSION_PATCH "${CMAKE_MATCH_1}")
    endif()
endforeach()
if(NOT DEFINED EVERON_VERSION_MAJOR OR NOT DEFINED EVERON_VERSION_MINOR OR NOT DEFINED EVERON_VERSION_PATCH)
    message(FATAL_ERROR "Unable to read Everon version from src/Version.h")
endif()

project(Everon VERSION "${EVERON_VERSION_MAJOR}.${EVERON_VERSION_MINOR}.${EVERON_VERSION_PATCH}" LANGUAGES CXX RC)

if(NOT WIN32)
    message(FATAL_ERROR "Everon is a native Windows application and can only be built on Windows.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(EVERON_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

function(everon_configure_target target)
    target_compile_definitions(${target} PRIVATE UNICODE _UNICODE)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8 /sdl /guard:cf)
        if(EVERON_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
        target_link_options(${target} PRIVATE /guard:cf /CETCOMPAT)
    endif()
endfunction()

function(everon_configure_test_target target)
    everon_configure_target(${target})
    if(MSVC)
        target_compile_options(${target} PRIVATE /Zi)
        target_link_options(${target} PRIVATE /DEBUG:FULL /PROFILE)
    endif()
endfunction()

set(EVERON_SOURCES
    src/main.cpp
    src/App.cpp
    src/AboutDialog.cpp
    src/HotkeyManager.cpp
    src/Localization.cpp
    src/PowerManager.cpp
    src/QuickTimerDialog.cpp
    src/Settings.cpp
    src/SettingsDialog.cpp
    src/TimerMode.cpp
    src/TrayIcon.cpp
    src/Utils.cpp
    src/app.rc
)

add_executable(Everon WIN32 ${EVERON_SOURCES})
target_include_directories(Everon PRIVATE src)
target_link_libraries(Everon PRIVATE comctl32 shell32 advapi32)
everon_configure_target(Everon)

if(MSVC)
    target_link_options(Everon PRIVATE
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
    target_compile_definitions(TimerModeTests PRIVATE EVERON_TESTING)
    everon_configure_test_target(TimerModeTests)
    add_test(NAME TimerModeTests COMMAND TimerModeTests)

    add_executable(HotkeyManagerTests
        tests/HotkeyManagerTests.cpp
        src/HotkeyManager.cpp
        src/Utils.cpp
    )
    target_include_directories(HotkeyManagerTests PRIVATE src)
    everon_configure_test_target(HotkeyManagerTests)
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
    target_compile_definitions(SettingsTests PRIVATE EVERON_TESTING)
    target_link_libraries(SettingsTests PRIVATE advapi32)
    everon_configure_test_target(SettingsTests)
    add_test(NAME SettingsTests COMMAND SettingsTests)

    add_executable(PowerManagerTests
        tests/PowerManagerTests.cpp
        src/PowerManager.cpp
        src/Utils.cpp
    )
    target_include_directories(PowerManagerTests PRIVATE src)
    everon_configure_test_target(PowerManagerTests)
    add_test(NAME PowerManagerTests COMMAND PowerManagerTests)

    add_executable(PowerPolicyTests
        tests/PowerPolicyTests.cpp
    )
    target_include_directories(PowerPolicyTests PRIVATE src)
    everon_configure_test_target(PowerPolicyTests)
    add_test(NAME PowerPolicyTests COMMAND PowerPolicyTests)

    add_executable(LocalizationTests
        tests/LocalizationTests.cpp
        src/Localization.cpp
    )
    target_include_directories(LocalizationTests PRIVATE src)
    everon_configure_test_target(LocalizationTests)
    add_test(NAME LocalizationTests COMMAND LocalizationTests)

    add_executable(OwnershipTests
        tests/OwnershipTests.cpp
    )
    target_include_directories(OwnershipTests PRIVATE src)
    everon_configure_test_target(OwnershipTests)
    add_test(NAME OwnershipTests COMMAND OwnershipTests)

    add_executable(SingleInstanceTests
        tests/SingleInstanceTests.cpp
        src/Utils.cpp
    )
    target_include_directories(SingleInstanceTests PRIVATE src)
    target_link_libraries(SingleInstanceTests PRIVATE shell32)
    everon_configure_test_target(SingleInstanceTests)
    add_test(NAME SingleInstanceTests COMMAND SingleInstanceTests)
endif()
