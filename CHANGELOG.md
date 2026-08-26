# Changelog

[English](CHANGELOG.md) | [Русский](CHANGELOG.ru.md)

All notable changes to Everon are documented in this file.

## 2.5 — 2026-08-26

### Changed

- Standardized all application and executable metadata on the PolyForm Noncommercial License 1.0.0.
- Updated the application version to 2.5.
- Refined the settings dialog layout to give localized labels more room.
- Updated product metadata to describe Everon accurately as a utility that keeps the PC awake.
- Expanded English and Russian documentation with build, testing, compatibility, and portable-use information.

### Added

- CMake-based reproducible Windows build.
- GitHub Actions workflow for x64 Release builds.
- CTest-based `TimerMode` regression tests.
- Automatic SHA-256 generation for CI build artifacts.

### Maintenance

- Removed the generated Visual Studio resource-editor cache file `src/app.aps` from version control.

## 2.4 — 2026-02-10

- Tray utility for preventing automatic sleep.
- Optional display keep-awake mode.
- Optional F15/F16/F17 key presses.
- Indefinite, duration, and until-time timer modes.
- Global toggle hotkey.
- Optional Windows autostart.
- Six interface languages: English, Russian, French, German, Italian, and Spanish.
- Reliability improvements for tray restoration, timer persistence, DST handling, and WinAPI error checking.
