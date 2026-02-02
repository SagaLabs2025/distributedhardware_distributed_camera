@echo off
chcp 65001 >nul
echo ============================================
echo   更新 Sink.dll 并运行测试
echo ============================================
echo.

REM 检查应用程序是否正在运行
tasklist /FI "IMAGENAME eq DistributedCameraTest.exe" 2>NUL | find /I /N "DistributedCameraTest.exe">NUL
if "%ERRORLEVEL%"=="0" (
    echo [!] 检测到 DistributedCameraTest.exe 正在运行
    echo [*] 正在关闭应用程序...
    taskkill /F /IM DistributedCameraTest.exe >nul 2>&1
    timeout /t 2 /nobreak >nul
)

echo [*] 复制更新后的 Sink.dll...
copy /Y "..\build_sink\Sink.dll" ".\build_ui\Sink.dll"
if %ERRORLEVEL% NEQ 0 (
    echo [X] 复制失败！
    pause
    exit /b 1
)

echo [√] Sink.dll 更新成功
echo.
echo [*] 启动测试程序...
start "" ".\build_ui\DistributedCameraTest.exe"

echo [√] 测试程序已启动
echo.
timeout /t 3
exit /b 0
