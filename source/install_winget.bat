@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo ========================================================
echo           WinGet Offline Installer Utility
echo ========================================================
echo.

echo [INFO] Checking for WinGet...
winget --version >nul 2>&1
if !errorlevel! equ 0 (
    echo [OK] WinGet is already installed and working.
    goto end
)

echo [INFO] Installing dependencies and WinGet from current folder...
echo.

echo [1/5] VCLibs 140.00 (MSIX)...
for %%f in (Microsoft.VCLibs*.appx) do (
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Add-AppxPackage -Path '%%f' -ErrorAction SilentlyContinue"
)

echo [2/5] VCLibs 140.00 UWPDesktop (MSIX)...
for %%f in (Microsoft.VCLibs*.UWPDesktop*.appx) do (
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Add-AppxPackage -Path '%%f' -ErrorAction SilentlyContinue"
)

echo [3/5] UI.Xaml 2.8 (APPX)...
for %%f in (Microsoft.UI.Xaml*.appx) do (
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Add-AppxPackage -Path '%%f' -ErrorAction SilentlyContinue"
)

echo [4/5] Windows App Runtime 1.8 (MSIX)...
for %%f in (Microsoft.WindowsAppRuntime*.appx) do (
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Add-AppxPackage -Path '%%f' -ErrorAction SilentlyContinue"
)

echo [5/5] Desktop App Installer (MSIXBUNDLE)...
for %%f in (Microsoft.DesktopAppInstaller_2026.917.151.0_neutral___8wekyb3d8bbwe.msixbundle) do (
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Add-AppxPackage -Path '%%f' -ForceApplicationShutdown"
)

:end
echo.
winget --version >nul 2>&1
if !errorlevel! equ 0 (
    echo [SUCCESS] WinGet successfully installed and ready!
) else (
    echo [ERROR] Installation check failed.
)
echo.
pause
exit /b 0
