@echo off
REM ============================================================================
REM AVCodec Mock 源码验证脚本
REM 用途: 验证关键接口在源码中的实现情况，无需编译
REM ============================================================================

setlocal enabledelayedexpansion

set "HEADER_FILE=include\avcodec_mock.h"
set "SOURCE_FILE=src\avcodec_mock.cpp"
set "TEST_FILE=test\avcodec_test.cpp"
set "BASIC_TEST_FILE=test_avcodec_basic.cpp"

echo ========================================
echo AVCodec Mock 源码验证工具
echo ========================================
echo.

set "ALL_FOUND=true"
set "TOTAL_CHECKS=0"
set "PASSED_CHECKS=0"

REM 检查文件是否存在
echo [1/5] 检查文件存在性...
echo ----------------------------------------

for %%F in (%HEADER_FILE% %SOURCE_FILE% %TEST_FILE% %BASIC_TEST_FILE%) do (
    if exist "%%F" (
        echo   [OK] %%F
        set /a PASSED_CHECKS+=1
    ) else (
        echo   [FAIL] %%F - 文件不存在
        set "ALL_FOUND=false"
    )
    set /a TOTAL_CHECKS+=1
)
echo.

if "%ALL_FOUND%"=="false" (
    echo [错误] 缺少必需文件，请检查目录结构
    goto :error_exit
)

REM 检查关键接口在头文件中的声明
echo [2/5] 检查头文件接口声明...
echo ----------------------------------------

set "INTERFACES=CreateByMime Configure Prepare Start CreateInputSurface SetCallback"

for %%I in (%INTERFACES%) do (
    findstr /C:"%%I" %HEADER_FILE% >nul 2>&1
    if !errorlevel! equ 0 (
        echo   [OK] %%I - 头文件已声明
        set /a PASSED_CHECKS+=1
    ) else (
        echo   [FAIL] %%I - 头文件未声明
        set "ALL_FOUND=false"
    )
    set /a TOTAL_CHECKS+=1
)
echo.

REM 检查关键接口在源文件中的实现
echo [3/5] 检查源文件接口实现...
echo ----------------------------------------

set "IMPLEMENTATIONS=CreateByMime Configure Prepare Start CreateInputSurface SetCallback"

for %%I in (%IMPLEMENTATIONS%) do (
    findstr /C:"%%I" %SOURCE_FILE% >nul 2>&1
    if !errorlevel! equ 0 (
        echo   [OK] %%I - 源文件已实现
        set /a PASSED_CHECKS+=1
    ) else (
        echo   [FAIL] %%I - 源文件未实现
        set "ALL_FOUND=false"
    )
    set /a TOTAL_CHECKS+=1
)
echo.

REM 检查关键类定义
echo [4/5] 检查关键类定义...
echo ----------------------------------------

set "CLASSES=AVCodecVideoEncoder AVCodecVideoDecoder VideoEncoderFactory VideoDecoderFactory Format AVBuffer Surface"

for %%C in (%CLASSES%) do (
    findstr /C:"class %%C" %HEADER_FILE% >nul 2>&1
    if !errorlevel! equ 0 (
        echo   [OK] %%C - 类已定义
        set /a PASSED_CHECKS+=1
    ) else (
        echo   [FAIL] %%C - 类未定义
        set "ALL_FOUND=false"
    )
    set /a TOTAL_CHECKS+=1
)
echo.

REM 检查测试覆盖
echo [5/5] 检查测试覆盖...
echo ----------------------------------------

set "TEST_FUNCTIONS=TestVideoEncoderFactory TestVideoEncoderSurface TestVideoEncoderCallback TestVideoEncoderLifecycle"

for %%T in (%TEST_FUNCTIONS%) do (
    findstr /C:"%%T" %TEST_FILE% >nul 2>&1
    if !errorlevel! equ 0 (
        echo   [OK] %%T - 测试已覆盖
        set /a PASSED_CHECKS+=1
    ) else (
        echo   [WARN] %%T - 测试未覆盖
    )
    set /a TOTAL_CHECKS+=1
)
echo.

REM 检查特定关键字实现
echo [额外] 检查特定功能实现...
echo ----------------------------------------

findstr /C:"CreateInputSurface" %SOURCE_FILE% >nul 2>&1
if !errorlevel! equ 0 (
    echo   [OK] CreateInputSurface - Surface模式支持
    set /a PASSED_CHECKS+=1
) else (
    echo   [FAIL] CreateInputSurface - Surface模式不支持
    set "ALL_FOUND=false"
)
set /a TOTAL_CHECKS+=1

findstr /C:"MediaCodecCallback" %HEADER_FILE% >nul 2>&1
if !errorlevel! equ 0 (
    echo   [OK] MediaCodecCallback - 新回调风格支持
    set /a PASSED_CHECKS+=1
) else (
    echo   [FAIL] MediaCodecCallback - 新回调风格不支持
    set "ALL_FOUND=false"
)
set /a TOTAL_CHECKS+=1

findstr /C:"SimulateEncodedOutput" %SOURCE_FILE% >nul 2>&1
if !errorlevel! equ 0 (
    echo   [OK] SimulateEncodedOutput - 测试辅助方法
    set /a PASSED_CHECKS+=1
) else (
    echo   [WARN] SimulateEncodedOutput - 测试辅助方法缺失
)
set /a TOTAL_CHECKS+=1

findstr /C:"video/hevc" %SOURCE_FILE% >nul 2>&1
if !errorlevel! equ 0 (
    echo   [OK] H.265编解码支持
    set /a PASSED_CHECKS+=1
) else (
    echo   [FAIL] H.265编解码支持缺失
    set "ALL_FOUND=false"
)
set /a TOTAL_CHECKS+=1

echo.

REM 统计结果
echo ========================================
echo 验证结果统计
echo ========================================
echo   总检查项: %TOTAL_CHECKS%
echo   通过项:   %PASSED_CHECKS%
set /a FAILED_CHECKS=%TOTAL_CHECK%-%PASSED_CHECKS%
echo   失败项:   %FAILED_CHECKS%
echo.

set /a PERCENT=%PASSED_CHECKS%*100/%TOTAL_CHECKS%
echo   通过率:   %PERCENT%%%
echo.

if "%ALL_FOUND%"=="true" (
    echo ========================================
    echo [SUCCESS] AVCodec Mock 模块验证通过！
    echo ========================================
    echo.
    echo 所有关键接口已实现：
    echo   - VideoEncoderFactory::CreateByMime()
    echo   - Configure()
    echo   - Prepare()
    echo   - Start()
    echo   - CreateInputSurface()
    echo   - SetCallback()
    echo.
    echo 编译和运行测试以验证功能：
    echo   cd build
    echo   cmake -G "MinGW Makefiles" ..
    echo   cmake --build . --target test_avcodec_basic
    echo   .\Debug\test_avcodec_basic.exe
    echo.
    goto :success_exit
) else (
    echo ========================================
    echo [FAILURE] 验证失败，存在缺失项
    echo ========================================
    echo.
    echo 请检查上述失败项并修复。
    goto :error_exit
)

:success_exit
    exit /b 0

:error_exit
    exit /b 1
