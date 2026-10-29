@echo off
setlocal enabledelayedexpansion
title WinGET Launcher - Update

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

set "GH_RAW=https://raw.githubusercontent.com/VeryHleb/WinGETLauncher/main/"
set "TMP_LIST=%TEMP%\wl_files.txt"
set "SELF_NAME=update.bat"
set "BS=\"

echo ===================================================
echo   WinGET Launcher - Update
echo ===================================================
echo.

:: ---- Wait for launcher.exe to close ----
echo [INFO] Waiting for launcher.exe to close...
set /a WAIT_COUNT=0
:wait_launcher
tasklist /FI "IMAGENAME eq launcher.exe" 2>nul | find /I "launcher.exe" >nul
if not errorlevel 1 (
    set /a WAIT_COUNT+=1
    if !WAIT_COUNT! gtr 10 (
        echo [WARN] launcher.exe did not close in 10 seconds. Forcing termination...
        taskkill /F /IM launcher.exe >nul 2>&1
        timeout /t 1 /nobreak >nul
        goto wait_done
    )
    timeout /t 1 /nobreak >nul
    goto wait_launcher
)
:wait_done
echo [OK] launcher.exe closed.
echo.

:: ---- Download files.txt ----
echo [INFO] Downloading file list...
if exist "%TMP_LIST%" del "%TMP_LIST%" >nul 2>&1
curl.exe -fsSL -A "WinGETLauncher-Update/1.0" -o "%TMP_LIST%" "%GH_RAW%source/files.txt"
if not exist "%TMP_LIST%" (
    echo [ERROR] Failed to download source/files.txt
    pause
    goto end_error
)

set /a OK_COUNT=0
set /a FAIL_COUNT=0
set /a SKIP_COUNT=0

:: Read file
for /f "usebackq tokens=* eol=#" %%L in ("%TMP_LIST%") do (
    set "LINE=%%L"
    :: ╨Ю╤З╨╕╤Б╤В╨║╨░ ╨┐╤А╨╛╨▒╨╡╨╗╨╛╨▓ ╨╕ ╤В╨░╨▒╨╛╨▓
    set "LINE=!LINE: =!"
    set "LINE=!LINE:	=!"
    
    :: ╨Я╤А╨╛╨▓╨╡╤А╨║╨░, ╤З╤В╨╛ ╤Б╤В╤А╨╛╨║╨░ ╨╜╨╡ ╨┐╤Г╤Б╤В╨░╤П
    if not "!LINE!"=="" (
        call :ProcessLine "!LINE!"
    )
)

:: ---- Force update version.txt ----
call :ProcessLine "version.txt"

echo.
echo ===================================================
echo   Update completed
echo ===================================================
echo   Success: %OK_COUNT%
echo   Failed:  %FAIL_COUNT%
echo ===================================================
echo.

del "%TMP_LIST%" >nul 2>&1

:: ---- Restart launcher ----
if exist "%ROOT%\launcher.exe" (
    echo [INFO] Restarting launcher...
    start "" "%ROOT%\launcher.exe"
) else (
    echo [WARN] launcher.exe not found!
)
pause
exit /b 0

:: ============================================================
:: ProcessLine <line>
:: ============================================================
:ProcessLine
setlocal
set "LINE=%~1"
if "%LINE%"=="" endlocal & exit /b 1

set "REL=!LINE!"
set "REL_WIN=!REL:/=%BS%!"
set "DEST=%ROOT%\!REL_WIN!"
set "NEW=!DEST!.new"

:: Skip update.bat itself
if /I "!REL_WIN!"=="%SELF_NAME%" (
    echo [SKIP] !REL! ^(in use^)
    endlocal & set /a SKIP_COUNT+=1
    exit /b 0
)

set "URL=%GH_RAW%!REL!"

:: ╨Я╤А╨╛╨▓╨╡╤А╨║╨░, ╤З╤В╨╛ URL ╤Б╤Д╨╛╤А╨╝╨╕╤А╨╛╨▓╨░╨╗╤Б╤П ╨║╨╛╤А╤А╨╡╨║╤В╨╜╨╛
if "!URL!"=="" (
    echo   [FAIL] URL is empty for line: !LINE!
    endlocal & set /a FAIL_COUNT+=1
    exit /b 1
)

for %%D in ("!DEST!") do set "DESTDIR=%%~dpD"
if not exist "!DESTDIR!" mkdir "!DESTDIR!" >nul 2>&1
if exist "!NEW!" del /f /q "!NEW!" >nul 2>&1

:: --- Download ---
echo [GET] !REL! (GitHub)
:: ╨Ъ╨░╨▓╤Л╤З╨║╨╕ ╨▓╨╛╨║╤А╤Г╨│ !NEW! ╨╕ !URL! ╨╛╨▒╤П╨╖╨░╤В╨╡╨╗╤М╨╜╤Л
curl.exe -fsSL -A "WinGETLauncher-Update/1.0" -o "!NEW!" "!URL!"
if !errorlevel! neq 0 (
    echo   [FAIL] Download failed for !REL!
    if exist "!NEW!" del /f /q "!NEW!" >nul 2>&1
    endlocal & set /a FAIL_COUNT+=1
    exit /b 1
)

:: --- Check and replace ---
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

if exist "!DEST!" (
    del /f /q "!DEST!" >nul 2>&1
)

move /y "!NEW!" "!DEST!" >nul 2>&1
if not exist "!DEST!" (
    echo   [FAIL] Failed to replace file
    endlocal & set /a FAIL_COUNT+=1
    exit /b 1
)

echo   [OK]  Done
endlocal & set /a OK_COUNT+=1
exit /b 0

:end_error
echo.
echo [ERROR] Update not completed.
pause
exit /b 1