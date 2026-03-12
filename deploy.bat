@echo off
chcp 65001 >nul
REM ============================================
REM GenPreCVSystem 部署脚本
REM ============================================

echo ============================================
echo 开始部署 GenPreCVSystem...
echo ============================================

REM 设置项目路径
set PROJECT_DIR=%~dp0
set BUILD_DIR=%PROJECT_DIR%build
set RELEASE_DIR=%BUILD_DIR%\Release
set EXE_FILE=%RELEASE_DIR%\GenPreCVSystem.exe

REM ============================================
REM 检测 Qt 安装路径
REM ============================================

REM 首先尝试从环境变量获取 Qt 路径
if defined QT_DIR (
    set QT_PATH=%QT_DIR%
    echo 从环境变量 QT_DIR 获取 Qt 路径: %QT_PATH%
    goto :QT_FOUND
)

if defined QTDIR (
    set QT_PATH=%QTDIR%
    echo 从环境变量 QTDIR 获取 Qt 路径: %QT_PATH%
    goto :QT_FOUND
)

REM 尝试从 CMake 缓存中读取 Qt 路径
set CMAKE_CACHE=%BUILD_DIR%\CMakeCache.txt
if exist "%CMAKE_CACHE%" (
    for /f "tokens=2 delims==" %%a in ('findstr /R "Qt[0-9]*_DIR:PATH" "%CMAKE_CACHE%" 2^>nul') do (
        set QT_CMAKE_PATH=%%a
        REM 提取 Qt 根目录 (从 lib\cmake\Qt6 向上两级)
        for %%b in ("%%a\..\..") do set QT_PATH=%%~fb
        echo 从 CMake 缓存获取 Qt 路径: %QT_PATH%
        goto :QT_FOUND
    )
)

REM 尝试常见的 Qt 安装路径
set QT_VERSION=6.9.3
set QT_COMPILER=msvc2022_64

set COMMON_QT_PATHS[0]=C:\Qt\%QT_VERSION%\%QT_COMPILER%
set COMMON_QT_PATHS[1]=D:\Environments\CPlusPlus\Qt\%QT_VERSION%\%QT_COMPILER%
set COMMON_QT_PATHS[2]=C:\Qt\6.8.2\%QT_COMPILER%
set COMMON_QT_PATHS[3]=C:\Qt\6.7.3\%QT_COMPILER%
set COMMON_QT_PATHS[4]=C:\Qt\6.6.3\%QT_COMPILER%
set COMMON_QT_PATHS[5]=D:\Qt\%QT_VERSION%\%QT_COMPILER%

setlocal enabledelayedexpansion
for /L %%i in (0,1,5) do (
    set TEST_PATH=!COMMON_QT_PATHS[%%i]!
    if exist "!TEST_PATH!\bin\windeployqt.exe" (
        set QT_PATH=!TEST_PATH!
        echo 找到 Qt 路径: !QT_PATH!
        goto :QT_FOUND
    )
)

REM 尝试从注册表获取 Qt 路径
for /f "tokens=2*" %%a in ('reg query "HKLM\SOFTWARE\QtProject\Qt" /v InstallDir 2^>nul') do (
    set QT_PATH=%%b\%QT_VERSION%\%QT_COMPILER%
    if exist "!QT_PATH!\bin\windeployqt.exe" (
        echo 从注册表获取 Qt 路径: !QT_PATH!
        goto :QT_FOUND
    )
)

REM 未找到 Qt 路径，显示错误并提示用户
echo.
echo 错误: 无法自动检测 Qt 安装路径！
echo.
echo 请尝试以下解决方案之一：
echo   1. 设置 QT_DIR 环境变量指向 Qt 安装目录
echo      例如: set QT_DIR=C:\Qt\6.9.3\msvc2022_64
echo   2. 修改本脚本顶部的 QT_VERSION 和 QT_COMPILER 变量
echo   3. 直接编辑脚本设置 QT_PATH 变量
echo.
echo 或者，您可以手动运行 windeployqt：
echo   C:\Path\To\Qt\bin\windeployqt.exe %EXE_FILE%
echo.
pause
exit /b 1

:QT_FOUND
set WINDEPLOYQT=%QT_PATH%\bin\windeployqt.exe

REM ============================================
REM 验证文件和工具
REM ============================================

echo.
echo [1/6] 检查可执行文件...
if not exist "%EXE_FILE%" (
    echo 错误: 未找到可执行文件 %EXE_FILE%
    echo 请先构建项目！
    pause
    exit /b 1
)
echo 找到可执行文件: %EXE_FILE%

echo.
echo [2/6] 检查 windeployqt 工具...
if not exist "%WINDEPLOYQT%" (
    echo 错误: 未找到 windeployqt.exe
    echo 请检查 QT_PATH 设置: %QT_PATH%
    pause
    exit /b 1
)
echo windeployqt 路径: %WINDEPLOYQT%

REM ============================================
REM 执行部署
REM ============================================

echo.
echo [3/6] 运行 windeployqt (部署到 Release 目录)...
"%WINDEPLOYQT%" "%EXE_FILE%" --no-translations

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo 部署失败！
    pause
    exit /b 1
)

REM ============================================
REM 清理根目录
REM ============================================

echo.
echo [4/6] 清理根目录中的Qt文件...
echo 正在清理根目录中的DLL和插件文件夹...

REM 删除根目录中的DLL文件
for %%f in (Qt6*.dll opengl32sw.dll D3Dcompiler_*.dll d3dcompiler_*.dll) do (
    if exist "%PROJECT_DIR%%%f" (
        del /f /q "%PROJECT_DIR%%%f" 2>nul
    )
)

REM 删除根目录中的插件文件夹
for %%d in (generic iconengines imageformats networkinformation platforms styles tls translations) do (
    if exist "%PROJECT_DIR%%%d" (
        rmdir /s /q "%PROJECT_DIR%%%d" 2>nul
    )
)

echo 根目录清理完成！

REM ============================================
REM 复制必要文件
REM ============================================

echo.
echo [5/6] 复制 Python 脚本...
set PYTHON_SRC=%PROJECT_DIR%src\services\inference\python
set PYTHON_DEST=%RELEASE_DIR%\python

if exist "%PYTHON_SRC%" (
    if not exist "%PYTHON_DEST%" mkdir "%PYTHON_DEST%"
    xcopy /Y /I "%PYTHON_SRC%\*.py" "%PYTHON_DEST%\" >nul 2>&1
    if %ERRORLEVEL% == 0 (
        echo Python 脚本已复制到: %PYTHON_DEST%
    ) else (
        echo 警告: 复制 Python 脚本失败
    )
) else (
    echo 警告: 未找到 Python 脚本目录: %PYTHON_SRC%
)

echo.
echo [6/6] 复制模型文件...
set MODELS_SRC=%PROJECT_DIR%src\resources\models
set MODELS_DEST=%RELEASE_DIR%\models

if exist "%MODELS_SRC%" (
    if not exist "%MODELS_DEST%" mkdir "%MODELS_DEST%"
    xcopy /Y /S /I "%MODELS_SRC%\*" "%MODELS_DEST%\" >nul 2>&1
    if %ERRORLEVEL% == 0 (
        echo 模型文件已复制到: %MODELS_DEST%
    ) else (
        echo 警告: 复制模型文件失败
    )
) else (
    echo 警告: 未找到模型文件目录: %MODELS_SRC%
)

REM ============================================
REM 部署完成
REM ============================================

echo.
echo ============================================
echo 部署完成！
echo 部署目录: %RELEASE_DIR%
echo ============================================
echo.
echo 提示: 可执行文件位于: %RELEASE_DIR%\GenPreCVSystem.exe
echo.

REM 成功完成后自动关闭（等待2秒让用户看到结果）
timeout /t 2 /nobreak >nul
exit /b 0
