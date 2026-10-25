@echo off
setlocal enabledelayedexpansion
title Установка Microsoft Store и зависимостей

:: Проверка прав администратора
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [ОШИБКА] Запустите этот скрипт от имени Администратора!
    pause
    exit /b
)

cd /d "%~dp0"
echo ===================================================
echo   Установка Microsoft Store (Appx)
echo ===================================================
echo.

:: 1. Устанавливаем зависимости (VCLibs, NET.Native и т.д.)
echo [1/2] Установка зависимостей...
for %%f in (Microsoft.NET.Native.Framework*.Appx Microsoft.NET.Native.Runtime*.Appx Microsoft.VCLibs*.Appx Microsoft.UI.Xaml*.Appx) do (
    if exist "%%f" (
        echo Установка: %%~nxf
        powershell -Command "Add-AppxPackage -Path '%%~ff'" >nul 2>&1
    )
)

:: 2. Устанавливаем сам Microsoft Store
echo.
echo [2/2] Установка Microsoft.WindowsStore...
for %%f in (Microsoft.WindowsStore*.Msixbundle) do (
    if exist "%%f" (
        echo Установка: %%~nxf
        powershell -Command "Add-AppxPackage -Path '%%~ff'" >nul 2>&1
        if !errorLevel! equ 0 (
            echo [OK] Microsoft Store успешно установлен.
        ) else (
            echo [ПРЕДУПРЕЖДЕНИЕ] Возможно, уже установлен или требуется перезагрузка.
        )
    )
)

echo.
echo ===================================================
echo   Готово.
echo ===================================================
pause