@echo off
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
set DIST_DIR=%PROJECT_DIR%dist\GenPreCVSystem
set EXE_FILE=%RELEASE_DIR%\GenPreCVSystem.exe

REM Qt 路径（根据实际情况修改）
set QT_PATH=D:\Environments\CPlusPlus\Qt\6.9.3\msvc2022_64
set WINDEPLOYQT=%QT_PATH%\bin\windeployqt.exe

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

echo.
echo [3/6] 清理旧的部署文件...
if exist "%DIST_DIR%" rmdir /s /q "%DIST_DIR%"
mkdir "%DIST_DIR%"

echo.
echo [4/6] 复制可执行文件...
copy "%EXE_FILE%" "%DIST_DIR%\"

echo.
echo [5/6] 运行 windeployqt...
"%WINDEPLOYQT%" --dir "%DIST_DIR%" "%EXE_FILE%"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo 部署失败！
    pause
    exit /b 1
)

echo.
echo [6/6] 清理根目录中的Qt文件...
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

echo.
echo ============================================
echo 部署完成！
echo 部署目录: %DIST_DIR%
echo ============================================

REM 计算部署目录大小
for /f "tokens=3" %%a in ('dir /s "%DIST_DIR%" 2^>nul ^| findstr /c:"个文件"') do set SIZE=%%a
echo 部署包大小: %SIZE% 字节

echo.
echo 提示: 可执行文件位于: %DIST_DIR%\GenPreCVSystem.exe
echo.
pause
