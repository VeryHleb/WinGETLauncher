@echo off
chcp 65001 >nul
title Сборка проекта WinGET Launcher + setup.exe

if exist "%~dp0mingw64\bin" (
    set "PATH=%~dp0mingw64\bin;%PATH%"
)

set "APP_VER=1.2"
if exist "%~dp0version.txt" (
    set /p APP_VER=<"%~dp0version.txt"
)
echo [INFO] Версия из version.txt: %APP_VER%

echo [RES] Компиляция ресурсов...
if exist resource.rc (
    windres resource.rc -O coff -o resource.res
) else (
    echo [WARN] resource.rc не найден, пропускаем ресурсы.
)

echo [1/2] Сборка launcher.exe...
if exist resource.res (
    gcc launcher_2.c resource.res -o launcher.exe -mwindows -municode -O2 -static -DAPP_VERSION_STR="L\"%APP_VER%\"" -lole32 -lcomctl32 -luxtheme -lshlwapi -lwininet -lshell32 -lmsimg32 -luuid
) else (
    gcc launcher_2.c -o launcher.exe -mwindows -municode -O2 -static -DAPP_VERSION_STR="L\"%APP_VER%\"" -lole32 -lcomctl32 -luxtheme -lshlwapi -lwininet -lshell32 -lmsimg32 -luuid
)
if errorlevel 1 (
    echo [ERROR] Ошибка компиляции launcher.exe!
    pause
    exit /b 1
)

echo [2/2] Сборка setup.exe...
if exist resource.res (
    gcc setup_v3.c resource.res -o setup.exe -mwindows -municode -O2 -static -lole32 -lcomctl32 -luxtheme -lshlwapi -lwininet -lshell32 -lmsimg32 -luuid
) else (
    gcc setup_v3.c -o setup.exe -mwindows -municode -O2 -static -lole32 -lcomctl32 -luxtheme -lshlwapi -lwininet -lshell32 -lmsimg32 -luuid
)
if errorlevel 1 (
    echo [ERROR] Ошибка компиляции setup.exe!
    pause
    exit /b 1
)

echo.
echo [SUCCESS] Собраны launcher.exe и setup.exe (версия %APP_VER%)!
pause