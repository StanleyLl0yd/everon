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
    void OnInitDialog(HWND dialog) const;
    bool OnOkClicked(HWND dialog) const;
    void OnLanguageChanged(HWND dialog) const;
    void OnKeyPressChanged(HWND dialog) const;
    void OnTimerModeChanged(HWND dialog) const;
    void PopulateLanguageComboBox(HWND dialog) const;
    void PopulateKeyComboBox(HWND comboBox, WORD selectedKey) const;
    void PopulateHotkeyComboBox(HWND dialog) const;
    void InitializeTimerControls(HWND dialog) const;
    void UpdateKeyPressControlsState(HWND dialog) const;
    void UpdatePowerControlsState(HWND dialog) const;
    void UpdateTimerControlsState(HWND dialog) const;
    void UpdateUntilHint(HWND dialog) const;
    void UpdateDialogText(HWND dialog) const;

    HINSTANCE m_instance = nullptr;
    Settings* m_settings = nullptr;
};

}