# Everon

[![Build](https://github.com/StanleyLl0yd/everon/actions/workflows/build.yml/badge.svg)](https://github.com/StanleyLl0yd/everon/actions/workflows/build.yml)

[English](README.md) | [Русский](README.ru.md)

Everon is a small native Windows tray utility that prevents automatic system sleep while it is enabled. It runs without a main window and is controlled from the system tray.

## Features

- Prevents automatic system sleep using the Windows `SetThreadExecutionState` API
- Can optionally keep the display active
- Can optionally send F15, F16, or F17 through `SendInput` at a configurable interval from 1 second to 24 hours
- Timer modes:
  - Indefinitely
  - For a selected duration
  - Until the next occurrence of a selected local time
- Enable or disable Everon from the tray menu
- Single-click the tray icon to open Settings
- Uses a visually muted tray icon while Everon is disabled
- Optional configurable global hotkey to enable/disable Everon
- Optional start with Windows for the current user
- Optional notifications when Everon is enabled or disabled; timer expiration and relevant errors are also reported through notifications
- Six interface languages: English, Russian, French, German, Italian, and Spanish
- Single-instance operation: starting Everon again opens Settings in the running instance
- Restores its tray icon after Windows Explorer restarts
- Stores settings for the current user under `HKCU\Software\Everon`

Everon is a keep-awake utility. It does not intercept manual lock, sleep, sign-out, or shutdown commands and is not intended to bypass system or organization policies.

## Usage

1. Download `Everon.exe` from the latest GitHub release.
2. Run it.
3. Use the Everon icon in the Windows system tray:
   - Right-click to enable/disable Everon, open **Settings** or **About**, or exit.
   - Single-click to open **Settings**.
4. Configure display behavior, optional key presses, timer mode, hotkey, notifications, language, and autostart as needed.

If **Start with Windows** is enabled, keep `Everon.exe` in a stable location. If the executable is moved, enable autostart again so the stored path is updated.

## Distribution

- Portable application; no installer is required
- Published GitHub release: Windows x64 executable
- Administrator privileges are not required

## Build from Source

### Prerequisites

- Visual Studio or Visual Studio Build Tools with **Desktop development with C++**
- CMake 3.21 or newer

Everon is built as a C++17 Win32 application.

```powershell
cmake -S . -B build -A x64 -DBUILD_TESTING=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

The executable is normally created at:

```text
build\Release\Everon.exe
```

The test suite currently covers timer behavior and hotkey parsing/serialization. GitHub Actions performs the x64 Release build and runs the tests on Windows.

## Author

**Stanley Lloyd**

## Changelog

See [CHANGELOG.md](CHANGELOG.md). The Russian version is available in [CHANGELOG.ru.md](CHANGELOG.ru.md).

## License

Everon is licensed under the **PolyForm Noncommercial License 1.0.0**.

Noncommercial use is permitted under the license terms. Commercial use is not granted by this license and requires separate permission from the licensor.

See [LICENSE](LICENSE) for the complete terms and required notice.
