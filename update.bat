@echo off
setlocal enabledelayedexpansion
chcp 65001 >nul
title WinGET Launcher — Обновление

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

:: ============================================================
:: НАСТРОЙКИ GITHUB
:: ============================================================
set "GH_RAW=https://raw.githubusercontent.com/VeryHleb/WinGETLauncher/main/"
set "TMP_LIST=%TEMP%\wl_files.txt"
set "SELF_NAME=update.bat"
set "BS=\"

echo ===================================================
echo   WinGET Launcher - Обновление
echo ===================================================
echo.

:: ---- Ждём закрытия launcher.exe ----
echo [INFO] Ожидание закрытия launcher.exe...
set /a WAIT_COUNT=0
:wait_launcher
tasklist /FI "IMAGENAME eq launcher.exe" 2>nul | find /I "launcher.exe" >nul
if not errorlevel 1 (
    set /a WAIT_COUNT+=1
    if !WAIT_COUNT! gtr 10 (
        echo [WARN] launcher.exe не закрылся за 10 секунд. Принудительное завершение...
        taskkill /F /IM launcher.exe >nul 2>&1
        timeout /t 1 /nobreak >nul
        goto wait_done
    )
    timeout /t 1 /nobreak >nul
    goto wait_launcher
)
:wait_done
echo [OK] launcher.exe закрыт.
echo.

:: ---- Скачиваем files.txt ----
echo [INFO] Скачивание списка файлов...
if exist "%TMP_LIST%" del "%TMP_LIST%" >nul 2>&1
curl.exe -fsSL -A "WinGETLauncher-Update/1.0" -o "%TMP_LIST%" "%GH_RAW%source/files.txt"
if not exist "%TMP_LIST%" (
    echo [ERROR] Не удалось скачать source/files.txt
    goto end_error
)

set /a OK_COUNT=0
set /a FAIL_COUNT=0
set /a SKIP_COUNT=0

:: Читаем файл
for /f "usebackq tokens=* eol=#" %%L in ("%TMP_LIST%") do (
    set "LINE=%%L"
    :: Очистка от пробелов и невидимых символов
    set "LINE=!LINE: =!"
    set "LINE=!LINE:	=!"
    
    if not "!LINE!"=="" (
        call :ProcessLine "!LINE!"
    )
)

:: ---- Обновляем version.txt (принудительно) ----
call :ProcessLine "version.txt"

echo.
echo ===================================================
echo   Обновление завершено
echo ===================================================
echo   Успешно: %OK_COUNT%
echo   Ошибок:  %FAIL_COUNT%
echo ===================================================
echo.

del "%TMP_LIST%" >nul 2>&1

:: ---- Перезапускаем лаунчер ----
if exist "%ROOT%\launcher.exe" start "" "%ROOT%\launcher.exe"
exit /b 0

:: ============================================================
:: ProcessLine <line>
:: ============================================================
:ProcessLine
setlocal
set "LINE=%~1"
if "%LINE%"=="" endlocal & exit /b 1

set "REL=!LINE!"
:: Заменяем прямые слэши на обратные
set "REL_WIN=!REL:/=%BS%!"
set "DEST=%ROOT%\!REL_WIN!"
set "NEW=!DEST!.new"

:: Пропускаем сам update.bat
if /I "!REL_WIN!"=="%SELF_NAME%" (
    echo [SKIP] !REL! ^(в работе^)
    endlocal & set /a SKIP_COUNT+=1
    exit /b 0
)

set "URL=%GH_RAW%!REL!"

for %%D in ("!DEST!") do set "DESTDIR=%%~dpD"
if not exist "!DESTDIR!" mkdir "!DESTDIR!" >nul 2>&1
if exist "!NEW!" del /f /q "!NEW!" >nul 2>&1

:: --- Скачиваем ---
echo [GET] !REL! (GitHub)
curl.exe -fsSL -A "WinGETLauncher-Update/1.0" -o "!NEW!" "!URL!"
if !errorlevel! neq 0 (
    echo   [FAIL] Ошибка скачивания
    if exist "!NEW!" del /f /q "!NEW!" >nul 2>&1
    endlocal & set /a FAIL_COUNT+=1
    exit /b 1
)

:: --- Проверка и замена ---
if not exist "!NEW!" (
    echo   [FAIL] Файл не создан
    endlocal & set /a FAIL_COUNT+=1
    exit /b 1
)

for %%F in ("!NEW!") do set "NEWSIZE=%%~zF"
if "!NEWSIZE!"=="0" (
    echo   [FAIL] Пустой файл
    del /f /q "!NEW!" >nul 2>&1
    endlocal & set /a FAIL_COUNT+=1
    exit /b 1
)

if exist "!DEST!" (
    del /f /q "!DEST!" >nul 2>&1
)

move /y "!NEW!" "!DEST!" >nul 2>&1
if not exist "!DEST!" (
    echo   [FAIL] Не удалось заменить файл
    endlocal & set /a FAIL_COUNT+=1
    exit /b 1
)

echo   [OK]  Готово
endlocal & set /a OK_COUNT+=1
exit /b 0

:end_error
echo.
echo [ERROR] Обновление не завершено.
pause
exit /b 1