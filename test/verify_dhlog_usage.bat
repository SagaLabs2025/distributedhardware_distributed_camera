@echo off
REM ============================================================
REM DHLOG使用验证脚本
REM 用途：自动检查测试文件中的DHLOG和LogCapture使用情况
REM ============================================================

setlocal enabledelayedexpansion

set "PROJECT_ROOT=%~dp0..\.."
set "TEST_DIR=%PROJECT_ROOT%\test"
set "REPORT_FILE=%TEST_DIR%\dhlog_verification_result.txt"

echo ============================================================
echo DHLOG使用验证工具
echo ============================================================
echo.

REM 创建报告文件
echo DHLOG使用验证报告 > "%REPORT_FILE%"
echo 生成时间: %date% %time% >> "%REPORT_FILE%"
echo. >> "%REPORT_FILE%"
echo ============================================================ >> "%REPORT_FILE%"
echo. >> "%REPORT_FILE%"

REM 计数器
set /a TOTAL_FILES=0
set /a DHLOG_FILES=0
set /a LOGCAPTURE_FILES=0
set /a ISSUES_FOUND=0

echo [1/5] 检查Source端测试...
echo. >> "%REPORT_FILE%"
echo === Source端测试检查 === >> "%REPORT_FILE%"
echo. >> "%REPORT_FILE%"

set "SOURCE_TEST=%TEST_DIR%\unit\source\test_source_workflow.cpp"
if exist "%SOURCE_TEST%" (
    set /a TOTAL_FILES+=1
    echo 检查文件: test_source_workflow.cpp

    REM 检查DHLOG使用
    findstr /C:"DHLOGI" "%SOURCE_TEST%" >nul 2>&1
    if !errorlevel! equ 0 (
        echo [OK] 找到DHLOGI使用
        echo [OK] test_source_workflow.cpp 使用DHLOGI >> "%REPORT_FILE%"
        set /a DHLOG_FILES+=1
    ) else (
        echo [WARN] 未找到DHLOGI使用
        echo [WARN] test_source_workflow.cpp 未使用DHLOGI >> "%REPORT_FILE%"
        set /a ISSUES_FOUND+=1
    )

    REM 检查LogCapture使用
    findstr /C:"LogCapture" "%SOURCE_TEST%" >nul 2>&1
    if !errorlevel! equ 0 (
        echo [OK] 找到LogCapture使用
        echo [OK] test_source_workflow.cpp 使用LogCapture >> "%REPORT_FILE%"
        set /a LOGCAPTURE_FILES+=1
    ) else (
        echo [WARN] 未找到LogCapture使用
        echo [WARN] test_source_workflow.cpp 未使用LogCapture >> "%REPORT_FILE%"
        set /a ISSUES_FOUND+=1
    )

    REM 检查StartCapture/StopCapture配对
    for /f %%i in ('findstr /C:"StartCapture" "%SOURCE_TEST%" ^| find /c /v ""') do set START_COUNT=%%i
    for /f %%i in ('findstr /C:"StopCapture" "%SOURCE_TEST%" ^| find /c /v ""') do set STOP_COUNT=%%i

    echo StartCapture调用次数: !START_COUNT!
    echo StopCapture调用次数: !STOP_COUNT!
    echo StartCapture: !START_COUNT!, StopCapture: !STOP_COUNT! >> "%REPORT_FILE%"

    if !START_COUNT! equ !STOP_COUNT! (
        echo [OK] StartCapture/StopCapture配对正确
        echo [OK] StartCapture/StopCapture配对正确 >> "%REPORT_FILE%"
    ) else (
        echo [ERROR] StartCapture/StopCapture不匹配
        echo [ERROR] StartCapture/StopCapture不匹配 (!START_COUNT! vs !STOP_COUNT!) >> "%REPORT_FILE%"
        set /a ISSUES_FOUND+=1
    )
) else (
    echo [ERROR] 文件不存在: test_source_workflow.cpp
    echo [ERROR] test_source_workflow.cpp 文件不存在 >> "%REPORT_FILE%"
    set /a ISSUES_FOUND+=1
)

echo.
echo [2/5] 检查Sink端测试...
echo. >> "%REPORT_FILE%"
echo === Sink端测试检查 === >> "%REPORT_FILE%"
echo. >> "%REPORT_FILE%"

set "SINK_TEST=%TEST_DIR%\unit\sink\test_sink_workflow.cpp"
if exist "%SINK_TEST%" (
    set /a TOTAL_FILES+=1
    echo 检查文件: test_sink_workflow.cpp

    REM 检查DHLOG使用
    findstr /C:"DHLOGI" "%SINK_TEST%" >nul 2>&1
    if !errorlevel! equ 0 (
        echo [OK] 找到DHLOGI使用
        echo [OK] test_sink_workflow.cpp 使用DHLOGI >> "%REPORT_FILE%"
        set /a DHLOG_FILES+=1
    ) else (
        echo [WARN] 未找到DHLOGI使用
        echo [WARN] test_sink_workflow.cpp 未使用DHLOGI >> "%REPORT_FILE%"
        set /a ISSUES_FOUND+=1
    )

    REM 检查LogCapture使用
    findstr /C:"LogCapture" "%SINK_TEST%" >nul 2>&1
    if !errorlevel! equ 0 (
        echo [OK] 找到LogCapture使用
        echo [OK] test_sink_workflow.cpp 使用LogCapture >> "%REPORT_FILE%"
        set /a LOGCAPTURE_FILES+=1
    ) else (
        echo [WARN] 未找到LogCapture使用
        echo [WARN] test_sink_workflow.cpp 未使用LogCapture >> "%REPORT_FILE%"
        set /a ISSUES_FOUND+=1
    )

    REM 检查StartCapture/StopCapture配对
    for /f %%i in ('findstr /C:"StartCapture" "%SINK_TEST%" ^| find /c /v ""') do set START_COUNT=%%i
    for /f %%i in ('findstr /C:"StopCapture" "%SINK_TEST%" ^| find /c /v ""') do set STOP_COUNT=%%i

    echo StartCapture调用次数: !START_COUNT!
    echo StopCapture调用次数: !STOP_COUNT!
    echo StartCapture: !START_COUNT!, StopCapture: !STOP_COUNT! >> "%REPORT_FILE%"

    if !START_COUNT! equ !STOP_COUNT! (
        echo [OK] StartCapture/StopCapture配对正确
        echo [OK] StartCapture/StopCapture配对正确 >> "%REPORT_FILE%"
    ) else (
        echo [ERROR] StartCapture/StopCapture不匹配
        echo [ERROR] StartCapture/StopCapture不匹配 (!START_COUNT! vs !STOP_COUNT!) >> "%REPORT_FILE%"
        set /a ISSUES_FOUND+=1
    )
) else (
    echo [ERROR] 文件不存在: test_sink_workflow.cpp
    echo [ERROR] test_sink_workflow.cpp 文件不存在 >> "%REPORT_FILE%"
    set /a ISSUES_FOUND+=1
)

echo.
echo [3/5] 检查集成测试...
echo. >> "%REPORT_FILE%"
echo === 集成测试检查 === >> "%REPORT_FILE%"
echo. >> "%REPORT_FILE%"

set "INTEGRATION_DIR=%TEST_DIR%\integration"
if exist "%INTEGRATION_DIR%" (
    dir /s /b "%INTEGRATION_DIR%\*.cpp" > "%TEMP%\integration_tests.txt"

    for /f "tokens=*" %%f in (%TEMP%\integration_tests.txt) do (
        set /a TOTAL_FILES+=1
        echo 检查文件: %%~nxf

        REM 检查是否使用std::cout
        findstr /C:"std::cout" "%%f" >nul 2>&1
        if !errorlevel! equ 0 (
            echo [WARN] 使用std::cout而非DHLOG
            echo [WARN] %%~nxf 使用std::cout而非DHLOG >> "%REPORT_FILE%"
            set /a ISSUES_FOUND+=1
        )

        REM 检查是否使用LogCapture
        findstr /C:"LogCapture" "%%f" >nul 2>&1
        if !errorlevel! neq 0 (
            echo [WARN] 未使用LogCapture
            echo [WARN] %%~nxf 未使用LogCapture >> "%REPORT_FILE%"
            set /a ISSUES_FOUND+=1
        ) else (
            echo [OK] 使用LogCapture
            echo [OK] %%~nxf 使用LogCapture >> "%REPORT_FILE%"
            set /a LOGCAPTURE_FILES+=1
        )
    )

    del "%TEMP%\integration_tests.txt"
) else (
    echo [INFO] 集成测试目录不存在
    echo [INFO] 集成测试目录不存在 >> "%REPORT_FILE%"
)

echo.
echo [4/5] 检查LogCapture工具...
echo. >> "%REPORT_FILE%"
echo === LogCapture工具检查 === >> "%REPORT_FILE%"
echo. >> "%REPORT_FILE%"

set "LOGCAPTURE_H=%TEST_DIR%\unit\mock\log_capture.h"
set "LOGCAPTURE_CPP=%TEST_DIR%\unit\mock\log_capture.cpp"
set "LOGCAPTURE_TEST=%TEST_DIR%\unit\mock\log_capture_test.cpp"

if exist "%LOGCAPTURE_H%" (
    echo [OK] log_capture.h 存在
    echo [OK] log_capture.h 存在 >> "%REPORT_FILE%"
) else (
    echo [ERROR] log_capture.h 不存在
    echo [ERROR] log_capture.h 不存在 >> "%REPORT_FILE%"
    set /a ISSUES_FOUND+=1
)

if exist "%LOGCAPTURE_CPP%" (
    echo [OK] log_capture.cpp 存在
    echo [OK] log_capture.cpp 存在 >> "%REPORT_FILE%"
) else (
    echo [ERROR] log_capture.cpp 不存在
    echo [ERROR] log_capture.cpp 不存在 >> "%REPORT_FILE%"
    set /a ISSUES_FOUND+=1
)

if exist "%LOGCAPTURE_TEST%" (
    echo [OK] log_capture_test.cpp 存在
    echo [OK] log_capture_test.cpp 存在 >> "%REPORT_FILE%"

    REM 统计测试用例数
    for /f %%i in ('findstr /C:"bool Test" "%LOGCAPTURE_TEST%" ^| find /c /v ""') do set TEST_COUNT=%%i
    echo LogCapture测试用例数: !TEST_COUNT!
    echo LogCapture测试用例数: !TEST_COUNT! >> "%REPORT_FILE%"
) else (
    echo [WARN] log_capture_test.cpp 不存在
    echo [WARN] log_capture_test.cpp 不存在 >> "%REPORT_FILE%"
)

echo.
echo [5/5] 生成统计报告...
echo. >> "%REPORT_FILE%"
echo === 统计汇总 === >> "%REPORT_FILE%"
echo. >> "%REPORT_FILE%"
echo 总检查文件数: %TOTAL_FILES% >> "%REPORT_FILE%"
echo 使用DHLOG的文件数: %DHLOG_FILES% >> "%REPORT_FILE%"
echo 使用LogCapture的文件数: %LOGCAPTURE_FILES% >> "%REPORT_FILE%"
echo 发现的问题数: %ISSUES_FOUND% >> "%REPORT_FILE%"

echo.
echo ============================================================
echo 验证完成
echo ============================================================
echo.
echo 统计结果:
echo   总检查文件数: %TOTAL_FILES%
echo   使用DHLOG的文件数: %DHLOG_FILES%
echo   使用LogCapture的文件数: %LOGCAPTURE_FILES%
echo   发现的问题数: %ISSUES_FOUND%
echo.
echo 详细报告已保存到: %REPORT_FILE%
echo.

if %ISSUES_FOUND% gtr 0 (
    echo [发现 %ISSUES_FOUND% 个问题，需要修复]
    echo 请查看报告文件获取详细信息
) else (
    echo [所有检查通过]
)

echo.
pause
