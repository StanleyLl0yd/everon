from pathlib import Path

path = Path("src/Settings.cpp")
text = path.read_text(encoding="utf-8")
old = '''    auto WriteString = [hKey](const wchar_t* name, const wchar_t* value) -> bool {
        DWORD size = static_cast<DWORD>((wcslen(value) + 1) * sizeof(wchar_t));
        const LONG res = RegSetValueExW(hKey, name, 0, REG_SZ,
                                        reinterpret_cast<const BYTE*>(value),
                                        size);
        if (!Utils::CheckWinApiStatus(res, L"RegSetValueExW(REG_SZ)")) {
            Utils::DebugLog(L"[Everon][Reg] Failed to write string value '%s'\\n", name);
            return false;
        }
        return true;
    };
'''
new = '''    auto WriteString = [hKey](const wchar_t* name, const wchar_t* value) -> bool {
        if (value == nullptr) {
            Utils::DebugLog(L"[Everon][Reg] Refusing null string value '%s'\\n", name);
            return false;
        }
        const std::wstring stored(value);
        const DWORD size = static_cast<DWORD>((stored.size() + 1) * sizeof(wchar_t));
        const LONG res = RegSetValueExW(hKey, name, 0, REG_SZ,
                                        reinterpret_cast<const BYTE*>(stored.c_str()),
                                        size);
        if (!Utils::CheckWinApiStatus(res, L"RegSetValueExW(REG_SZ)")) {
            Utils::DebugLog(L"[Everon][Reg] Failed to write string value '%s'\\n", name);
            return false;
        }
        return true;
    };
'''
if old not in text:
    raise SystemExit("Settings WriteString block not found")
path.write_text(text.replace(old, new, 1), encoding="utf-8")
