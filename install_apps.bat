@echo off
chcp 65001 >nul
title Installing apps via winget
setlocal enabledelayedexpansion

set "ROOT=%~dp0"
set "SOURCE_DIR=%ROOT%source"
set "WINGET_INSTALLER=%SOURCE_DIR%\install_winget.bat"
set "EXE_DIR=%SOURCE_DIR%\exe"
set "LOG_DIR=%ROOT%logs"

if not defined REPO_FILE set "REPO_FILE=%SOURCE_DIR%\repositories.txt"
if not defined PKG_MGR set "PKG_MGR=winget"

set "TG_DIR=%ProgramFiles%\TgWsProxy"
set "TG_EXE=%TG_DIR%\TgWsProxy_windows_7_64bit.exe"
set "TG_SRC=%EXE_DIR%\TgWsProxy_windows_7_64bit.exe"
set "TG_TASK=TgWsProxy"

if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"

:: Auto-cleanup logs older than 30 days (BEFORE creating new one)
forfiles /p "%LOG_DIR%" /m install_*.log /d -30 /c "cmd /c del @path" >nul 2>&1

:: Build log file name
for /f "tokens=1-4 delims=/:. " %%a in ("%date% %time%") do set "STAMP=%%d%%b%%c_%%a"
if "%STAMP%"=="" set "STAMP=%RANDOM%"
set "LOG_FILE=%LOG_DIR%\install_%STAMP%.log"

:: Create log file immediately
echo. > "%LOG_FILE%"

echo [INFO] Log file: %LOG_FILE%
echo [INFO] Repository: %REPO_FILE%
echo [INFO] Package manager: %PKG_MGR%
echo ============================================================ >> "%LOG_FILE%"
echo   install session started at %date% %time%                 >> "%LOG_FILE%"
echo   Repository: %REPO_FILE%                                 >> "%LOG_FILE%"
echo   Manager: %PKG_MGR%                                       >> "%LOG_FILE%"
echo ============================================================ >> "%LOG_FILE%"

net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [WARN] Not running as Administrator.
    echo [WARN] Some installations may fail.
    echo.
    echo [WARN] Not running as Administrator >> "%LOG_FILE%"
)

if not exist "%REPO_FILE%" (
    echo [ERROR] Repository file not found: %REPO_FILE%
    echo [ERROR] Repository file not found >> "%LOG_FILE%"
    echo [BAT_DONE] rc=2
    if not defined REPO_FILE pause
    exit /b 2
)

if "%PKG_MGR%"=="winget" (
    where winget >nul 2>&1
    if !errorLevel! neq 0 (
        echo [WARN] winget not found. Falling back to Chocolatey.
        set "PKG_MGR=choco"
    )
)

if "%PKG_MGR%"=="choco" (
    where choco >nul 2>&1
    if !errorLevel! neq 0 (
        echo [INFO] Installing Chocolatey...
        powershell -NoProfile -ExecutionPolicy Bypass -Command ^
          "[System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))" >> "%LOG_FILE%" 2>&1
        set "PATH=%PATH%;%ALLUSERSPROFILE%\chocolatey\bin"
        where choco >nul 2>&1
        if !errorLevel! neq 0 (
            echo [ERROR] Failed to install Chocolatey.
            echo [BAT_DONE] rc=2
            if not defined REPO_FILE pause
            exit /b 2
        )
    )
)

set /a CNT_OK=0
set /a CNT_SKIP=0
set /a CNT_FAIL=0
set /a CNT_TOTAL=0
set "FAILED_LIST="
set "TG_FOUND=0"

echo ===================================================
echo   Installing apps via %PKG_MGR% (from repository)
echo ===================================================
echo.

:: Format: winget.id|choco.id|Name
for /f "usebackq tokens=* eol=#" %%L in ("%REPO_FILE%") do (
    set "RAW_LINE=%%L"

    if defined RAW_LINE (
        for /f "tokens=1 delims=﻿" %%B in ("!RAW_LINE!") do set "RAW_LINE=%%B"
    )

    if not "!RAW_LINE!"=="" (
        set "APP_ID="
        set "APP_CHOCO="
        set "APP_NAME="

        echo !RAW_LINE! | findstr /c:"|" >nul
        if !errorLevel! equ 0 (
            for /f "tokens=1,2,3 delims=|" %%A in ("!RAW_LINE!") do (
                set "APP_ID=%%A"
                set "APP_CHOCO=%%B"
                set "APP_NAME=%%C"
            )
        ) else (
            set "APP_ID=!RAW_LINE!"
        )

        for /f "tokens=* delims= " %%T in ("!APP_ID!")    do set "APP_ID=%%T"
        for /f "tokens=* delims= " %%T in ("!APP_CHOCO!") do set "APP_CHOCO=%%T"
        for /f "tokens=* delims= " %%T in ("!APP_NAME!")  do set "APP_NAME=%%T"

        if not "!APP_ID!"=="" (
            set /a CNT_TOTAL+=1
            echo ---------------------------------------------------
            echo [INSTALL] !APP_ID!
            echo ---------------------------------------------------
            echo [INSTALL] !APP_ID! >> "%LOG_FILE%"

            echo !APP_ID! | findstr /i "telegram" >nul
            if !errorLevel! equ 0 set "TG_FOUND=1"

            call :ProcessOne "!APP_ID!" "!APP_CHOCO!"
        )
    )
)
goto :after_loop

:ProcessOne
    setlocal
    set "PKG=%~1"
    set "CHOCO_ID=%~2"
    set "MGR=%PKG_MGR%"

    if "!MGR!"=="choco" (
        if "!CHOCO_ID!"=="" (
            echo [SKIP] !PKG! ^(no choco.id^).
            echo [SKIP] !PKG! ^(no choco.id^) >> "%LOG_FILE%"
            endlocal & set /a CNT_SKIP+=1
            exit /b 0
        )
        goto :use_choco
    )

    goto :use_winget

:use_winget
    :: ---- Проверка: уже установлено? ----
    set "FOUND=0"
    for /f "delims=" %%O in ('winget list --id "!PKG!" 2^>nul') do (
        echo %%O | findstr /i /c:"!PKG!" >nul 2>&1
        if not errorlevel 1 set "FOUND=1"
    )
    if "!FOUND!"=="1" (
        echo [SKIP] !PKG! already installed.
        echo [SKIP] !PKG! >> "%LOG_FILE%"
        endlocal & set /a CNT_SKIP+=1
        exit /b 0
    )

    :: ---- 1) Попытка через winget ----
    set "ATTEMPT=0"
    :retry_w
        winget install --id "!PKG!" --source winget --silent --accept-package-agreements --accept-source-agreements --disable-interactivity >> "%LOG_FILE%" 2>&1
        set "WINGET_RC=!errorLevel!"
        if !WINGET_RC! equ 0 goto :install_ok
        set /a ATTEMPT+=1
        if !ATTEMPT! lss 2 (
            timeout /t 2 /nobreak >nul
            goto :retry_w
        )
        goto :try_msstore

    :: ---- 2) Fallback: msstore ----
    :try_msstore
        echo [INFO] winget failed for !PKG!. Trying msstore...
        echo [INFO] winget failed, trying msstore >> "%LOG_FILE%"

        winget list --id "!PKG!" --source msstore >nul 2>&1
        if not errorlevel 1 (
            echo [SKIP] !PKG! already installed ^(msstore^).
            echo [SKIP] !PKG! >> "%LOG_FILE%"
            endlocal & set /a CNT_SKIP+=1
            exit /b 0
        )

        set "ATTEMPT=0"
        :retry_ms
            winget install --id "!PKG!" --source msstore --silent --accept-package-agreements --accept-source-agreements --disable-interactivity >> "%LOG_FILE%" 2>&1
            set "WINGET_RC=!errorLevel!"
            if !WINGET_RC! equ 0 goto :install_ok
            set /a ATTEMPT+=1
            if !ATTEMPT! lss 2 (
                timeout /t 2 /nobreak >nul
                goto :retry_ms
            )
            goto :install_fail

:use_choco
    set "FOUND=0"
    for /f "delims=" %%O in ('choco list --local-only "!CHOCO_ID!" 2^>nul') do (
        echo %%O | findstr /i /c:"!CHOCO_ID!" >nul 2>&1
        if not errorlevel 1 set "FOUND=1"
    )
    if "!FOUND!"=="1" (
        echo [SKIP] !CHOCO_ID! already installed ^(choco^).
        echo [SKIP] !CHOCO_ID! ^(choco^) >> "%LOG_FILE%"
        endlocal & set /a CNT_SKIP+=1
        exit /b 0
    )
    set "ATTEMPT=0"
    :retry_c
        choco install "!CHOCO_ID!" -y --no-progress >> "%LOG_FILE%" 2>&1
        set "CHOCO_RC=!errorLevel!"
        if !CHOCO_RC! equ 0 goto :install_ok
        set /a ATTEMPT+=1
        if !ATTEMPT! lss 3 (
            timeout /t 3 /nobreak >nul
            goto :retry_c
        )
        goto :install_fail

:install_ok
    echo [OK] !PKG! installed.
    echo [OK] !PKG! >> "%LOG_FILE%"
    endlocal & set /a CNT_OK+=1
    exit /b 0

:install_fail
    echo [WARN] Failed to install !PKG!.
    echo [FAIL] !PKG! >> "%LOG_FILE%"
    endlocal & set /a CNT_FAIL+=1 & set "FAILED_LIST=%FAILED_LIST% !PKG!"
    exit /b 1

:after_loop

echo ===================================================
echo   SUMMARY
echo ===================================================
echo   Manager   : %PKG_MGR%
echo   Total     : %CNT_TOTAL%
echo   Installed : %CNT_OK%
echo   Skipped   : %CNT_SKIP%
echo   Failed    : %CNT_FAIL%
if not "%FAILED_LIST%"=="" echo   Failed list:%FAILED_LIST%
echo   Log file  : %LOG_FILE%
echo ===================================================
echo.

echo =================================================== >> "%LOG_FILE%"
echo   SUMMARY >> "%LOG_FILE%"
echo   Manager   : %PKG_MGR% >> "%LOG_FILE%"
echo   Total     : %CNT_TOTAL% >> "%LOG_FILE%"
echo   Installed : %CNT_OK% >> "%LOG_FILE%"
echo   Skipped   : %CNT_SKIP% >> "%LOG_FILE%"
echo   Failed    : %CNT_FAIL% >> "%LOG_FILE%"
if not "%FAILED_LIST%"=="" echo   Failed list:%FAILED_LIST% >> "%LOG_FILE%"
echo =================================================== >> "%LOG_FILE%"
echo   Session ended at %date% %time% >> "%LOG_FILE%"

if "%TG_FOUND%"=="0" goto :tg_done

echo.
echo ===================================================
echo   TgWsProxy setup
echo ===================================================

if not exist "%TG_SRC%" (
    echo [WARN] Source file not found: %TG_SRC%
    echo [WARN] Source file not found >> "%LOG_FILE%"
    goto :tg_done
)

if not exist "%TG_DIR%" mkdir "%TG_DIR%"

taskkill /f /im TgWsProxy_windows_7_64bit.exe >nul 2>&1
timeout /t 1 /nobreak >nul

copy /y "%TG_SRC%" "%TG_EXE%" >> "%LOG_FILE%" 2>&1
if errorlevel 1 (
    echo [ERROR] Copy failed: %TG_EXE%
    goto :tg_done
)

schtasks /query /tn "%TG_TASK%" >nul 2>&1
if !errorLevel! equ 0 schtasks /delete /tn "%TG_TASK%" /f >nul 2>&1

schtasks /create /tn "%TG_TASK%" /tr "\"%TG_EXE%\"" /sc onlogon /rl highest /f >> "%LOG_FILE%" 2>&1
if !errorLevel! equ 0 echo [OK] Autostart task created.

start "" /b "%TG_EXE%"
echo [OK] TgWsProxy launched.

:tg_done
echo.

echo ===================================================
echo   All done.
echo ===================================================
echo   Log file : %LOG_FILE%
echo ===================================================
echo.

set "FINAL_RC=0"
if %CNT_FAIL% gtr 0 set "FINAL_RC=1"

echo [BAT_DONE] rc=%FINAL_RC%
echo [BAT_DONE] rc=%FINAL_RC% >> "%LOG_FILE%"

if not defined REPO_FILE pause
exit /b %FINAL_RC%