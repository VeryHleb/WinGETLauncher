@echo off
setlocal

:: Обёртка для search_winget.ps1
:: Позволяет запускать поиск из cmd:
::   search_winget.bat vmware
::   search_winget.bat vmware --add
::   search_winget.bat vmware --add-all
::

set "PS1=%~dp0search_winget.ps1"

if not exist "%PS1%" (
    echo [ERROR] search_winget.ps1 not found: %PS1%
    exit /b 1
)

:: Собираем аргументы для PowerShell
set "ARGS=-NoProfile -ExecutionPolicy Bypass -File "%PS1%""

if "%~1"=="" (
    echo Usage: search_winget.bat ^<query^> [--add ^<id^> ^| --add-all]
    exit /b 1
)

:: Первый аргумент — query
set "ARGS=%ARGS% -Query "%~1""

:: Разбор остальных аргументов
shift
:parse_args
if "%~1"=="" goto :run
if /i "%~1"=="--add" (
    if "%~2"=="" (
        set "ARGS=%ARGS% -Add"
        shift
    ) else (
        set "ARGS=%ARGS% -AddId "%~2""
        shift
        shift
    )
    goto :parse_args
)
if /i "%~1"=="--add-all" (
    set "ARGS=%ARGS% -AddAll"
    shift
    goto :parse_args
)
echo [WARN] Unknown argument: %~1
shift
goto :parse_args

:run
powershell %ARGS%
exit /b %errorLevel%