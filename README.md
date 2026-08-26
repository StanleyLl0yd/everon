# Everon

[![Windows CI](https://github.com/StanleyLl0yd/everon/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/StanleyLl0yd/everon/actions/workflows/build.yml)
[![Windows x64](https://img.shields.io/badge/Windows-x64-0078D4?logo=windows11&logoColor=white)](https://github.com/StanleyLl0yd/everon/releases)
[![License](https://img.shields.io/badge/license-PolyForm%20Noncommercial%201.0.0-blue)](LICENSE)

[English](README.md) · [Русский](README.ru.md)

A lightweight native Windows tray utility that prevents automatic system sleep while it is enabled. Everon runs without a main window and is controlled from the system tray.

**Source version:** 2.6.1 · **Platform:** Windows x64 · **Language:** C++17

[GitHub Releases](https://github.com/StanleyLl0yd/everon/releases)

## Features

- Prevents automatic system sleep using the Windows `SetThreadExecutionState` API.
- Can optionally keep the display active.
- Can optionally send F15, F16, or F17 through `SendInput` at a configurable interval from **1 second to 24 hours**.
- Three timer modes: **Indefinitely**, **For duration**, and **Until time**.
- Enable or disable Everon from the tray menu.
- Single-click the tray icon to open **Settings**.
- Uses a visually muted tray icon while Everon is disabled.
- Optional configurable global hotkey to enable or disable Everon.
- Optional start with Windows for the current user.
- Optional notifications when Everon is enabled or disabled; timer expiration and relevant errors are also reported through notifications.
- Six interface languages: English, Russian, French, German, Italian, and Spanish.
- Single-instance operation: starting Everon again opens **Settings** in the running instance.
- Restores its tray icon after Windows Explorer restarts.
- Stores settings for the current user under `HKCU\Software\Everon`.
- **About** dialog with the installed version, purpose, author, license, and a clickable GitHub repository link.

## System behavior

Everon is a keep-awake utility. It prevents automatic system sleep while enabled, but does **not** intercept manual lock, sleep, sign-out, or shutdown commands and is not intended to bypass system or organization policies.

The application is portable, requires no installer, and does not require administrator privileges. If **Start with Windows** is enabled, keep `Everon.exe` in a stable location; after moving the executable, enable autostart again so the stored path is updated.

## Usage

1. Download `Everon.exe` from the latest [GitHub Release](https://github.com/StanleyLl0yd/everon/releases).
2. Run it.
3. Use the Everon icon in the Windows system tray:
   - right-click to enable or disable Everon, open **Settings** or **About**, or exit;
   - single-click to open **Settings**.
4. Configure display behavior, optional key presses, timer mode, hotkey, notifications, language, and autostart as needed.

## Build from source

Requirements: Visual Studio or Visual Studio Build Tools with **Desktop development with C++**, and CMake 3.21 or newer.

```powershell
cmake -S . -B build -A x64 -DBUILD_TESTING=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

The executable is normally created at:

```text
build\Release\Everon.exe
```

Project checks used by CI include the Windows x64 Release build, timer tests, hotkey parsing/serialization tests, and SHA-256 generation for the build artifact.

Main stack: C++17, native Win32 API, CMake, and CTest.

## Changelog

[English](CHANGELOG.md) · [Русский](CHANGELOG.ru.md)

## License

Licensed under the **PolyForm Noncommercial License 1.0.0**. See [LICENSE](LICENSE) for the full terms.

Copyright © 2026 Stanley Lloyd.

## Author

**Stanley Lloyd**