@echo off
rem Build STARFALL and run it. Works from this folder so data\ is found.
setlocal
cd /d "%~dp0"

rem Some locked-down Windows setups refuse writes to %TEMP%. Give the
rem compiler a private scratch folder inside the project instead.
rem (build\tmp is disposable and covered by .gitignore.)
set "BUILDTMP=%~dp0build\tmp"
if not exist "%BUILDTMP%" mkdir "%BUILDTMP%" >nul 2>nul
set "TMP=%BUILDTMP%"
set "TEMP=%BUILDTMP%"
set "TMPDIR=%BUILDTMP%"

rem Prefer the portable toolchain if it sits next to the project.
set "GPP="
if exist "%~dp0..\.tools\w64devkit\bin\g++.exe" set "GPP=%~dp0..\.tools\w64devkit\bin\g++.exe"
if not defined GPP (
    where g++ >nul 2>nul && set "GPP=g++"
)
if not defined GPP (
    echo.
    echo No g++ found.
    echo Install a compiler: w64devkit ^(portable zip^) or MSYS2 UCRT64 + gcc.
    echo Then run this file again.
    echo.
    pause
    exit /b 1
)

echo Building with "%GPP%" ...
"%GPP%" -pedantic-errors -std=c++11 -Wall -Wextra -O2 *.cpp -o main.exe
if errorlevel 1 (
    echo.
    echo Build failed. Copy the errors above.
    pause
    exit /b 1
)

if exist "%~dp0build\tmp" rmdir /s /q "%~dp0build\tmp" >nul 2>nul
rem (cleanup is best effort; left-over build\tmp is disposable)

echo Build OK. Starting STARFALL...
main.exe %*
