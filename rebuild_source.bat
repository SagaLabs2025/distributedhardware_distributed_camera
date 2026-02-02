@echo off
setlocal enabledelayedexpansion

echo ============================================
echo   重新编译 Source.dll (修复回调bug)
echo ============================================
echo.

set "CMAKE_CMD=C:\Work\vcpkg\vcpkg\downloads\tools\cmake-3.31.10-windows\cmake-3.31.10-windows-x86_64\bin\cmake.exe"
set "MAKE_CMD=C:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe"
set "GCC_CMD=C:\Qt\Tools\mingw1310_64\bin\gcc.exe"
set "GXX_CMD=C:\Qt\Tools\mingw1310_64\bin\g++.exe"
set "SRC_DIR=C:\Work\OpenHarmony\distributedhardware_distributed_camera\source_module"
set "BUILD_DIR=C:\Work\OpenHarmony\distributedhardware_distributed_camera\build_source"
set "DEST_DIR=C:\Work\OpenHarmony\distributedhardware_distributed_camera\build_ui"

echo [*] Source: %SRC_DIR%
echo [*] Build:  %BUILD_DIR%
echo [*] Dest:   %DEST_DIR%
echo.

REM 删除旧的构建目录
if exist "%BUILD_DIR%" (
    echo [*] 清理旧的构建目录...
    rmdir /S /Q "%BUILD_DIR%"
)

REM 创建构建目录
echo [*] 创建构建目录...
mkdir "%BUILD_DIR%"

REM 配置CMake
echo.
echo [*] 配置 CMake 项目...
"%CMAKE_CMD%" -S "%SRC_DIR%" -B "%BUILD_DIR%" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM="%MAKE_CMD%" -DCMAKE_C_COMPILER="%GCC_CMD%" -DCMAKE_CXX_COMPILER="%GXX_CMD%"
if errorlevel 1 (
    echo [X] CMake 配置失败
    goto :error
)

REM 编译
echo.
echo [*] 编译 Source.dll...
"%MAKE_CMD%" -C "%BUILD_DIR%" -j4
if errorlevel 1 (
    echo [X] 编译失败
    goto :error
)

echo.
echo [√] 编译成功！
echo.

REM 复制到 build_ui
echo [*] 复制到 %DEST_DIR%...
copy /Y "%BUILD_DIR%\libSource.dll" "%DEST_DIR%\Source.dll"
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
