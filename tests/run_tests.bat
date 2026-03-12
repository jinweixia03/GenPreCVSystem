@echo off
chcp 65001 >nul
echo ========================================
echo GenPreCVSystem Test Suite
echo ========================================
echo.

set BUILD_DIR=build_test
set RESULT=0

:: 创建构建目录
echo [1/4] Creating build directory...
if not exist %BUILD_DIR% mkdir %BUILD_DIR%

:: 配置 CMake
echo [2/4] Configuring with CMake...
cd %BUILD_DIR%
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=%QT_DIR%
if errorlevel 1 (
    echo ERROR: CMake configuration failed!
    set RESULT=1
    goto :cleanup
)

:: 构建测试
echo [3/4] Building tests...
cmake --build . --config Debug --parallel 4
if errorlevel 1 (
    echo ERROR: Build failed!
    set RESULT=1
    goto :cleanup
)

:: 运行测试
echo [4/4] Running tests...
echo.
Debug\GenPreCVSystemTests.exe
if errorlevel 1 (
    echo.
    echo ========================================
    echo TESTS FAILED
    echo ========================================
    set RESULT=1
) else (
    echo.
    echo ========================================
    echo ALL TESTS PASSED
    echo ========================================
)

:cleanup
cd ..

exit /b %RESULT%
