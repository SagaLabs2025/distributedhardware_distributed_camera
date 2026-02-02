@echo off
REM Sink端工作流测试执行脚本 (Windows)

setlocal enabledelayedexpansion

echo ========================================
echo Sink端工作流测试套件 - Windows
echo ========================================
echo.

REM 设置颜色
set GREEN=[92m
set RED=[91m
set YELLOW=[93m
set RESET=[0m

REM 检查CMake
where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo %RED%错误: 未找到CMake，请先安装CMake%RESET%
    exit /b 1
)

REM 创建构建目录
if not exist build mkdir build
cd build

echo %YELLOW%[1/4] 配置项目...%RESET%
cmake .. -G "Ninja" || cmake ..
if %ERRORLEVEL% NEQ 0 (
    echo %RED%错误: CMake配置失败%RESET%
    cd ..
    exit /b 1
)

echo.
echo %YELLOW%[2/4] 编译测试...%RESET%
cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo %RED%错误: 编译失败%RESET%
    cd ..
    exit /b 1
)

echo.
echo %YELLOW%[3/4] 运行测试...%RESET%
Release\test_sink_workflow.exe --gtest_output=xml:sink_test_results.xml
if %ERRORLEVEL% NEQ 0 (
    echo %RED%错误: 测试执行失败%RESET%
    cd ..
    exit /b 1
)

echo.
echo %GREEN%[4/4] 测试完成！%RESET%
echo.
echo 测试结果文件: build\sink_test_results.xml
echo.

REM 检查测试结果
findstr /C:"[  PASSED  ]" sink_test_results.xml >nul
if %ERRORLEVEL% EQU 0 (
    echo %GREEN%所有测试通过！%RESET%
) else (
    echo %YELLOW%存在失败的测试用例，请查看测试报告%RESET%
)

cd ..
echo.
echo ========================================
echo 测试执行完成
echo ========================================

endlocal
