@echo off
chcp 65001 >nul
title Installing apps via winget
setlocal enabledelayedexpansion

:: ============================================================
:: Paths
:: ============================================================
set "ROOT=%~dp0"
set "SOURCE_DIR=%ROOT%source"
set "WINGET_INSTALLER=%SOURCE_DIR%\install_winget.bat"
set "EXE_DIR=%SOURCE_DIR%\exe"
set "LOG_DIR=%ROOT%logs"

if not defined REPO_FILE set "REPO_FILE=%SOURCE_DIR%\repositories.txt"

set "TG_DIR=%ProgramFiles%\TgWsProxy"
set "TG_EXE=%TG_DIR%\TgWsProxy_windows_7_64bit.exe"
set "TG_SRC=%EXE_DIR%\TgWsProxy_windows_7_64bit.exe"
set "TG_TASK=TgWsProxy"

:: ============================================================
:: Logging setup
:: ============================================================
if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"
forfiles /p "%LOG_DIR%" /m install_*.log /d -30 /c "cmd /c del @path" >nul 2>&1

for /f "tokens=1-4 delims=/:. " %%a in ("%date% %time%") do set "STAMP=%%d%%b%%c_%%a"
if "%STAMP%"=="" set "STAMP=%RANDOM%"
set "LOG_FILE=%LOG_DIR%\install_%STAMP%.log"

echo [INFO] Log file: %LOG_FILE%
echo [INFO] Repository: %REPO_FILE%
echo ============================================================ >> "%LOG_FILE%"
echo   winget install session started at %date% %time%         >> "%LOG_FILE%"
echo   Repository: %REPO_FILE%                                 >> "%LOG_FILE%"
echo ============================================================ >> "%LOG_FILE%"

:: ============================================================
:: Check Administrator rights (NO auto-elevate)
:: ============================================================
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [WARN] Not running as Administrator.
    echo [WARN] Some installations may fail.
    echo.
    echo [WARN] Not running as Administrator >> "%LOG_FILE%"
)

:: ============================================================
:: Check repository file
:: ============================================================
if not exist "%REPO_FILE%" (
    echo [ERROR] Repository file not found: %REPO_FILE%
    echo [ERROR] Repository file not found: %REPO_FILE% >> "%LOG_FILE%"
    echo [BAT_DONE] rc=2
    if not defined REPO_FILE pause
    exit /b 2
)

:: ============================================================
:: Check winget availability
:: ============================================================
where winget >nul 2>&1
if %errorLevel% neq 0 (
    echo [WARN] winget not found on this system.
    echo [WARN] winget not found >> "%LOG_FILE%"

    if not exist "%SOURCE_DIR%" (
        echo [ERROR] Source folder not found: %SOURCE_DIR%
        echo [ERROR] Source folder not found >> "%LOG_FILE%"
        echo [BAT_DONE] rc=2
        if not defined REPO_FILE pause
        exit /b 2
    )

    if exist "%WINGET_INSTALLER%" (
        echo [INFO] Running winget bootstrap: %WINGET_INSTALLER%
        echo [INFO] Running bootstrap >> "%LOG_FILE%"
        call "%WINGET_INSTALLER%"

        set "PATH=%PATH%;%LOCALAPPDATA%\Microsoft\WindowsApps;%ProgramFiles%\WindowsApps"

        where winget >nul 2>&1
        if !errorLevel! neq 0 (
            echo [WARN] winget still not available after bootstrap.
            echo [WARN] winget not available after bootstrap >> "%LOG_FILE%"
            echo [BAT_DONE] rc=3
            if not defined REPO_FILE pause
            exit /b 3
        )
    ) else (
        echo [ERROR] install_winget.bat not found in: %SOURCE_DIR%
        echo [ERROR] install_winget.bat not found >> "%LOG_FILE%"
        echo [BAT_DONE] rc=2
        if not defined REPO_FILE pause
        exit /b 2
    )
)

:: ============================================================
:: Counters
:: ============================================================
set /a CNT_OK=0
set /a CNT_SKIP=0
set /a CNT_FAIL=0
set /a CNT_TOTAL=0
set "FAILED_LIST="
set "TG_FOUND=0"

echo ===================================================
echo   Installing apps via winget (from repository)
echo ===================================================
echo.
echo [INFO] winget version: >> "%LOG_FILE%"
winget --version >> "%LOG_FILE%" 2>&1

:: ============================================================
:: Read repository and install apps
:: ============================================================
for /f "usebackq tokens=* eol=#" %%L in ("%REPO_FILE%") do (
    set "RAW_LINE=%%L"

    :: ---- Strip UTF-8 BOM if present ----
    if defined RAW_LINE (
        for /f "tokens=1 delims=﻿" %%B in ("!RAW_LINE!") do set "RAW_LINE=%%B"
    )

    if not "!RAW_LINE!"=="" (
        set "APP_ID="

        :: 1) Разделитель | (формат winget.id|Имя)
        echo !RAW_LINE! | findstr /c:"|" >nul
        if !errorLevel! equ 0 (
            for /f "tokens=1 delims=|" %%A in ("!RAW_LINE!") do set "APP_ID=%%A"
        ) else (
            :: 2) Разделитель , или ; (старый формат)
            for /f "tokens=1,2 delims=,;" %%A in ("!RAW_LINE!") do (
                if not "%%B"=="" (set "APP_ID=%%B") else (set "APP_ID=%%A")
            )
        )

        for /f "tokens=* delims= " %%T in ("!APP_ID!") do set "APP_ID=%%T"

        if not "!APP_ID!"=="" (
            set /a CNT_TOTAL+=1

            echo ---------------------------------------------------
            echo [INSTALL] !APP_ID!
            echo ---------------------------------------------------
            echo [INSTALL] !APP_ID! >> "%LOG_FILE%"

            echo !APP_ID! | findstr /i "telegram" >nul
            if !errorLevel! equ 0 (
                set "TG_FOUND=1"
                echo [INFO] Telegram detected >> "%LOG_FILE%"
            )

            call :ProcessOne "!APP_ID!"
        )
    )
)
goto :after_loop

:: ============================================================
:: Subroutine: process one package
:: ============================================================
:ProcessOne
    setlocal
    set "PKG=%~1"

    :: --- Проверка: уже установлен? (через вывод winget list) ---
    set "FOUND=0"
    for /f "delims=" %%O in ('winget list --id "!PKG!" --source winget 2^>nul') do (
        echo %%O | findstr /i /c:"!PKG!" >nul 2>&1
        if not errorlevel 1 set "FOUND=1"
    )

    if "!FOUND!"=="1" (
        echo [SKIP] !PKG! already installed.
        echo [SKIP] !PKG! >> "%LOG_FILE%"
        endlocal & set /a CNT_SKIP+=1
        exit /b 0
    )

    :: --- Установка с retry (до 3 попыток) ---
    set "ATTEMPT=0"
    :retry_install
        winget install --id "!PKG!" --source winget --silent --accept-package-agreements --accept-source-agreements --disable-interactivity >> "%LOG_FILE%" 2>&1
        if not errorlevel 1 goto :install_ok
        set /a ATTEMPT+=1
        if !ATTEMPT! lss 3 (
            timeout /t 2 /nobreak >nul
            goto :retry_install
        )
        goto :install_fail

    :install_ok
        echo [OK] !PKG! installed.
        echo [OK] !PKG! >> "%LOG_FILE%"
        endlocal & set /a CNT_OK+=1
        exit /b 0

    :install_fail
        set "WINGET_RC=!errorLevel!"
        echo [WARN] Failed to install !PKG!. Exit code: !WINGET_RC!
        echo [FAIL] !PKG! (exit !WINGET_RC!) >> "%LOG_FILE%"
        endlocal & set /a CNT_FAIL+=1 & set "FAILED_LIST=%FAILED_LIST% %~1"
        exit /b 1

:after_loop

:: ============================================================
:: Summary
:: ============================================================
echo ===================================================
echo   SUMMARY
echo ===================================================
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
echo   Total     : %CNT_TOTAL% >> "%LOG_FILE%"
echo   Installed : %CNT_OK% >> "%LOG_FILE%"
echo   Skipped   : %CNT_SKIP% >> "%LOG_FILE%"
echo   Failed    : %CNT_FAIL% >> "%LOG_FILE%"
if not "%FAILED_LIST%"=="" echo   Failed list:%FAILED_LIST% >> "%LOG_FILE%"
echo   Telegram  : %TG_FOUND% >> "%LOG_FILE%"
echo =================================================== >> "%LOG_FILE%"
echo   Session ended at %date% %time% >> "%LOG_FILE%"

:: ============================================================
:: TgWsProxy — ONLY if Telegram was in selection
:: ============================================================
if "%TG_FOUND%"=="0" goto :tg_done

echo.
echo ===================================================
echo   TgWsProxy setup
echo ===================================================
echo   TgWsProxy setup >> "%LOG_FILE%"

echo [INFO] Telegram found in selection - setting up TgWsProxy...
echo [INFO] Setting up TgWsProxy >> "%LOG_FILE%"

if not exist "%TG_SRC%" (
    echo [WARN] Source file not found: %TG_SRC%
    echo [WARN] Source file not found >> "%LOG_FILE%"
    goto :tg_done
)
echo [OK] Source: %TG_SRC%
echo [OK] Source: %TG_SRC% >> "%LOG_FILE%"

if not exist "%TG_DIR%" (
    echo [INFO] Creating folder: %TG_DIR%
    echo [INFO] Creating folder: %TG_DIR% >> "%LOG_FILE%"
    mkdir "%TG_DIR%"
    if !errorLevel! neq 0 (
        echo [ERROR] Failed to create folder: %TG_DIR%
        echo [ERROR] Failed to create folder >> "%LOG_FILE%"
        goto :tg_done
    )
)

:: --- Останавливаем TgWsProxy, если запущен (иначе файл занят) ---
taskkill /f /im TgWsProxy_windows_7_64bit.exe >nul 2>&1
timeout /t 1 /nobreak >nul

echo [INFO] Copying TgWsProxy to: %TG_EXE%
echo [INFO] Copying to %TG_EXE% >> "%LOG_FILE%"
copy /y "%TG_SRC%" "%TG_EXE%" >> "%LOG_FILE%" 2>&1

if errorlevel 1 (
    echo [ERROR] Copy failed (code !errorLevel!): %TG_EXE%
    echo [ERROR] Copy failed >> "%LOG_FILE%"
    goto :tg_done
)
if not exist "%TG_EXE%" (
    echo [ERROR] Copy failed. File not present: %TG_EXE%
    echo [ERROR] Copy failed >> "%LOG_FILE%"
    goto :tg_done
)
echo [OK] Copied successfully.
echo [OK] Copy successful >> "%LOG_FILE%"

schtasks /query /tn "%TG_TASK%" >nul 2>&1
if !errorLevel! equ 0 (
    echo [INFO] Removing existing task "%TG_TASK%"...
    echo [INFO] Removing old task >> "%LOG_FILE%"
    schtasks /delete /tn "%TG_TASK%" /f >> "%LOG_FILE%" 2>&1
)

echo [INFO] Creating autostart task: %TG_TASK%
echo [INFO] Creating autostart task >> "%LOG_FILE%"
schtasks /create /tn "%TG_TASK%" /tr "\"%TG_EXE%\"" /sc onlogon /rl highest /f >> "%LOG_FILE%" 2>&1

if !errorLevel! equ 0 (
    echo [OK] Autostart task created: %TG_TASK%
    echo [OK] Autostart task created >> "%LOG_FILE%"
) else (
    echo [WARN] Failed to create autostart task. Exit code: !errorLevel!
    echo [FAIL] Autostart task creation >> "%LOG_FILE%"
)

echo [INFO] Launching TgWsProxy from: %TG_EXE%
echo [INFO] Launching from %TG_EXE% >> "%LOG_FILE%"
start "" /b "%TG_EXE%"

if !errorLevel! equ 0 (
    echo [OK] TgWsProxy launched.
    echo [OK] TgWsProxy launched >> "%LOG_FILE%"
) else (
    echo [WARN] Failed to launch TgWsProxy. Exit code: !errorLevel!
    echo [FAIL] TgWsProxy launch exit !errorLevel! >> "%LOG_FILE%"
)

:tg_done
echo.

:: ============================================================
:: Final marker for launcher
:: ============================================================
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