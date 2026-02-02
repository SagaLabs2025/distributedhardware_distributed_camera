@echo off
setlocal

echo ============================================
echo   编译 UI 测试程序 (QMake)
echo ============================================
echo.

set QT_PATH=C:\Qt\6.10.2\mingw_64
set MINGW_PATH=C:\Qt\Tools\mingw1310_64

set PATH=%QT_PATH%\bin;%MINGW_PATH%\bin;%PATH%

set SRC_DIR=%~dp0
set BUILD_DIR=%~dp0build_ui_qmake
set DEST_DIR=%~dp0..\build_ui

echo [*] Source: %SRC_DIR%
echo [*] Build:  %BUILD_DIR%
echo [*] Dest:   %DEST_DIR%
echo.

if exist "%BUILD_DIR%" rmdir /S /Q "%BUILD_DIR%"
mkdir "%BUILD_DIR%"

cd /d "%BUILD_DIR%"

echo [*] Running qmake...
"%QT_PATH%\bin\qmake.exe" "%SRC_DIR%\windows_test_environment.pro"
if errorlevel 1 (
    echo [X] qmake failed
    pause
    exit /b 1
)

echo.
echo [*] Running mingw32-make...
"%MINGW_PATH%\bin\mingw32-make.exe"
if errorlevel 1 (
    echo [X] Build failed
    pause
    exit /b 1
)

echo.
echo [*] Copying to %DEST_DIR%...
copy /Y "%BUILD_DIR%\release\DistributedCameraTest.exe" "%DEST_DIR%\DistributedCameraTest.exe"

echo [√] Done!
pause
