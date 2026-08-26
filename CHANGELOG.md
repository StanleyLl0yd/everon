# Changelog

[English](CHANGELOG.md) | [Русский](CHANGELOG.ru.md)

All notable changes to Everon are documented in this file.

## 2.6.0 — 2026-08-26

### Changed

- Refreshed the native Settings dialog with a wider Segoe UI layout and removed runtime control repositioning.
- Simplified global hotkey configuration: selecting `None` disables it; any selected hotkey enables it.
- Disabled the key-press interval controls when synthetic key presses are turned off.
- A single tray-icon click now opens Settings; keyboard tray activation is also supported.
- Added a visually muted tray icon for the disabled state.
- Simplified About to show the version, purpose, author, license, and GitHub repository.

## 2.5.1 — 2026-08-26

### Fixed

- Preserved unsaved key-press and hotkey selections when changing the interface language.
- Hardened registry string parsing against malformed non-terminated values.
- Removed the `MAX_PATH` limitation from executable-path detection used by autostart.

### Security and reliability

- Added explicit `asInvoker` / `uiAccess=false` execution policy to the application manifest.
- Enabled MSVC SDL, Control Flow Guard, and CET compatibility hardening.
- Added HotkeyManager regression tests and stricter validation of persisted hotkey values.
- Made GitHub releases tag-driven and immutable: existing release assets are no longer overwritten.

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
