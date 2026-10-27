@echo off
setlocal enabledelayedexpansion
:: Без chcp 65001 для предотвращения кракозябр
title Installing apps via winget

set "ROOT=%~dp0"
set "SOURCE_DIR=%ROOT%source"
set "EXE_DIR=%SOURCE_DIR%\exe"
set "LOG_DIR=%ROOT%logs"

if not defined REPO_FILE set "REPO_FILE=%SOURCE_DIR%\repositories.txt"

set "TG_DIR=%ProgramFiles%\TgWsProxy"
set "TG_EXE=%TG_DIR%\TgWsProxy_windows_7_64bit.exe"
set "TG_SRC=%EXE_DIR%\TgWsProxy_windows_7_64bit.exe"
set "TG_TASK=TgWsProxy"

if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"
forfiles /p "%LOG_DIR%" /m install_*.log /d -30 /c "cmd /c del @path" >nul 2>&1

for /f "tokens=1-4 delims=/:. " %%a in ("%date% %time%") do set "STAMP=%%d%%b%%c_%%a"
if "%STAMP%"=="" set "STAMP=%RANDOM%"
set "LOG_FILE=%LOG_DIR%\install_%STAMP%.log"

echo. > "%LOG_FILE%"

echo [INFO] Log file: %LOG_FILE%
echo [INFO] Repository: %REPO_FILE%
echo ============================================================ >> "%LOG_FILE%"
echo   install session started at %date% %time%                 >> "%LOG_FILE%"
echo   Repository: %REPO_FILE%                                 >> "%LOG_FILE%"
echo   Manager: winget (no msstore)                             >> "%LOG_FILE%"
echo ============================================================ >> "%LOG_FILE%"

net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [WARN] Not running as Administrator.
    echo [WARN] Not running as Administrator >> "%LOG_FILE%"
)

if not exist "%REPO_FILE%" (
    echo [ERROR] Repository file not found: %REPO_FILE%
    echo [ERROR] Repository file not found >> "%LOG_FILE%"
    echo [BAT_DONE] rc=2
    if not defined REPO_FILE pause
    exit /b 2
)

:: ============================================================
:: Проверка winget
:: ============================================================
winget --version >nul 2>&1
if %errorLevel% neq 0 (
    echo [ERROR] winget not found or not in PATH.
    echo [ERROR] winget not found >> "%LOG_FILE%"
    echo [BAT_DONE] rc=2
    if not defined REPO_FILE pause
    exit /b 2
)

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

:: Format: winget.id|Name
for /f "usebackq tokens=* eol=#" %%L in ("%REPO_FILE%") do (
    set "RAW_LINE=%%L"

    if defined RAW_LINE (
        for /f "tokens=1 delims=﻿" %%B in ("!RAW_LINE!") do set "RAW_LINE=%%B"
    )

    if not "!RAW_LINE!"=="" (
        set "APP_ID="
        set "APP_NAME="

        for /f "tokens=1,2 delims=|" %%A in ("!RAW_LINE!") do (
            set "APP_ID=%%A"
            set "APP_NAME=%%B"
        )
        if "!APP_ID!"=="" set "APP_ID=!RAW_LINE!"

        for /f "tokens=* delims= " %%T in ("!APP_ID!")    do set "APP_ID=%%T"
        for /f "tokens=* delims= " %%T in ("!APP_NAME!")  do set "APP_NAME=%%T"

        if not "!APP_ID!"=="" (
            set /a CNT_TOTAL+=1
            echo ---------------------------------------------------
            echo [INSTALL] !APP_ID!
            echo ---------------------------------------------------
            echo [INSTALL] !APP_ID! >> "%LOG_FILE%"

            echo !APP_ID! | findstr /i "telegram" >nul
            if !errorLevel! equ 0 set "TG_FOUND=1"

            call :ProcessOne "!APP_ID!" "!APP_NAME!"
        )
    )
)
goto :after_loop

:ProcessOne
    setlocal
    set "PKG=%~1"
    set "NAME=%~2"

    :: ---- Check installed ----
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

    :: ---- Установка ТОЛЬКО через winget ----
    winget install --id "!PKG!" --source winget --silent --accept-package-agreements --accept-source-agreements --disable-interactivity >> "%LOG_FILE%" 2>&1
    if !errorLevel! equ 0 goto :install_ok

    winget list --id "!PKG!" --source winget >nul 2>&1
    if !errorLevel! equ 0 (
        echo [SKIP] !PKG! already installed ^(winget, verified^).
        echo [SKIP] !PKG! >> "%LOG_FILE%"
        endlocal & set /a CNT_SKIP+=1
        exit /b 0
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
ping -n 2 127.0.0.1 >nul

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