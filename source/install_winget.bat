@echo off
setlocal enabledelayedexpansion
title LTÑ¦ÄTÀTÍTÎTÂTÊTÀ App Installer (winget) TÈ TÇTÀTÂTÈTÑTÈTÌTÎTÑ¦ÄTÅTÉ

:: Tæ¦ÁTÎTÂTÅ¦ÁTÊTÀ TÏ¦ÁTÀTÂ TÀTÄTÌTÈTÍTÈTÑ¦Ä¦ÁTÀ¦ÄTÎ¦ÁTÀ
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [++L+TæL] TæTÀTÏTÔTÑ¦ÄTÈ¦ÄTÅ Tä¦ÄTÎ¦Ä TÑTÊ¦ÁTÈTÏ¦Ä TÎ¦Ä TÈTÌTÅTÍTÈ LTÄTÌTÈTÍTÈTÑ¦Ä¦ÁTÀ¦ÄTÎ¦ÁTÀ!
    pause
    exit /b
)

cd /d "%~dp0"
echo ===================================================
echo   LTÑ¦ÄTÀTÍTÎTÂTÊTÀ App Installer (winget)
echo ===================================================
echo.

:: LTÔTÍTÊ¦ÎTÈTà TÔTÑ¦ÄTÀTÍTÎTÂTÊTÈ TÎTÄTÍTÎTÃTÎ TÏTÀTÊTÅ¦ÄTÀ APPX/MSIX/MSIXBUNDLE
set "PS_CMD=Add-AppxPackage -Path"

echo [1/5] VCLibs 140.00 (MSIX)...
for %%f in (Microsoft.VCLibs.140.00_*.msix Microsoft.VCLibs.140.00_*.appx) do (
    if exist "%%f" powershell -Command "Add-AppxPackage -Path '%%~ff'"
)

echo [2/5] VCLibs 140.00 UWPDesktop (MSIX)...
for %%f in (Microsoft.VCLibs.140.00.UWPDesktop_*.msix Microsoft.VCLibs.140.00.UWPDesktop_*.appx) do (
    if exist "%%f" powershell -Command "Add-AppxPackage -Path '%%~ff'"
)

echo [3/5] UI.Xaml 2.8 (APPX)...
for %%f in (Microsoft.UI.Xaml.2.8_*.appx Microsoft.UI.Xaml.2.8_*.msix) do (
    if exist "%%f" powershell -Command "Add-AppxPackage -Path '%%~ff'"
)

echo [4/5] Windows App Runtime 1.8 (MSIX)...
for %%f in (Microsoft.WindowsAppRuntime.1.8_*.msix Microsoft.WindowsAppRuntime.1.8_*.appx) do (
    if exist "%%f" powershell -Command "Add-AppxPackage -Path '%%~ff'"
)

echo [5/5] Desktop App Installer (MSIXBUNDLE)...
for %%f in (Microsoft.DesktopAppInstaller_*.msixbundle Microsoft.DesktopAppInstaller_*.appxbundle) do (
    if exist "%%f" powershell -Command "Add-AppxPackage -Path '%%~ff'"
)

echo.
echo ===================================================
echo   Tæ¦ÁTÎTÂTÅ¦ÁTÊTÀ TÔTÑ¦ÄTÀTÍTÎTÂTÊTÈ winget
echo ===================================================
where winget >nul 2>&1
if %errorLevel% equ 0 (
    echo [OK] winget TÔTÑTÏTÅT-TÍTÎ TÔTÑ¦ÄTÀTÍTÎTÂTËTÅTÍ!
    winget --version
) else (
    echo [T=LTæL=L+] winget TÍTÅ TÍTÀTÉTÄTÅTÍ TÂ PATH. 
    echo TæTÎTÏ¦ÁTÎTÁTÔTÉ¦ÄTÅ TÏTÅ¦ÁTÅTÇTÀTÉ¦ÄTÈ TÂ TÑTÈTÑ¦ÄTÅTÌTÔ TÈTËTÈ TÇTÀTÏTÔTÑ¦ÄTÈ¦ÄòÄÖ:
    echo   winget --version
)
echo.
pause