# Everon

[![CI](https://github.com/StanleyLl0yd/everon/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/StanleyLl0yd/everon/actions/workflows/build.yml)
[![Windows x64](https://img.shields.io/badge/Windows-x64-0078D4?logo=windows11&logoColor=white)](https://github.com/StanleyLl0yd/everon/releases)
[![macOS Universal 2](https://img.shields.io/badge/macOS-Universal%202-000000?logo=apple&logoColor=white)](https://github.com/StanleyLl0yd/everon/releases)
[![License](https://img.shields.io/badge/license-PolyForm%20Noncommercial%201.0.0-blue)](LICENSE)

[English](README.md) · [Русский](README.ru.md)

Everon — лёгкая нативная утилита для Windows и macOS, предотвращающая автоматический сон системы. Она работает без главного окна и управляется из системного трея Windows или строки меню macOS.

**Версия исходного кода:** 2.9.0 · **Платформы:** Windows x64, macOS Universal 2 (arm64 + x86_64) · **Языки:** C++20 / Objective-C

[GitHub Releases](https://github.com/StanleyLl0yd/everon/releases)

## Возможности

### Общие

- Предотвращает автоматический сон системы, пока Everon включён.
- При необходимости может удерживать дисплей активным.
- Три режима таймера: бессрочно, на выбранную длительность и до указанного времени.
- Быстрые таймеры на 15 минут, 30 минут, 1 час и 2 часа, а также произвольная длительность и режим «До времени».
- Опциональная отправка F15, F16 или F17 с интервалом от 1 секунды до 24 часов.
- Опциональная глобальная горячая клавиша, уведомления и автозапуск.
- Шесть языков интерфейса: английский, русский, французский, немецкий, итальянский и испанский.
- «О программе» с версией, автором, лицензией, сайтом приложения и политикой конфиденциальности.

### Windows

- Для удержания системы активной используется `SetThreadExecutionState`.
- Настройки питания позволяют учитывать Windows Battery Saver и отключать удержание дисплея активным при работе от батареи.
- Таймер длительности использует монотонные часы во время работы и сохранённый UTC-дедлайн для восстановления после перезапуска.
- Живой статус в трее, отметка активного быстрого таймера, восстановление после перезапуска Explorer и режим одного экземпляра.
- Настройки хранятся в `HKCU\Software\Everon`.

### macOS

- Нативное приложение строки меню на Objective-C, AppKit и IOKit без сторонних библиотек.
- Для удержания системы и, при необходимости, дисплея активными используются IOKit power assertions.
- Universal 2 для Apple Silicon (`arm64`) и Intel (`x86_64`).
- Настройки хранятся через `NSUserDefaults`.
- Автозапуск реализован через `ServiceManagement`, уведомления — через `UserNotifications`.
- Глобальная горячая клавиша использует системные API macOS без сторонней библиотеки shortcuts.
- До появления сертификата Developer ID сборка macOS намеренно остаётся неподписанной и ненотаризованной.

## Поведение системы

Everon предотвращает автоматический сон при бездействии, пока приложение включено. Оно не перехватывает явные команды пользователя на блокировку, сон, выход из системы, перезагрузку или выключение и не предназначено для обхода системных или корпоративных политик.

На macOS для опциональной синтетической отправки клавиш в зависимости от политики системы может потребоваться разрешение Accessibility. Основное удержание системы активной через IOKit этого разрешения не требует.

## Использование

### Windows

1. Скачайте `Everon.exe` из последнего GitHub Release.
2. Запустите приложение и управляйте им через значок в системном трее.
3. В Settings настраиваются дисплей, батарея, синтетические клавиши, таймеры, горячая клавиша, уведомления, язык и автозапуск.

### macOS

1. Скачайте `Everon-macOS-universal.zip` из последнего GitHub Release и распакуйте `Everon.app`.
2. Поскольку текущая сборка неподписана, macOS может заблокировать первый запуск. Разрешите его через **System Settings → Privacy & Security → Open Anyway**.
3. Используйте значок Everon в строке меню для включения/отключения, запуска таймера, открытия Settings/About или выхода.

## Сборка из исходного кода

### Windows

Требуются Visual Studio или Visual Studio Build Tools с компонентом **Desktop development with C++**, а также CMake 3.21 или новее.

```powershell
cmake -S . -B build -A x64 -DBUILD_TESTING=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Исполняемый файл обычно создаётся как `build\Release\Everon.exe`.

### macOS

Требуются macOS 13 или новее, Xcode Command Line Tools и CMake 3.21 или новее.

```bash
cmake -S . -B build-macos -DCMAKE_BUILD_TYPE=Release
cmake --build build-macos --parallel
```

Universal 2 bundle создаётся как `build-macos/macos/Everon.app`.

Homebrew, CocoaPods, Swift Package Manager и другие сторонние runtime/build-зависимости самому приложению не требуются.

## Проверка SHA-256 релиза

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

## История изменений

[English](CHANGELOG.md) · [Русский](CHANGELOG.ru.md)

## Лицензия

Проект распространяется по лицензии **PolyForm Noncommercial License 1.0.0**. Полные условия приведены в [LICENSE](LICENSE).

Copyright © 2026 Stanley Lloyd.

## Автор

**Stanley Lloyd**
