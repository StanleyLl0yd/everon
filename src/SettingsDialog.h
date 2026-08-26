#pragma once

#include <windows.h>

namespace Everon {

class Settings;

class SettingsDialog {
public:
    explicit SettingsDialog(HINSTANCE instance);
    bool Show(HWND parent, Settings& settings);

private:
    static INT_PTR CALLBACK DialogProc(HWND dialog, UINT message,
                                      WPARAM wParam, LPARAM lParam);
    void OnInitDialog(HWND dialog);
    bool OnOkClicked(HWND dialog);
    void OnLanguageChanged(HWND dialog);
    void OnKeyPressChanged(HWND dialog);
    void OnTimerModeChanged(HWND dialog);
    void PopulateLanguageComboBox(HWND dialog);
    void PopulateKeyComboBox(HWND comboBox, WORD selectedKey);
    void PopulateHotkeyComboBox(HWND dialog);
    void InitializeTimerControls(HWND dialog);
    void UpdateKeyPressControlsState(HWND dialog);
    void UpdateTimerControlsState(HWND dialog);
    void UpdateDialogText(HWND dialog);

    HINSTANCE m_instance = nullptr;
    Settings* m_settings = nullptr;
};

}
