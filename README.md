# Everon

[![CI](https://github.com/StanleyLl0yd/everon/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/StanleyLl0yd/everon/actions/workflows/build.yml)
[![Windows x64](https://img.shields.io/badge/Windows-x64-0078D4?logo=windows11&logoColor=white)](https://github.com/StanleyLl0yd/everon/releases)
[![macOS Universal 2](https://img.shields.io/badge/macOS-Universal%202-000000?logo=apple&logoColor=white)](https://github.com/StanleyLl0yd/everon/releases)
[![License](https://img.shields.io/badge/license-PolyForm%20Noncommercial%201.0.0-blue)](LICENSE)

[English](README.md) · [Русский](README.ru.md)

Everon is a lightweight native keep-awake utility for Windows and macOS. It runs without a main window and is controlled from the Windows system tray or macOS menu bar.

**Source version:** 2.9.0 · **Platforms:** Windows x64, macOS Universal 2 (arm64 + x86_64) · **Languages:** C++20 / Objective-C

[GitHub Releases](https://github.com/StanleyLl0yd/everon/releases)

## Features

### Common

- Prevents automatic system sleep while Everon is enabled.
- Can optionally keep the display active.
- Three timer modes: indefinite, duration, and until time.
- Quick timers for 15 minutes, 30 minutes, 1 hour, and 2 hours, plus custom duration and until-time dialogs.
- Optional F15, F16, or F17 synthetic key presses at a configurable interval from 1 second to 24 hours.
- Optional global hotkey, notifications, and start-at-login/start-with-Windows behavior.
- Six interface languages: English, Russian, French, German, Italian, and Spanish.
- About UI with version, author, license, application website, and Privacy Policy links.

### Windows

- Uses `SetThreadExecutionState` for keep-awake behavior.
- Battery-aware options can respect Windows Battery Saver and suppress display keep-awake while running on battery.
- Duration timers use a monotonic clock while running and a persisted UTC deadline for restart recovery.
- Live tray status, active quick-timer indication, Explorer restart recovery, and single-instance forwarding.
- Settings are stored under `HKCU\Software\Everon`.

### macOS

- Native menu-bar application built with Objective-C, AppKit, and IOKit only.
- Uses IOKit power assertions for system and optional display keep-awake behavior.
- Universal 2 binary for both Apple Silicon (`arm64`) and Intel (`x86_64`).
- Settings are stored with `NSUserDefaults`.
- Start at login uses `ServiceManagement`; notifications use `UserNotifications`.
- Global hotkeys use the built-in macOS event APIs; no third-party shortcut library is included.
- The macOS build is intentionally unsigned and not notarized until a Developer ID certificate is available.

## System behavior

Everon prevents automatic idle sleep while enabled. It does not intercept explicit user requests to lock, sleep, sign out, restart, or shut down, and it is not intended to bypass system or organization policies.

On macOS, optional synthetic key presses may require Accessibility permission depending on system policy. Core IOKit keep-awake behavior does not require Accessibility permission.

## Usage

### Windows

1. Download `Everon.exe` from the latest GitHub Release.
2. Run it and use the tray icon to control Everon.
3. Open Settings to configure display behavior, battery policy, synthetic key presses, timers, hotkey, notifications, language, and autostart.

### macOS

1. Download `Everon-macOS-universal.zip` from the latest GitHub Release and extract `Everon.app`.
2. Because the current build is unsigned, macOS may block the first launch. Allow it in **System Settings → Privacy & Security → Open Anyway**.
3. Use the Everon menu-bar icon to enable/disable keep-awake mode, start a timer, open Settings or About, or quit.

## Build from source

### Windows

Requirements: Visual Studio or Visual Studio Build Tools with **Desktop development with C++**, and CMake 3.21 or newer.

```powershell
cmake -S . -B build -A x64 -DBUILD_TESTING=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

The executable is normally created at `build\Release\Everon.exe`.

### macOS

Requirements: macOS 13 or newer, Xcode Command Line Tools, and CMake 3.21 or newer.

```bash
cmake -S . -B build-macos -DCMAKE_BUILD_TYPE=Release
cmake --build build-macos --parallel
```

The Universal 2 application bundle is created at `build-macos/macos/Everon.app`.

No Homebrew, CocoaPods, Swift Package Manager, or other third-party runtime/build dependency is required for the application itself.

## Verify release checksums

Windows:

```powershell
(Get-FileHash .\Everon.exe -Algorithm SHA256).Hash.ToLower()
Get-Content .\Everon.exe.sha256
```

macOS:

```bash
shasum -a 256 Everon-macOS-universal.zip
cat Everon-macOS-universal.zip.sha256
```

## Changelog

[English](CHANGELOG.md) · [Русский](CHANGELOG.ru.md)

## License

Licensed under the **PolyForm Noncommercial License 1.0.0**. See [LICENSE](LICENSE) for the full terms.

Copyright © 2026 Stanley Lloyd.

## Author

**Stanley Lloyd**
