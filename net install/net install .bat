@echo off
setlocal enabledelayedexpansion
title "WinGET Launcher - Net Installer"

:: ---- Проверка прав администратора ----
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [WARN] Requesting Administrator privileges...
    "%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
set "LOG_FILE=%ROOT%\install.log"

echo === WinGET Launcher Net Install started at %date% %time% === > "%LOG_FILE%"

set "GH_RAW=https://raw.githubusercontent.com/VeryHleb/WinGETLauncher/main/"
set "TMP_LIST=%TEMP%\wl_net_files.txt"

:: Путь к вашему списку файлов для установки
set "NET_FILES_PATH=source/net_install.txt"

:: Ссылка на ваш архив в формате .zip (замените на актуальную ссылку релиза GitHub)
set "ZIP_ARCHIVE=%ROOT%\source\Winget.zip"
set "ZIP_URL=https://github.com/VeryHleb/WinGETLauncher/releases/download/WinGETLauncherv1.3-update-fles/Winget.zip"

echo ===================================================
echo   WinGET Launcher - Net Installer (.ZIP Version)
echo ===================================================
echo.

:: ============================================================
:: 1. DOWNLOAD NET INSTALL FILE LIST
:: ============================================================
echo [INFO] Downloading installation file list...
echo [INFO] Downloading installation file list... >> "%LOG_FILE%"

if exist "%TMP_LIST%" del "%TMP_LIST%" >nul 2>&1

curl.exe -fsSL --retry 3 -A "WinGETLauncher-NetInstall/1.0" -o "%TMP_LIST%" "%GH_RAW%%NET_FILES_PATH%?t=%RANDOM%"
if !errorlevel! neq 0 (
    echo [ERROR] Failed to download net_install.txt. Errorlevel: !errorlevel!
    echo [ERROR] Failed to download net_install.txt >> "%LOG_FILE%"
    pause
    goto end_error
)

if not exist "%TMP_LIST%" (
    echo [ERROR] net_install.txt not found after download!
    echo [ERROR] net_install.txt not found after download! >> "%LOG_FILE%"
    pause
    goto end_error
)

set /a OK_COUNT=0
set /a FAIL_COUNT=0
set /a LINE_COUNT=0

echo [DEBUG] Parsing net_install.txt and downloading core files...
echo [DEBUG] Starting to parse net_install.txt... >> "%LOG_FILE%"

for /f "usebackq tokens=* eol=#" %%L in (`findstr /r "^[a-zA-Z0-9#]" "%TMP_LIST%"`) do (
    set /a LINE_COUNT+=1
    set "LINE=%%L"
    set "LINE=!LINE: =!"
    set "LINE=!LINE:	=!"
    
    if not "!LINE!"=="" (
        echo [DEBUG] Processing: !LINE!
        echo [DEBUG] Processing: !LINE! >> "%LOG_FILE%"
        call :ProcessLine "!LINE!"
    )
)

:: Force version.txt
call :ProcessLine "version.txt"
echo.

:: ============================================================
:: 2. DOWNLOAD & EXTRACT WINGET.ZIP (USING POWERSHELL)
:: ============================================================
echo [INFO] Checking dependencies archive (Winget.zip)...
echo [INFO] Checking Winget.zip... >> "%LOG_FILE%"

if not exist "%ROOT%\source" mkdir "%ROOT%\source"

if not exist "%ZIP_ARCHIVE%" (
    echo [INFO] Downloading Winget.zip from releases...
    echo [INFO] Downloading Winget.zip... >> "%LOG_FILE%"
    
    curl.exe -fSL -L -C - --retry 5 --retry-delay 2 -A "WinGETLauncher-NetInstaller/1.0" -o "%ZIP_ARCHIVE%" "%ZIP_URL%"
    if !errorlevel! neq 0 (
        echo [ERROR] Failed to download Winget.zip. Errorlevel: !errorlevel!
        echo [ERROR] Failed to download Winget.zip >> "%LOG_FILE%"
        goto end_error
    )
) else (
    echo [OK] Winget.zip found locally. Skipping download.
    echo [OK] Winget.zip found locally. >> "%LOG_FILE%"
)

echo [INFO] Extracting Winget.zip via PowerShell...
echo [INFO] Extracting Winget.zip... >> "%LOG_FILE%"

powershell -NoProfile -Command "Expand-Archive -LiteralPath '%ZIP_ARCHIVE%' -DestinationPath '%ROOT%\source' -Force"
if %errorlevel% equ 0 (
    echo [OK] Archive extracted successfully.
    echo [OK] Archive extracted successfully. >> "%LOG_FILE%"
    
    echo [INFO] Removing Winget.zip...
    del /f /q "%ZIP_ARCHIVE%" >nul 2>&1
) else (
    echo [WARN] PowerShell extraction returned code %errorlevel%.
    echo [WARN] PowerShell extraction failed. >> "%LOG_FILE%"
)
echo.

:: ============================================================
:: 3. VERIFY INSTALLED FILES
:: ============================================================
echo [INFO] Verifying installation...
echo [INFO] Verifying installation... >> "%LOG_FILE%"

set "VERIFY_FAIL=0"
if not exist "%ROOT%\launcher.exe" (
    echo [WARN] launcher.exe is missing!
    set "VERIFY_FAIL=1"
)
if not exist "%ROOT%\install_apps.bat" (
    echo [WARN] install_apps.bat is missing!
    set "VERIFY_FAIL=1"
)

if "%VERIFY_FAIL%"=="1" (
    echo [ERROR] Installation incomplete. Some core files are missing.
    goto end_error
) else (
    echo [OK] Installation successfully completed!
    echo [OK] Installation completed successfully. >> "%LOG_FILE%"
)
echo.

del "%TMP_LIST%" >nul 2>&1

:: ============================================================
:: 4. LAUNCHER START & SELF-DESTRUCTION
:: ============================================================
echo [INFO] Starting WinGET Launcher...
if exist "%ROOT%\launcher.exe" (
    start "" "%ROOT%\launcher.exe"
)

echo [INFO] Cleaning up installer script...
start "" /b cmd.exe /c "timeout /t 2 /nobreak >nul & del /f /q ""%~f0"" >nul 2>&1"

exit /b 0

:: ============================================================
:: ProcessLine <line>
:: ============================================================
:ProcessLine
setlocal
set "LINE=%~1"
if "%LINE%"=="" endlocal & exit /b 1
if "%LINE:~0,1%"=="#" endlocal & exit /b 1

set "REL=%~1"
set "BS=\"
set "REL_WIN=!REL:/=%BS%!"
set "DEST=%ROOT%\!REL_WIN!"
set "NEW=!DEST!.new"

set "URL=%GH_RAW%!REL!"

echo [GET] !REL! (GitHub)

for %%D in ("!DEST!") do set "DESTDIR=%%~dpD"
if not exist "!DESTDIR!" mkdir "!DESTDIR!" >nul 2>&1
if exist "!NEW!" del /f /q "!NEW!" >nul 2>&1

curl.exe -fsSL -A "WinGETLauncher-NetInstall/1.0" -o "!NEW!" "!URL!"
if !errorlevel! neq 0 (
    echo   [FAIL] Download failed
    if exist "!NEW!" del /f /q "!NEW!" >nul 2>&1
    endlocal & set /a FAIL_COUNT+=1
    exit /b 1
)

:FinishReplace
if not exist "!NEW!" (
    echo   [FAIL] File not created
    endlocal & set /a FAIL_COUNT+=1
    exit /b 1
)

for %%F in ("!NEW!") do set "NEWSIZE=%%~zF"
if "!NEWSIZE!"=="0" (
    echo   [FAIL] Empty file
    del /f /q "!NEW!" >nul 2>&1
    endlocal & set /a FAIL_COUNT+=1
    exit /b 1
)

if exist "!DEST!" del /f /q "!DEST!" >nul 2>&1
move /y "!NEW!" "!DEST!" >nul 2>&1

if not exist "!DEST!" (
    echo   [FAIL] Failed to replace
    endlocal & set /a FAIL_COUNT+=1
    exit /b 1
)

echo   [OK] Done
endlocal & set /a OK_COUNT+=1
exit /b 0

:end_error
echo.
echo [ERROR] Installation failed.
pause
exit /b 1