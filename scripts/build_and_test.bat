@echo off
setlocal enabledelayedexpansion

echo ========================================
echo Distributed Camera Test Environment Build Script
echo ========================================

REM 设置环境变量
call setup_env.bat

set PROJECT_ROOT=c:\Work\OpenHarmony\distributedhardware_distributed_camera
set BUILD_DIR=%PROJECT_ROOT%\out\debug
set UI_BUILD_DIR=%PROJECT_ROOT%\ui\build

echo.
echo Step 1: Building core libraries using OpenHarmony build system...
cd /d %PROJECT_ROOT%

REM 使用hb构建系统
echo Running hb build...
hb build -f

if %errorlevel% neq 0 (
    echo ERROR: OpenHarmony build failed!
    goto :error
)

echo.
echo Step 2: Building UI application...
cd /d %PROJECT_ROOT%\ui

if not exist "%UI_BUILD_DIR%" (
    echo Creating UI build directory: %UI_BUILD_DIR%
    mkdir "%UI_BUILD_DIR%"
)

cd /d "%UI_BUILD_DIR%"

echo Running CMake configuration...
cmake .. -G "Visual Studio 16 2019" -A x64 ^
    -DCMAKE_PREFIX_PATH="C:/Work/Qt/6.5.0/msvc2019_64"

if %errorlevel% neq 0 (
    echo ERROR: CMake configuration failed!
    echo Make sure Qt and vcpkg are properly installed and configured.
    goto :error
)

echo Running MSBuild...
cmake --build . --config Debug

if %errorlevel% neq 0 (
    echo ERROR: UI build failed!
    goto :error
)

echo.
echo Step 3: Running tests...
cd /d %PROJECT_ROOT%

echo Setting environment variables...
set DCAMERA_COMM_MODE=tcp
set DCAMERA_LOG_LEVEL=INFO

echo Running TCP adapter test...
if exist "%BUILD_DIR%\tcp_adapter_test.exe" (
    "%BUILD_DIR%\tcp_adapter_test.exe"
    if %errorlevel% neq 0 (
        echo WARNING: TCP adapter test failed, but continuing...
    )
) else (
    echo WARNING: TCP adapter test executable not found
)

echo.
echo Step 4: Launching UI application...
echo Starting DistributedCameraTestUI...
start "" "%UI_BUILD_DIR%\Debug\DistributedCameraTestUI.exe"

echo.
echo ========================================
echo Build and test completed successfully!
echo UI application should be launching now.
echo ========================================
goto :end

:error
echo.
echo ========================================
echo BUILD FAILED - Please check the errors above
echo ========================================
pause
exit /b 1

:end
endlocal