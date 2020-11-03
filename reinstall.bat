@echo off
setlocal enabledelayedexpansion
title "WinGET Launcher - Full Reinstall & Repair"

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
set "LOG_FILE=%ROOT%\reinstall.log"

echo === WinGET Launcher Reinstall started at %date% %time% === > "%LOG_FILE%"

set "GH_RAW=https://raw.githubusercontent.com/VeryHleb/WinGETLauncher/main/"
set "TMP_LIST=%TEMP%\wl_files.txt"
set "WINGET_ARCHIVE=%ROOT%\source\Winget.7z"
set "WINGET_URL=https://github.com/VeryHleb/WinGETLauncher/releases/download/WinGETLauncherv1.3-update-fles/Winget.7z"
set "SEVEN_ZIP=%ROOT%\source\7-Zip\7z.exe"

echo ===================================================
echo   WinGET Launcher - Full Reinstall & Repair
echo ===================================================
echo.

:: Wait for launcher.exe to close
echo [INFO] Waiting for launcher.exe...
echo [INFO] Waiting for launcher.exe... >> "%LOG_FILE%"
:wait_launcher
tasklist /FI "IMAGENAME eq launcher.exe" 2>nul | find /I "launcher.exe" >nul
if not errorlevel 1 (
    timeout /t 1 /nobreak >nul
    goto wait_launcher
)
timeout /t 2 /nobreak >nul
echo [OK] launcher.exe closed.
echo [OK] launcher.exe closed. >> "%LOG_FILE%"
echo.

:: ============================================================
:: 0. CLEANUP OLD FILES (EXCEPT repositories.txt and source/)
:: ============================================================
echo [INFO] Cleaning up old files (keeping repositories.txt)...
echo [INFO] Cleaning up old files... >> "%LOG_FILE%"

for %%F in ("%ROOT%\*.*") do (
    set "FILENAME=%%~nxF"
    :: Не удаляем repositories.txt, сам скрипт переустановки и логи
    if /I not "!FILENAME!"=="repositories.txt" (
        if /I not "!FILENAME!"=="reinstall.bat" (
            if /I not "!FILENAME!"=="reinstall.log" (
                del /f /q "%%F" >nul 2>&1
                echo   [-] Removed file: !FILENAME!
                echo   [-] Removed file: !FILENAME! >> "%LOG_FILE%"
            )
        )
    )
)
echo [OK] Cleanup finished.
echo.

:: ============================================================
:: 1. DOWNLOAD FILES.TXT AND RE-DOWNLOAD ALL CORE FILES
:: ============================================================
echo [INFO] Downloading file list for reinstallation...
echo [INFO] Downloading file list... >> "%LOG_FILE%"

if exist "%TMP_LIST%" del "%TMP_LIST%" >nul 2>&1

curl.exe -fsSL --retry 3 -A "WinGETLauncher-Reinstall/1.0" -o "%TMP_LIST%" "%GH_RAW%source/files.txt?t=%RANDOM%"
if !errorlevel! neq 0 (
    echo [ERROR] curl failed to download files.txt. Errorlevel: !errorlevel!
    echo [ERROR] curl failed to download files.txt. Errorlevel: !errorlevel! >> "%LOG_FILE%"
    pause
    goto end_error
)

if not exist "%TMP_LIST%" (
    echo [ERROR] files.txt not found after download!
    echo [ERROR] files.txt not found after download! >> "%LOG_FILE%"
    pause
    goto end_error
)

set /a OK_COUNT=0
set /a FAIL_COUNT=0
set /a SKIP_COUNT=0
set /a LINE_COUNT=0

echo [DEBUG] Starting to re-download files from list...
echo [DEBUG] Starting to re-download files from list... >> "%LOG_FILE%"

for /f "usebackq tokens=* eol=#" %%L in (`findstr /r "^[a-zA-Z0-9#]" "%TMP_LIST%"`) do (
    set /a LINE_COUNT+=1
    set "LINE=%%L"
    set "LINE=!LINE: =!"
    set "LINE=!LINE:	=!"
    
    if not "!LINE!"=="" (
        echo [DEBUG] Processing line !LINE_COUNT!: !LINE!
        echo [DEBUG] Processing line !LINE_COUNT!: !LINE! >> "%LOG_FILE%"
        call :ProcessLine "!LINE!"
    )
)

:: Force version.txt
call :ProcessLine "version.txt"

:: ============================================================
:: 2. DEPENDENCIES CHECK & WINGET.7Z DOWNLOAD/EXTRACTION
:: ============================================================
echo.
echo [INFO] Checking required winget dependencies...
echo [INFO] Checking required winget dependencies... >> "%LOG_FILE%"

if not exist "%ROOT%\source" mkdir "%ROOT%\source"

set "MISSING_DEPS=0"
if not exist "%ROOT%\source\install_winget.bat" set "MISSING_DEPS=1"
if not exist "%ROOT%\source\Microsoft.DesktopAppInstaller_*.msixbundle" set "MISSING_DEPS=1"
if not exist "%ROOT%\source\Microsoft.VCLibs.140.00.*.Appx" set "MISSING_DEPS=1"

if "%MISSING_DEPS%"=="1" (
    echo [INFO] Some dependencies are missing. Downloading Winget.7z...
    echo [INFO] Some dependencies are missing. Downloading Winget.7z... >> "%LOG_FILE%"
    
    if not exist "%WINGET_ARCHIVE%" (
        curl.exe -fSL -L -C - --retry 5 --retry-delay 2 -A "WinGETLauncher-Reinstall/1.0" -o "%WINGET_ARCHIVE%" "%WINGET_URL%"
        if !errorlevel! neq 0 (
            echo [ERROR] Failed to download Winget.7z
            echo [ERROR] Failed to download Winget.7z >> "%LOG_FILE%"
            goto end_error
        )
    )

    echo [INFO] Extracting Winget.7z...
    echo [INFO] Extracting Winget.7z... >> "%LOG_FILE%"

    if exist "%SEVEN_ZIP%" (
        "%SEVEN_ZIP%" x "%WINGET_ARCHIVE%" -o"%ROOT%\source" -y >nul 2>&1
        if !errorlevel! equ 0 (
            echo [OK] Archive extracted successfully.
            echo [OK] Archive extracted successfully. >> "%LOG_FILE%"
            
            echo [INFO] Removing Winget.7z...
            del /f /q "%WINGET_ARCHIVE%" >nul 2>&1
        ) else (
            echo [WARN] Extraction failed with errorlevel !errorlevel!.
            echo [WARN] Extraction failed with errorlevel !errorlevel!. >> "%LOG_FILE%"
        )
    ) else (
        echo [ERROR] 7z.exe not found at %SEVEN_ZIP%! Cannot extract.
        echo [ERROR] 7z.exe not found at %SEVEN_ZIP%! >> "%LOG_FILE%"
    )
) else (
    echo [OK] All dependencies are already present. Skipping download.
    echo [OK] All dependencies are already present. >> "%LOG_FILE%"
)
echo.

:: ============================================================
:: 3. VERIFY FILES
:: ============================================================
echo [INFO] Verifying reinstalled files...
echo [INFO] Verifying reinstalled files... >> "%LOG_FILE%"

set "VERIFY_FAIL=0"
if not exist "%ROOT%\launcher.exe" (
    echo [WARN] launcher.exe is missing!
    echo [WARN] launcher.exe is missing! >> "%LOG_FILE%"
    set "VERIFY_FAIL=1"
)
if not exist "%ROOT%\install_apps.bat" (
    echo [WARN] install_apps.bat is missing!
    echo [WARN] install_apps.bat is missing! >> "%LOG_FILE%"
    set "VERIFY_FAIL=1"
)
if not exist "%ROOT%\source\repositories.txt" (
    echo [WARN] repositories.txt is missing!
    echo [WARN] repositories.txt is missing! >> "%LOG_FILE%"
    set "VERIFY_FAIL=1"
)

if "%VERIFY_FAIL%"=="1" (
    echo [WARN] Some core files are missing after reinstallation.
    echo [WARN] Some core files are missing. >> "%LOG_FILE%"
) else (
    echo [OK] All core files are successfully restored.
    echo [OK] All core files restored. >> "%LOG_FILE%"
)
echo.

:: ============================================================
:: SUMMARY
:: ============================================================
echo ===================================================
echo   Reinstallation completed
echo ===================================================
echo   Success: %OK_COUNT%
echo   Failed:  %FAIL_COUNT%
echo ===================================================
echo.

echo. >> "%LOG_FILE%"
echo =================================================== >> "%LOG_FILE%"
echo   Reinstallation completed >> "%LOG_FILE%"
echo   Success: %OK_COUNT% >> "%LOG_FILE%"
echo   Failed:  %FAIL_COUNT% >> "%LOG_FILE%"
echo   Lines parsed: %LINE_COUNT% >> "%LOG_FILE%"
echo =================================================== >> "%LOG_FILE%"

del "%TMP_LIST%" >nul 2>&1

:: ============================================================
:: 4. RESTART LAUNCHER
:: ============================================================
if exist "%ROOT%\launcher.exe" (
    echo [INFO] Restarting launcher...
    start "" "%ROOT%\launcher.exe"
) else (
    echo [WARN] launcher.exe not found!
)

timeout /t 3 /nobreak >nul
exit /b 0

:: ============================================================
:: ProcessLine <line>
:: ============================================================
:ProcessLine
setlocal
set "LINE=%~1"
if "%LINE%"=="" endlocal & exit /b 1
if "%LINE:~0,1%"=="#" endlocal & exit /b 1

set "IS_URL=0"
echo !LINE! | findstr /i "^http" >nul
if !errorlevel! equ 0 set "IS_URL=1"

if "!IS_URL!"=="1" goto :DoUrl
goto :DoGitHub

:DoUrl
set "URL=!LINE!"
for %%F in ("!URL!") do set "FILENAME=%%~nxF"
set "DEST=%ROOT%\source\!FILENAME!"
set "NEW=%DEST%.new"

echo [GET] !FILENAME! (Release)
echo [GET] !FILENAME! (Release) >> "%LOG_FILE%"

if "!URL!"=="" (
    echo   [FAIL] URL is empty
    endlocal & set /a FAIL_COUNT+=1
    exit /b 1
)

if exist "!NEW!" del /f /q "!NEW!" >nul 2>&1

curl.exe -fSL -L -C - --retry 5 --retry-delay 2 -A "WinGETLauncher-Reinstall/1.0" -o "!NEW!" "!URL!"
if !errorlevel! neq 0 (
    echo   [FAIL] Download failed. Errorlevel: !errorlevel!
    echo   [FAIL] Download failed. Errorlevel: !errorlevel! >> "%LOG_FILE%"
    if exist "!NEW!" del /f /q "!NEW!" >nul 2>&1
    endlocal & set /a FAIL_COUNT+=1
    exit /b 1
)
goto :FinishReplace

:DoGitHub
set "REL=!LINE!"
set "BS=\"
set "REL_WIN=!REL:/=%BS%!"
set "DEST=%ROOT%\!REL_WIN!"
set "NEW=!DEST!.new"

for %%A in ("!REL_WIN!") do set "JUST_NAME=%%~nxA"

if /I "!JUST_NAME!"=="reinstall.bat" (
    endlocal & exit /b 0
)

set "URL=%GH_RAW%!REL!"

echo [GET] !REL! (GitHub)
echo [GET] !REL! (GitHub) >> "%LOG_FILE%"

if "!URL!"=="" (
    echo   [FAIL] URL is empty
    endlocal & set /a FAIL_COUNT+=1
    exit /b 1
)

for %%D in ("!DEST!") do set "DESTDIR=%%~dpD"
if not exist "!DESTDIR!" mkdir "!DESTDIR!" >nul 2>&1
if exist "!NEW!" del /f /q "!NEW!" >nul 2>&1

curl.exe -fsSL -A "WinGETLauncher-Reinstall/1.0" -o "!NEW!" "!URL!"

if !errorlevel! neq 0 (
    echo   [FAIL] Download failed
    echo   [FAIL] Download failed >> "%LOG_FILE%"
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

echo   [OK]  Done
endlocal & set /a OK_COUNT+=1
exit /b 0

:end_error
echo.
echo [ERROR] Reinstallation not completed.
pause
exit /b 1