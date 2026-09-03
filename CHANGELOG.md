# Changelog

[![en](https://img.shields.io/badge/lang-en-red.svg)](CHANGELOG.md)
[![ru](https://img.shields.io/badge/lang-ru-blue.svg)](CHANGELOG.ru.md)

All notable changes to Everon are documented in this file.

---

## 2.8.2 — 2026-09-03

### Changed

- Replaced the GitHub repository link in the About dialog with the Everon application website.
- Added a localized Privacy Policy link to the About dialog.
- Regular About-dialog navigation now stays on the public Everon website instead of sending users to GitHub.

## 2.8.1 — 2026-09-02

### Changed

- Replaced the Everon application icon with a new modern high-contrast design optimized for both large and small Windows icon sizes.

### Maintenance

- Refreshed the native Windows icon resource without changing application behavior.

## 2.8.0 — 2026-08-27

### Added

- Added live tray status refresh so active timer information remains current while Everon runs.
- Added a status line to the tray menu and check marks for active quick-duration presets.
- Added native `Custom...` duration and dedicated `Until time...` quick-timer dialogs.
- Added battery-aware options to respect Windows Battery Saver and optionally allow display keep-awake while on battery.
- Added explicit power/resume handling so active timers and keep-awake state are refreshed after resume or power-source changes.
- Added battery-policy regression tests and expanded Settings coverage.

### Changed

- Quick timers no longer require opening the full Settings dialog for custom duration or until-time selection.
- Synthetic F15/F16/F17 input pauses while Battery Saver policy is actively suspending keep-awake behavior.
- The Settings dialog now exposes battery behavior without changing existing defaults for upgraded users.

### Security and maintenance

- Pinned GitHub Actions to immutable commit SHAs and current Node 24 action versions.
- Split release build/test and publish into separate least-privilege jobs.
- Manual releases now build the exact workflow-dispatch commit through `github.sha`.
- Added protected `v*` release-tag rules and expanded regression coverage for Settings and power handling.

## 2.7.0 — 2026-08-26

### Added

- Added quick tray timers for 15 minutes, 30 minutes, 1 hour, and 2 hours.
- Added a localized Tomorrow indicator when an Until time selection resolves to the next day.
- Added automatic retry and a single warning notification when Windows temporarily rejects the requested power state.

### Changed

- Duration timers now use a monotonic clock while Everon is running, so system clock changes do not alter the active duration. A persisted UTC deadline remains the restart recovery source.
- Power-state bookkeeping now changes only after `SetThreadExecutionState` succeeds.
- Expanded timer regression coverage for monotonic timing, restart recovery, and next-day time selection.
- Removed obsolete localization entries that were no longer used by the current interface.

## 2.6.1 — 2026-08-26

### Changed

- Replaced the About message box with a compact native Win32 dialog.
- Made the GitHub repository URL in About a keyboard-accessible clickable `SysLink` that opens in the default browser.

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
