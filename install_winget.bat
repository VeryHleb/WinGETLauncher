@echo off
setlocal enabledelayedexpansion
title Установка App Installer (winget) и зависимостей

:: Проверка прав администратора
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [ОШИБКА] Запустите этот скрипт от имени Администратора!
    pause
    exit /b
)

cd /d "%~dp0"
echo ===================================================
echo   Установка App Installer (winget)
echo ===================================================
echo.

:: Функция установки одного пакета APPX/MSIX/MSIXBUNDLE
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
echo   Проверка установки winget
echo ===================================================
where winget >nul 2>&1
if %errorLevel% equ 0 (
    echo [OK] winget успешно установлен!
    winget --version
) else (
    echo [ВНИМАНИЕ] winget не найден в PATH. 
    echo Попробуйте перезайти в систему или запустить:
    echo   winget --version
)
echo.
pause