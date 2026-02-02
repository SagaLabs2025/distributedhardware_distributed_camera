@echo off
setlocal enabledelayedexpansion

echo ============================================
echo   编译 UI 测试程序 (MinGW)
echo ============================================
echo.

set "QMAKE=C:\Qt\6.10.2\mingw_64\bin\qmake.exe"
set "MAKE=C:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe"

set "SRC_DIR=C:\Work\OpenHarmony\distributedhardware_distributed_camera\windows_test_environment"
set "BUILD_DIR=%SRC_DIR%\build_ui_qmake"
set "DEST_DIR=C:\Work\OpenHarmony\distributedhardware_distributed_camera\build_ui"

echo [*] Source: %SRC_DIR%
echo [*] Build:  %BUILD_DIR%
echo [*] Dest:   %DEST_DIR%
echo.

REM 清理旧的构建目录
if exist "%BUILD_DIR%" (
    echo [*] 清理旧的构建目录...
    rmdir /S /Q "%BUILD_DIR%"
)

REM 创建构建目录
echo [*] 创建构建目录...
mkdir "%BUILD_DIR%"

REM 使用 qmake 生成 Makefile
echo.
echo [*] 使用 qmake 生成项目文件...
cd /d "%BUILD_DIR%"
"%QMAKE%" "%SRC_DIR%\windows_test_environment.pro" -o Makefile
if errorlevel 1 (
    echo [X] qmake 失败
    goto :error
)

REM 编译
echo.
echo [*] 编译项目...
"%MAKE%" -j4
if errorlevel 1 (
    echo [X] 编译失败
    goto :error
)

echo.
echo [√] 编译成功！
echo.

REM 复制到目标目录
echo [*] 复制到 %DEST_DIR%...
copy /Y "%BUILD_DIR%\release\DistributedCameraTest.exe" "%DEST_DIR%\DistributedCameraTest.exe"
if errorlevel 1 (
    echo [X] 复制失败
    goto :error
)

echo [√] 完成！
goto :end

:error
echo.
echo [X] 发生错误
pause
exit /b 1

:end
pause
exit /b 0
