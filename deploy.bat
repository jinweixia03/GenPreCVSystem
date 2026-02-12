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

REM Qt 路径（根据实际情况修改）
set QT_PATH=D:\Environments\CPlusPlus\Qt\6.9.3\msvc2022_64
set WINDEPLOYQT=%QT_PATH%\bin\windeployqt.exe

echo.
echo [1/4] 检查可执行文件...
if not exist "%EXE_FILE%" (
    echo 错误: 未找到可执行文件 %EXE_FILE%
    echo 请先构建项目！
    pause
    exit /b 1
)
echo 找到可执行文件: %EXE_FILE%

echo.
echo [2/4] 检查 windeployqt 工具...
if not exist "%WINDEPLOYQT%" (
    echo 错误: 未找到 windeployqt.exe
    echo 请检查 QT_PATH 设置: %QT_PATH%
    pause
    exit /b 1
)
echo windeployqt 路径: %WINDEPLOYQT%

echo.
echo [3/4] 运行 windeployqt (部署到 Release 目录)...
"%WINDEPLOYQT%" "%EXE_FILE%"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo 部署失败！
    pause
    exit /b 1
)

echo.
echo [4/4] 清理根目录中的Qt文件...
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
echo 部署目录: %RELEASE_DIR%
echo ============================================
echo.
echo 提示: 可执行文件位于: %RELEASE_DIR%\GenPreCVSystem.exe
echo.

REM 成功完成后自动关闭（等待2秒让用户看到结果）
timeout /t 2 /nobreak >nul
exit /b 0
