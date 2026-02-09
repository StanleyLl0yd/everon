#pragma once

#include <windows.h>
#include <cstddef>
#include <array>

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
    void OnHotkeyEnableChanged(HWND dialog);
    void OnTimerModeChanged(HWND dialog);
    void PopulateLanguageComboBox(HWND dialog);
    void PopulateKeyComboBox(HWND comboBox, WORD selectedKey);
    void PopulateHotkeyComboBox(HWND dialog);
    void InitializeTimerControls(HWND dialog);
    void UpdateTimerControlsState(HWND dialog);
    void UpdateDialogText(HWND dialog);
    void CaptureBaseLayout(HWND dialog);
    void RestoreBaseLayout(HWND dialog);

    struct LayoutSlot {
        int controlId = 0;
        RECT rect = {};
        bool valid = false;
    };

    static constexpr size_t TRACKED_LAYOUT_SLOTS = 14;

    HINSTANCE m_instance = nullptr;
    Settings* m_settings = nullptr;
    bool m_baseLayoutCaptured = false;
    std::array<LayoutSlot, TRACKED_LAYOUT_SLOTS> m_baseLayout = {};
};

} // namespace Everon
