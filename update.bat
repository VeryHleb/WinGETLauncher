@echo off
setlocal enabledelayedexpansion
chcp 65001 >nul
title WinGET Launcher — Обновление

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

set "GH_RAW=https://raw.githubusercontent.com/VeryHleb/WinGETLauncher/main/"
set "TMP_LIST=%TEMP%\wl_files.txt"
set "SELF_NAME=update.bat"

echo ===================================================
echo   WinGET Launcher - Обновление
echo ===================================================
echo.
echo   Директория: %ROOT%
echo.

:: ---- Ждём закрытия launcher.exe ----
echo [INFO] Ожидание закрытия launcher.exe...
:wait_launcher
tasklist /FI "IMAGENAME eq launcher.exe" 2>nul | find /I "launcher.exe" >nul
if not errorlevel 1 (
    timeout /t 1 /nobreak >nul
    goto wait_launcher
)
timeout /t 2 /nobreak >nul
echo [OK]   launcher.exe закрыт.
echo.

:: ---- Скачиваем files.txt ----
echo [INFO] Скачивание списка файлов...
if exist "%TMP_LIST%" del "%TMP_LIST%" >nul 2>&1
curl.exe -fsSL -A "WinGETLauncher-Update/1.0" -o "%TMP_LIST%" "%GH_RAW%source/files.txt"
if not exist "%TMP_LIST%" (
    echo [ERROR] Не удалось скачать source/files.txt
    goto end_error
)

:: ---- Считаем количество файлов ----
set /a TOTAL_COUNT=0
for /f "usebackq tokens=* eol=#" %%L in ("%TMP_LIST%") do (
    set "LINE=%%L"
    if not "!LINE!"=="" set /a TOTAL_COUNT+=1
)

if %TOTAL_COUNT% equ 0 (
    echo [ERROR] Список файлов пуст.
    goto end_error
)

echo [INFO] Файлов для обновления: %TOTAL_COUNT%
echo.

:: ---- Удаляем и скачиваем каждый файл ----
set /a OK_COUNT=0
set /a FAIL_COUNT=0
set /a SKIP_COUNT=0

for /f "usebackq tokens=* eol=#" %%L in ("%TMP_LIST%") do (
    set "REL=%%L"
    if not "!REL!"=="" (
        call :ReplaceOne "!REL!"
    )
)

:: ---- Обновляем version.txt ----
call :ReplaceOne "version.txt"

echo.
echo ===================================================
echo   Обновление завершено
echo ===================================================
echo   Успешно: %OK_COUNT%
echo   Пропущено (в работе): %SKIP_COUNT%
echo   Ошибок:  %FAIL_COUNT%
echo ===================================================
echo.

del "%TMP_LIST%" >nul 2>&1

:: ---- Перезапускаем лаунчер ----
echo [INFO] Запуск лаунчера...
if exist "%ROOT%\launcher.exe" (
    start "" "%ROOT%\launcher.exe"
) else (
    echo [WARN] launcher.exe не найден. Запустите вручную.
)

exit /b 0

:: ============================================================
:: ReplaceOne <relative/path>
::   1. Скачивает файл в <DEST>.new
::   2. Проверяет, что файл скачался
::   3. Удаляет старый <DEST> (если есть)
::   4. Переименовывает <DEST>.new -> <DEST>
:: ============================================================
:ReplaceOne
setlocal
set "REL=%~1"
if "%REL%"=="" endlocal & exit /b 1

:: Заменяем / на \ для локального пути
set "REL_WIN=%REL:/=\%"
set "DEST=%ROOT%\%REL_WIN%"
set "NEW=%DEST%.new"

:: ---- Пропускаем сам update.bat ----
if /I "%REL_WIN%"=="%SELF_NAME%" (
    echo [SKIP] %REL% ^(в работе^)
    endlocal & set /a SKIP_COUNT+=1
    exit /b 0
)

:: Создаём директорию назначения
for %%D in ("%DEST%") do set "DESTDIR=%%~dpD"
if not exist "%DESTDIR%" mkdir "%DESTDIR%" >nul 2>&1

:: Удаляем возможный старый .new
if exist "%NEW%" del /f /q "%NEW%" >nul 2>&1

:: Формируем URL (с прямыми слэшами)
set "URL=%GH_RAW%%REL%"

:: ---- 1. Скачиваем во временный .new ----
echo [GET] %REL%
curl.exe -fsSL -A "WinGETLauncher-Update/1.0" -o "%NEW%" "%URL%"
set "RC=%errorlevel%"

if %RC% neq 0 (
    echo   [FAIL] curl ошибка %RC%
    if exist "%NEW%" del /f /q "%NEW%" >nul 2>&1
    endlocal & set /a FAIL_COUNT+=1
    exit /b 1
)

if not exist "%NEW%" (
    echo   [FAIL] Файл не создан
    endlocal & set /a FAIL_COUNT+=1
    exit /b 1
)

:: Проверяем размер (пустой файл = ошибка)
for %%F in ("%NEW%") do set "NEWSIZE=%%~zF"
if "%NEWSIZE%"=="0" (
    echo   [FAIL] Пустой файл
    del /f /q "%NEW%" >nul 2>&1
    endlocal & set /a FAIL_COUNT+=1
    exit /b 1
)

:: ---- 2. Удаляем старый файл ----
if exist "%DEST%" (
    del /f /q "%DEST%" >nul 2>&1
    if exist "%DEST%" (
        echo   [WARN] Не удалось удалить старый файл
        if exist "%NEW%" del /f /q "%NEW%" >nul 2>&1
        endlocal & set /a FAIL_COUNT+=1
        exit /b 1
    )
    echo   [DEL] %REL%
)

:: ---- 3. Переименовываем .new -> .DEST ----
move /y "%NEW%" "%DEST%" >nul 2>&1
if not exist "%DEST%" (
    echo   [FAIL] Не удалось переименовать
    endlocal & set /a FAIL_COUNT+=1
    exit /b 1
)

echo   [OK]  %REL%
endlocal & set /a OK_COUNT+=1
exit /b 0

:: ============================================================
:end_error
echo.
echo [ERROR] Обновление не завершено.
pause
exit /b 1