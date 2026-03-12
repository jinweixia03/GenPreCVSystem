@echo off
setlocal enabledelayedexpansion

REM GenPreCVSystem Deployment Script

echo ============================================
echo Starting GenPreCVSystem Deployment...
echo ============================================

set "PROJECT_DIR=%~dp0"
set "BUILD_DIR=%PROJECT_DIR%build"
set "RELEASE_DIR=%BUILD_DIR%\Release"
set "EXE_FILE=%RELEASE_DIR%\GenPreCVSystem.exe"

REM Detect Qt Installation Path

if defined QT_DIR (
    set QT_PATH=%QT_DIR%
    echo Using QT_DIR: %QT_PATH%
    goto :QT_FOUND
)

if defined QTDIR (
    set QT_PATH=%QTDIR%
    echo Using QTDIR: %QT_PATH%
    goto :QT_FOUND
)

set CMAKE_CACHE=%BUILD_DIR%\CMakeCache.txt
if exist "%CMAKE_CACHE%" (
    for /f "tokens=2 delims==" %%a in ('findstr /R "Qt[0-9]*_DIR:PATH" "%CMAKE_CACHE%" 2^>nul') do (
        set "QT_CMAKE_PATH=%%a"
        for %%b in ("%%a\..\..\..") do set "QT_PATH=%%~dpnb"
        echo Found Qt from CMake: !QT_PATH!
        goto :QT_FOUND
    )
)

set QT_VERSION=6.9.3
set QT_COMPILER=msvc2022_64

set "QT_PATH=C:\Qt\%QT_VERSION%\%QT_COMPILER%"
if exist "%QT_PATH%\bin\windeployqt.exe" goto :QT_FOUND

set "QT_PATH=D:\Environments\CPlusPlus\Qt\%QT_VERSION%\%QT_COMPILER%"
if exist "%QT_PATH%\bin\windeployqt.exe" goto :QT_FOUND

echo Error: Cannot find Qt installation!
echo Please set QT_DIR environment variable
pause
exit /b 1

:QT_FOUND
echo Qt Path: %QT_PATH%
set "WINDEPLOYQT=%QT_PATH%\bin\windeployqt.exe"

REM Verify Files

echo.
echo [1/7] Checking executable...
if not exist "%EXE_FILE%" (
    echo Error: Executable not found: %EXE_FILE%
    pause
    exit /b 1
)
echo Found: %EXE_FILE%

echo.
echo [2/7] Checking windeployqt...
if not exist "%WINDEPLOYQT%" (
    echo Error: windeployqt.exe not found
    pause
    exit /b 1
)
echo Found: %WINDEPLOYQT%

REM Deploy

echo.
echo [3/7] Running windeployqt...
"%WINDEPLOYQT%" "%EXE_FILE%" --no-translations

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Deployment failed!
    pause
    exit /b 1
)

REM Cleanup Root Directory

echo.
echo [4/7] Cleaning root directory...

for %%f in (Qt6*.dll opengl32sw.dll D3Dcompiler_*.dll d3dcompiler_*.dll) do (
    if exist "%PROJECT_DIR%%%f" (
        del /f /q "%PROJECT_DIR%%%f" 2>nul
    )
)

for %%d in (generic iconengines imageformats networkinformation platforms styles tls translations) do (
    if exist "%PROJECT_DIR%%%d" (
        rmdir /s /q "%PROJECT_DIR%%%d" 2>nul
    )
)

echo Cleanup complete!

REM Copy Resources

echo.
echo [5/7] Copying resources...

set "RESOURCES_SRC=%PROJECT_DIR%src\resources\images"
set "RESOURCES_DEST=%RELEASE_DIR%\images"

if exist "%RESOURCES_SRC%" (
    if not exist "%RESOURCES_DEST%" mkdir "%RESOURCES_DEST%"
    xcopy /Y /I "%RESOURCES_SRC%\*.svg" "%RESOURCES_DEST%\" >nul 2>&1
    echo Resources copied
)

echo.
echo [6/7] Copying Python scripts...
set PYTHON_SRC=%PROJECT_DIR%src\services\inference\python
set PYTHON_DEST=%RELEASE_DIR%\python

if exist "%PYTHON_SRC%" (
    if not exist "%PYTHON_DEST%" mkdir "%PYTHON_DEST%"
    xcopy /Y /I "%PYTHON_SRC%\*.py" "%PYTHON_DEST%\" >nul 2>&1
    echo Python scripts copied
)

echo.
echo [7/7] Copying model files...
set MODELS_SRC=%PROJECT_DIR%src\resources\models
set MODELS_DEST=%RELEASE_DIR%\models

if exist "%MODELS_SRC%" (
    if not exist "%MODELS_DEST%" mkdir "%MODELS_DEST%"
    xcopy /Y /S /I "%MODELS_SRC%\*" "%MODELS_DEST%\" >nul 2>&1
    echo Model files copied
)

REM Complete

echo.
echo ============================================
echo Deployment Complete!
echo Directory: %RELEASE_DIR%
echo ============================================
echo.
echo Run with: %RELEASE_DIR%\GenPreCVSystem.exe
echo.

timeout /t 2 /nobreak >nul
exit /b 0
