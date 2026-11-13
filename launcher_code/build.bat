@echo off
chcp 65001 >nul
setlocal

echo ========================================================
echo           Сборка WinGET Launcher (UCRT)
echo ========================================================
echo.

:: 1. Компиляция ресурсов
echo [INFO] Компиляция ресурсов (resource.rc -> app.res)...
C:\launcher_code\mingw64\bin\windres.exe --use-temp-file -i resource.rc -O coff -o app.res

if %errorlevel% neq 0 (
    echo [ERROR] Ошибка компиляции ресурсов!
    pause
    exit /b 1
)
echo [OK] Ресурсы успешно скомпилированы.
echo.

:: 2. Сборка лаунчера через GCC
echo [INFO] Сборка исполняемого файла (launcher.exe)...
C:\launcher_code\mingw64\bin\gcc.exe launcher.c app.res -o launcher.exe -O3 -mwindows -municode -lcomctl32 -lwininet -lgdi32 -lmsimg32 -lucrt "-Wl,-s"

if %errorlevel% neq 0 (
    echo [ERROR] Ошибка сборки GCC!
    pause
    exit /b 1
)

echo.
echo [SUCCESS] Лаунчер успешно собран: launcher.exe
echo.
pause