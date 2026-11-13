@echo off
setlocal enabledelayedexpansion

:: Запрос прав администратора, если запуск не от админа
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [WARN] Requesting Administrator privileges...
    "%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
set "LOG_FILE=%ROOT%\install.log"

echo === WinGET Launcher Net Install started at %date% %time% === > "%LOG_FILE%"

set "ZIP_ARCHIVE=%ROOT%\source\WinGETLauncher.zip"
set "ZIP_URL=https://github.com/VeryHleb/WinGETLauncher/releases/latest/download/WinGETLauncher.zip"

echo ========================================================
echo       WinGET Launcher Net Installer (.ZIP Version)
echo ========================================================
echo.

:: ========================================================
:: 1. DOWNLOAD WinGETLauncher.ZIP (Latest Release)
:: ========================================================
if not exist "%ROOT%\source" mkdir "%ROOT%\source"

echo [INFO] Downloading WinGETLauncher.zip from latest release...
curl -L -# -o "%ZIP_ARCHIVE%" "%ZIP_URL%"

if %errorlevel% neq 0 (
    echo [ERROR] Failed to download WinGETLauncher.zip
    echo [ERROR] Failed to download WinGETLauncher.zip >> "%LOG_FILE%"
    pause
    exit /b 1
)

echo [OK] WinGETLauncher.zip downloaded successfully.
echo.

:: ========================================================
:: 2. EXTRACTION
:: ========================================================
echo [INFO] Extracting WinGETLauncher.zip via PowerShell...
powershell -Command "Expand-Archive -Path '%ZIP_ARCHIVE%' -DestinationPath '%ROOT%' -Force"

if %errorlevel% neq 0 (
    echo [ERROR] Extraction failed.
    echo [ERROR] Extraction failed. >> "%LOG_FILE%"
    pause
    exit /b 1
)

echo [OK] Archive extracted successfully.
echo.

echo [SUCCESS] Net installation completed!
pause
