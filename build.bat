@echo off
setlocal enabledelayedexpansion

:: Convert current script directory to 8.3 short path to prevent issues with Unicode / Cyrillic paths in GCC / ld
for %%I in ("%~dp0") do set "ROOT_DIR=%%~sI"
cd /d "%ROOT_DIR%"

echo ========================================================
echo   Shadow Dimension - Universal Build Script
echo ========================================================

set "DEV_BIN=%ROOT_DIR%tools\w64devkit\bin"
set "RAYLIB_INC=%ROOT_DIR%tools\raylib-5.0_win64_mingw-w64\include"
set "RAYLIB_LIB=%ROOT_DIR%tools\raylib-5.0_win64_mingw-w64\lib"

:: Check if local compiler or raylib is missing, auto-download if needed
if not exist "%DEV_BIN%\g++.exe" (
    echo Local build tools not found. Downloading automatically...
    powershell -ExecutionPolicy Bypass -File "%ROOT_DIR%tools\download_tools.ps1"
)
if not exist "%RAYLIB_INC%\raylib.h" (
    echo Local Raylib not found. Downloading automatically...
    powershell -ExecutionPolicy Bypass -File "%ROOT_DIR%tools\download_tools.ps1"
)

:: Prefer bundled toolchain if present, otherwise fallback to system g++
set CXX=
if exist "%DEV_BIN%\g++.exe" (
    set "PATH=%DEV_BIN%;%PATH%"
    set CXX="%DEV_BIN%\g++.exe"
) else (
    where g++ >nul 2>nul
    if !ERRORLEVEL! EQU 0 (
        set CXX=g++
    ) else (
        echo [ERROR] Neither local w64devkit nor system g++ was found!
        echo Please run powershell tools\download_tools.ps1 or install MinGW.
        pause
        exit /b 1
    )
)

echo Compiling Shadow Dimension...
%CXX% -std=c++17 -O2 "main.cpp" -o "shadow_dimension.exe" -I"%RAYLIB_INC%" -L"%RAYLIB_LIB%" -lraylib -lopengl32 -lgdi32 -lwinmm -static -static-libgcc -static-libstdc++

if %ERRORLEVEL% EQU 0 (
    echo ========================================================
    echo Build SUCCESSFUL! Output: shadow_dimension.exe
    echo ========================================================
) else (
    echo ========================================================
    echo Build FAILED with error code %ERRORLEVEL%
    echo ========================================================
)
endlocal
