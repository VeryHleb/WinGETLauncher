@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

:: =================================================================
:: УКАЖИТЕ ПУТИ ДЛЯ ДОБАВЛЕНИЯ В СИСТЕМНЫЙ PATH (разделяйте точкой с запятой)
:: Пример: C:\mingw64\bin;C:\Tools
:: =================================================================
set "NEW_PATHS=C:\mingw64\bin"

:: Проверка прав администратора (нужны для изменения системного PATH)
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [WARN] Запрос прав администратора для изменения системного PATH...
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

echo [INFO] Обновление системной переменной PATH...

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
    "$newPaths = '%NEW_PATHS%' -split ';';" ^
    "$currentPath = [Environment]::GetEnvironmentVariable('Path', 'Machine');" ^
    "$pathArray = $currentPath -split ';' | Where-Object { $_ -ne '' };" ^
    "$updated = $false;" ^
    "foreach ($p in $newPaths) {" ^
    "    $trimmed = $p.Trim().TrimEnd('\');" ^
    "    if ($trimmed -and ($pathArray -notcontains $trimmed) -and ($pathArray -notcontains ($trimmed + '\'))) {" ^
    "        $pathArray += $trimmed;" ^
    "        Write-Host '[+] Добавлен путь:' $trimmed -ForegroundColor Green;" ^
    "        $updated = $true;" ^
    "    } else {" ^
    "        Write-Host '[=] Уже существует или пуст:' $trimmed -ForegroundColor Yellow;" ^
    "    }" ^
    "}" ^
    "if ($updated) {" ^
    "    $finalPath = $pathArray -join ';';" ^
    "    [Environment]::SetEnvironmentVariable('Path', $finalPath, 'Machine');" ^
    "    Write-Host '[OK] Системный PATH успешно обновлен!' -ForegroundColor Cyan;" ^
    "} else {" ^
    "    Write-Host '[INFO] Изменения не требуются.' -ForegroundColor Gray;" ^
    "}"

echo.
echo [NOTE] Перезапустите открытые окна терминала или IDE, чтобы изменения вступили в силу.
pause