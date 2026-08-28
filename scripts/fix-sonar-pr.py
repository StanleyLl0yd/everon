from pathlib import Path


def replace(path, old, new, count=1):
    p = Path(path)
    text = p.read_text(encoding="utf-8")
    if text.count(old) < count:
        raise SystemExit(f"Replacement not found in {path}")
    p.write_text(text.replace(old, new, count), encoding="utf-8")


replace(
    "src/App.cpp",
    "        auto& loc = Localization::Instance();\n        m_trayIcon->ShowNotification(loc.GetString(StringID::ErrorTitle),\n                                     loc.GetString(StringID::ErrorTimerState), NIIF_WARNING);",
    "        const auto& loc = Localization::Instance();\n        m_trayIcon->ShowNotification(loc.GetString(StringID::ErrorTitle),\n                                     loc.GetString(StringID::ErrorTimerState), NIIF_WARNING);",
)

replace(
    "src/Settings.cpp",
    "    result = RegQueryValueExW(hKey, APP_NAME, nullptr, &type,\n                              reinterpret_cast<LPBYTE>(value.data()), &readSize);",
    "    result = RegGetValueW(hKey, nullptr, APP_NAME,\n                          RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND,\n                          &type, value.data(), &readSize);",
)

tray = Path("src/TrayIcon.cpp")
text = tray.read_text(encoding="utf-8")
text = text.replace("#include <strsafe.h>\n", "#include <strsafe.h>\n#include <array>\n#include <string_view>\n", 1)
text = text.replace(
    "    auto append = [&status](const std::wstring& value) {\n        if (!value.empty()) {\n            status += L\" \\x2022 \";\n            status += value;\n        }\n    };",
    "    auto append = [&status](std::wstring_view value) {\n        if (!value.empty()) {\n            status += L\" • \";\n            status.append(value);\n        }\n    };",
    1,
)
text = text.replace(
    "            wchar_t part[64] = {};\n            StringCchPrintfW(part, _countof(part), L\"%s/%lu%c\",",
    "            std::array<wchar_t, 64> part{};\n            StringCchPrintfW(part.data(), part.size(), L\"%s/%lu%c\",",
    1,
)
text = text.replace("            append(part);", "            append(part.data());", 1)
text = text.replace("                wchar_t part[96] = {};", "                std::array<wchar_t, 96> part{};", 1)
text = text.replace("StringCchPrintfW(part, _countof(part),", "StringCchPrintfW(part.data(), part.size(),", 2)
text = text.replace("                append(part);", "                append(part.data());", 1)
text = text.replace("            wchar_t part[128] = {};", "            std::array<wchar_t, 128> part{};", 1)
text = text.replace("StringCchPrintfW(part, _countof(part),", "StringCchPrintfW(part.data(), part.size(),", 2)
text = text.replace("            append(part);", "            append(part.data());", 1)
tray.write_text(text, encoding="utf-8")

version = Path("src/Version.h")
text = version.read_text(encoding="utf-8")
old = '''#if VER_PATCH == 0
#define VER_VERSION_STR EVERON_STRINGIZE(VER_MAJOR) "." EVERON_STRINGIZE(VER_MINOR)
#else
#define VER_VERSION_STR EVERON_STRINGIZE(VER_MAJOR) "." EVERON_STRINGIZE(VER_MINOR) "." EVERON_STRINGIZE(VER_PATCH)
#endif

#define VER_VERSION_STR_W EVERON_WIDEN(VER_VERSION_STR)
'''
new = '''#if VER_PATCH == 0
#define VER_VERSION_STR EVERON_STRINGIZE(VER_MAJOR) "." EVERON_STRINGIZE(VER_MINOR)
#define VER_VERSION_STR_W EVERON_WIDEN(EVERON_STRINGIZE(VER_MAJOR)) L"." EVERON_WIDEN(EVERON_STRINGIZE(VER_MINOR))
#else
#define VER_VERSION_STR EVERON_STRINGIZE(VER_MAJOR) "." EVERON_STRINGIZE(VER_MINOR) "." EVERON_STRINGIZE(VER_PATCH)
#define VER_VERSION_STR_W EVERON_WIDEN(EVERON_STRINGIZE(VER_MAJOR)) L"." EVERON_WIDEN(EVERON_STRINGIZE(VER_MINOR)) L"." EVERON_WIDEN(EVERON_STRINGIZE(VER_PATCH))
#endif
'''
if old not in text:
    raise SystemExit("Version macro block not found")
version.write_text(text.replace(old, new, 1), encoding="utf-8")
