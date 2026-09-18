$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

Add-Type -AssemblyName System.Drawing

Add-Type @"
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

public static class EveronCaptureNative {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }

    private delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll", SetLastError=true)]
    private static extern bool EnumWindows(EnumWindowsProc callback, IntPtr lParam);

    [DllImport("user32.dll", SetLastError=true)]
    public static extern bool IsWindowVisible(IntPtr hWnd);

    [DllImport("user32.dll", SetLastError=true)]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint processId);

    [DllImport("user32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
    public static extern IntPtr FindWindow(string className, string windowName);

    [DllImport("user32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
    private static extern int GetClassName(IntPtr hWnd, StringBuilder className, int maxCount);

    [DllImport("user32.dll", SetLastError=true)]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);

    [DllImport("user32.dll", SetLastError=true)]
    public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdc, uint flags);

    [DllImport("user32.dll", SetLastError=true)]
    public static extern bool PostMessage(IntPtr hWnd, uint message, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll", SetLastError=true)]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

    [DllImport("user32.dll", SetLastError=true)]
    public static extern bool SetCursorPos(int x, int y);

    [DllImport("user32.dll")]
    public static extern void keybd_event(byte virtualKey, byte scanCode, uint flags, UIntPtr extraInfo);

    public static IntPtr[] AllWindowsForProcess(uint processId) {
        var result = new List<IntPtr>();
        EnumWindows((hWnd, lParam) => {
            uint pid;
            GetWindowThreadProcessId(hWnd, out pid);
            if (pid == processId) {
                result.Add(hWnd);
            }
            return true;
        }, IntPtr.Zero);
        return result.ToArray();
    }

    public static IntPtr[] VisibleWindowsForProcess(uint processId) {
        var result = new List<IntPtr>();
        foreach (var hWnd in AllWindowsForProcess(processId)) {
            if (IsWindowVisible(hWnd)) {
                result.Add(hWnd);
            }
        }
        return result.ToArray();
    }

    public static string ClassName(IntPtr hWnd) {
        var buffer = new StringBuilder(256);
        GetClassName(hWnd, buffer, buffer.Capacity);
        return buffer.ToString();
    }
}
"@

function Get-WindowArea([IntPtr]$Handle) {
    $rect = New-Object EveronCaptureNative+RECT
    if (-not [EveronCaptureNative]::GetWindowRect($Handle, [ref]$rect)) {
        return 0
    }
    return [Math]::Max(0, $rect.Right - $rect.Left) * [Math]::Max(0, $rect.Bottom - $rect.Top)
}

function Wait-ProcessWindow([uint32]$ProcessId, [string]$ClassName, [IntPtr]$Exclude = [IntPtr]::Zero) {
    $deadline = (Get-Date).AddSeconds(15)
    do {
        $candidate = [EveronCaptureNative]::VisibleWindowsForProcess($ProcessId) |
            Where-Object {
                $_ -ne $Exclude -and
                [EveronCaptureNative]::ClassName($_) -eq $ClassName -and
                (Get-WindowArea $_) -gt 5000
            } |
            Sort-Object { Get-WindowArea $_ } -Descending |
            Select-Object -First 1
        if ($candidate) {
            return [IntPtr]$candidate
        }
        Start-Sleep -Milliseconds 200
    } while ((Get-Date) -lt $deadline)
    throw "Timed out waiting for visible $ClassName window for process $ProcessId"
}

function Capture-Window([IntPtr]$Handle, [string]$Path) {
    $rect = New-Object EveronCaptureNative+RECT
    if (-not [EveronCaptureNative]::GetWindowRect($Handle, [ref]$rect)) {
        throw "GetWindowRect failed for $Handle"
    }
    $width = $rect.Right - $rect.Left
    $height = $rect.Bottom - $rect.Top
    if ($width -lt 100 -or $height -lt 100) {
        throw "Unexpected capture bounds: $($width)x$($height)"
    }

    $bitmap = New-Object System.Drawing.Bitmap($width, $height)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        $hdc = $graphics.GetHdc()
        try {
            $ok = [EveronCaptureNative]::PrintWindow($Handle, $hdc, 2)
        } finally {
            $graphics.ReleaseHdc($hdc)
        }
        if (-not $ok) {
            $graphics.CopyFromScreen($rect.Left, $rect.Top, 0, 0,
                (New-Object System.Drawing.Size($width, $height)))
        }
        $bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    } finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }

    $bytes = [System.IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 1024 -or
        $bytes[0] -ne 0x89 -or $bytes[1] -ne 0x50 -or
        $bytes[2] -ne 0x4E -or $bytes[3] -ne 0x47) {
        throw "Invalid PNG capture: $Path"
    }
    Write-Host "$(Split-Path $Path -Leaf): $($width)x$($height), $($bytes.Length) bytes"
}

function Send-Key([byte]$VirtualKey) {
    [EveronCaptureNative]::keybd_event($VirtualKey, 0, 0, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 50
    [EveronCaptureNative]::keybd_event($VirtualKey, 0, 2, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 120
}

$root = Join-Path $env:RUNNER_TEMP "everon-capture"
New-Item -ItemType Directory -Force -Path $root | Out-Null
$out = Join-Path $root "screenshots"
New-Item -ItemType Directory -Force -Path $out | Out-Null

$exe = Join-Path $root "Everon.exe"
Invoke-WebRequest -Uri "https://github.com/StanleyLl0yd/everon/releases/download/v2.8.2/Everon.exe" -OutFile $exe
$actualHash = (Get-FileHash $exe -Algorithm SHA256).Hash.ToLowerInvariant()
$expectedHash = "184158acc00c715d24f92dc4bb6097abd7a884d969d897827e56eacae9b7f714"
if ($actualHash -ne $expectedHash) {
    throw "Release EXE SHA-256 mismatch: $actualHash"
}
Write-Host "Verified Everon.exe SHA-256: $actualHash"

if (-not (Get-Process explorer -ErrorAction SilentlyContinue)) {
    Start-Process -FilePath "$env:WINDIR\explorer.exe" | Out-Null
}
$desktopDeadline = (Get-Date).AddSeconds(20)
do {
    $taskbar = [EveronCaptureNative]::FindWindow("Shell_TrayWnd", $null)
    if ($taskbar -ne [IntPtr]::Zero) {
        break
    }
    Start-Sleep -Milliseconds 500
} while ((Get-Date) -lt $desktopDeadline)
if ($taskbar -eq [IntPtr]::Zero) {
    throw "Interactive Windows shell/taskbar is unavailable on this runner"
}
Write-Host "Windows shell taskbar detected."

$process = Start-Process -FilePath $exe -PassThru
try {
    Start-Sleep -Seconds 3
    if ($process.HasExited) {
        throw "Everon exited before capture with code $($process.ExitCode)"
    }
    $allWindows = @([EveronCaptureNative]::AllWindowsForProcess([uint32]$process.Id))
    foreach ($candidate in $allWindows) {
        Write-Host "Everon HWND=$candidate class=$([EveronCaptureNative]::ClassName($candidate)) visible=$([EveronCaptureNative]::IsWindowVisible($candidate))"
    }
    $hidden = $allWindows |
        Where-Object { [EveronCaptureNative]::ClassName($_) -eq "EveronMainWindow" } |
        Select-Object -First 1
    if (-not $hidden) {
        throw "Everon hidden main window was not found by PID enumeration"
    }
    $hidden = [IntPtr]$hidden

    [EveronCaptureNative]::PostMessage($hidden, 0x8002, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    $settings = Wait-ProcessWindow ([uint32]$process.Id) "#32770"
    [EveronCaptureNative]::SetForegroundWindow($settings) | Out-Null
    Start-Sleep -Milliseconds 500
    Capture-Window $settings (Join-Path $out "01-settings.png")

    [EveronCaptureNative]::PostMessage($settings, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Seconds 1

    [EveronCaptureNative]::SetCursorPos(640, 480) | Out-Null
    [EveronCaptureNative]::PostMessage($hidden, 0x8001, [IntPtr]::Zero, [IntPtr]0x0205) | Out-Null
    $menu = Wait-ProcessWindow ([uint32]$process.Id) "#32768"
    Start-Sleep -Milliseconds 400
    Capture-Window $menu (Join-Path $out "02-tray-menu.png")

    Send-Key 0x28
    Send-Key 0x28
    Send-Key 0x28
    Send-Key 0x28
    Send-Key 0x0D

    $about = Wait-ProcessWindow ([uint32]$process.Id) "#32770"
    [EveronCaptureNative]::SetForegroundWindow($about) | Out-Null
    Start-Sleep -Milliseconds 400
    Capture-Window $about (Join-Path $out "03-about.png")
    [EveronCaptureNative]::PostMessage($about, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null

    $expected = @("01-settings.png", "02-tray-menu.png", "03-about.png")
    $actual = @(Get-ChildItem -File $out -Filter "*.png" | Sort-Object Name | ForEach-Object Name)
    if (($actual -join "|") -ne ($expected -join "|")) {
        throw "Unexpected screenshot manifest: $($actual -join ', ')"
    }

    Get-FileHash (Join-Path $out "*.png") -Algorithm SHA256 |
        Sort-Object Path |
        ForEach-Object { "$($_.Hash.ToLowerInvariant())  $(Split-Path $_.Path -Leaf)" } |
        Set-Content (Join-Path $out "SCREENSHOTS-SHA256.txt") -Encoding ascii
} finally {
    Get-Process -Id $process.Id -ErrorAction SilentlyContinue | Stop-Process -Force
}
