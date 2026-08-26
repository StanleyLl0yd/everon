# Everon

[![Build](https://github.com/StanleyLl0yd/everon/actions/workflows/build.yml/badge.svg)](https://github.com/StanleyLl0yd/everon/actions/workflows/build.yml)

[English](README.md) | [Русский](README.ru.md)

Everon is a lightweight native Windows tray utility that keeps your computer awake when you need it. It runs quietly in the background and gives quick access to its controls from the system tray.

## Features

- Prevent automatic system sleep while enabled
- Optionally keep the display on
- Optionally send periodic F15/F16/F17 key presses
- Timer modes:
  - Indefinitely
  - For a selected duration
  - Until a selected time of day
- Toggle Everon from the tray menu
- Global hotkey for quick enable/disable
- Optional start with Windows
- Notifications for important events
- Six interface languages: English, Russian, French, German, Italian, and Spanish
- Single-instance behavior: launching Everon again opens the settings of the running instance
- Tray icon restoration after Windows Explorer restarts

## Typical Use Cases

- Long downloads or uploads
- Presentations and meetings
- Monitoring and automation tasks
- Media playback
- Keeping a workstation awake during short breaks

## Requirements

- Windows 10 or Windows 11
- The published build is x64
- Administrator privileges are not required

Everon is portable: there is no installer and the application stores its settings in the current user's Windows registry under `HKCU\Software\Everon`.

## Quick Start

1. Download `Everon.exe` from the latest GitHub release.
2. Run it.
3. Find the Everon icon in the Windows system tray.
4. Right-click the icon to enable/disable Everon or open **Settings**.
5. Configure the timer, display behavior, optional key press, hotkey, notifications, language, and Windows autostart as needed.

If **Start with Windows** is enabled, keep `Everon.exe` in a stable location. Moving the executable changes its path and the autostart entry should then be enabled again from Settings.

## How It Works

Everon uses the standard Windows `SetThreadExecutionState` API to request that the system stay awake. When **Keep display on** is enabled, it also requests that the display remain active. The optional F15/F16/F17 mode uses `SendInput` and is disabled by default.

The timer stores its active deadline as an absolute UTC value so duration and until-time modes remain stable across restarts and common clock/DST edge cases.

## Build from Source

### Prerequisites

- Visual Studio 2022 or Visual Studio Build Tools 2022
- **Desktop development with C++** workload
- CMake 3.21 or newer

### Configure, build, and test

```powershell
cmake -S . -B build -A x64 -DBUILD_TESTING=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

The resulting executable is normally located at:

```text
build\Release\Everon.exe
```

The repository also includes a GitHub Actions workflow that performs the same x64 Release build and runs the tests on `windows-latest`.

## Project Structure

```text
src/                 Win32/C++ application sources and resources
tests/               Timer regression tests
.github/workflows/    CI build workflow
CMakeLists.txt        Reproducible build definition
```

## Author

**Stanley Lloyd**

## Changelog

See [CHANGELOG.md](CHANGELOG.md). A Russian version is available in [CHANGELOG.ru.md](CHANGELOG.ru.md).

## License

Everon is licensed under the **PolyForm Noncommercial License 1.0.0**.

Noncommercial use is permitted under the license terms. Commercial use is not granted by this license and requires separate permission from the licensor.

See [LICENSE](LICENSE) for the complete terms and required notice.
