from pathlib import Path

path = Path("src/Settings.cpp")
text = path.read_text(encoding="utf-8")
old = '''        const std::wstring stored(value);
        const DWORD size = static_cast<DWORD>((stored.size() + 1) * sizeof(wchar_t));
        const LONG res = RegSetValueExW(hKey, name, 0, REG_SZ,
                                        reinterpret_cast<const BYTE*>(stored.c_str()),
                                        size);
'''
new = '''        const auto stored = std::wstring(value);
        const DWORD size = static_cast<DWORD>((stored.size() + 1) * sizeof(wchar_t));
        const LONG res = RegSetKeyValueW(hKey, nullptr, name, REG_SZ,
                                         stored.c_str(), size);
'''
if old not in text:
    raise SystemExit("Settings registry string write block not found")
path.write_text(text.replace(old, new, 1), encoding="utf-8")
