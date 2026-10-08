@echo off
setlocal

where gcc >nul 2>&1
if %errorLevel% neq 0 (
    echo [ERROR] gcc not found in PATH.
    pause
    exit /b 1
)

echo [INFO] Compiling resource.rc ...
windres resource.rc -O coff -o resource.res
if %errorLevel% neq 0 (
    echo [ERROR] windres failed.
    pause
    exit /b 1
)

echo [INFO] Compiling launcher.c ...
gcc launcher.c resource.res -o launcher.exe ^
    -mwindows ^
    -municode ^
    -O2 ^
    -s ^
    -static ^
    -lcomctl32 -luser32 -lgdi32 -lshell32 -lmsimg32
if %errorLevel% neq 0 (
    echo [ERROR] gcc failed.
    pause
    exit /b 1
)

echo.
echo [OK] launcher.exe built.
dir launcher.exe
pause