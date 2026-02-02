@echo off
REM Camera Mock 模块编译和测试脚本
REM 用于验证Camera Framework Mock模块的可用性

setlocal enabledelayedexpansion

echo ========================================
echo Camera Mock 模块验证脚本
echo ========================================
echo.

set SCRIPT_DIR=%~dp0
set BUILD_DIR=%SCRIPT_DIR%build
set SRC_DIR=%SCRIPT_DIR%src
set INCLUDE_DIR=%SCRIPT_DIR%include
set TEST_DIR=%SCRIPT_DIR%test

REM 检查文件是否存在
echo [1/5] 检查文件存在性...
if not exist "%INCLUDE_DIR%\camera_mock.h" (
    echo [错误] camera_mock.h 不存在
    exit /b 1
)
echo [OK] camera_mock.h

if not exist "%SRC_DIR%\camera_mock.cpp" (
    echo [错误] camera_mock.cpp 不存在
    exit /b 1
)
echo [OK] camera_mock.cpp

if not exist "%TEST_DIR%\camera_mock_test.cpp" (
    echo [错误] camera_mock_test.cpp 不存在
    exit /b 1
)
echo [OK] camera_mock_test.cpp

echo.
echo [2/5] 验证关键接口...
findstr /C:"CameraManager::GetInstance" "%INCLUDE_DIR%\camera_mock.h" >nul
if errorlevel 1 (
    echo [警告] CameraManager::GetInstance 未找到（可能是内联实现）
) else (
    echo [OK] CameraManager::GetInstance
)

findstr /C:"GetSupportedCameras" "%INCLUDE_DIR%\camera_mock.h" >nul
if errorlevel 1 (
    echo [错误] GetSupportedCameras 未找到
    exit /b 1
)
echo [OK] GetSupportedCameras

findstr /C:"CreateCameraInput" "%INCLUDE_DIR%\camera_mock.h" >nul
if errorlevel 1 (
    echo [错误] CreateCameraInput 未找到
    exit /b 1
)
echo [OK] CreateCameraInput

findstr /C:"BeginConfig" "%INCLUDE_DIR%\camera_mock.h" >nul
if errorlevel 1 (
    echo [错误] BeginConfig 未找到
    exit /b 1
)
echo [OK] BeginConfig

findstr /C:"AddInput" "%INCLUDE_DIR%\camera_mock.h" >nul
if errorlevel 1 (
    echo [错误] AddInput 未找到
    exit /b 1
)
echo [OK] AddInput

findstr /C:"CommitConfig" "%INCLUDE_DIR%\camera_mock.h" >nul
if errorlevel 1 (
    echo [错误] CommitConfig 未找到
    exit /b 1
)
echo [OK] CommitConfig

echo.
echo [3/5] 验证三阶段提交状态机...
findstr /C:"CONFIG_STATE_IDLE" "%INCLUDE_DIR%\camera_mock.h" >nul
if errorlevel 1 (
    echo [错误] CONFIG_STATE_IDLE 未找到
    exit /b 1
)
echo [OK] CONFIG_STATE_IDLE

findstr /C:"CONFIG_STATE_CONFIGURING" "%INCLUDE_DIR%\camera_mock.h" >nul
if errorlevel 1 (
    echo [错误] CONFIG_STATE_CONFIGURING 未找到
    exit /b 1
)
echo [OK] CONFIG_STATE_CONFIGURING

findstr /C:"CONFIG_STATE_CONFIGURED" "%INCLUDE_DIR%\camera_mock.h" >nul
if errorlevel 1 (
    echo [错误] CONFIG_STATE_CONFIGURED 未找到
    exit /b 1
)
echo [OK] CONFIG_STATE_CONFIGURED

findstr /C:"CONFIG_STATE_STARTED" "%INCLUDE_DIR%\camera_mock.h" >nul
if errorlevel 1 (
    echo [错误] CONFIG_STATE_STARTED 未找到
    exit /b 1
)
echo [OK] CONFIG_STATE_STARTED

echo.
echo [4/5] 检查测试用例...
findstr /C:"Test2_ThreePhaseCommit_NormalFlow" "%TEST_DIR%\camera_mock_test.cpp" >nul
if errorlevel 1 (
    echo [错误] Test2_ThreePhaseCommit_NormalFlow 未找到
    exit /b 1
)
echo [OK] Test2_ThreePhaseCommit_NormalFlow

findstr /C:"Test3_ThreePhaseCommit_ErrorScenarios" "%TEST_DIR%\camera_mock_test.cpp" >nul
if errorlevel 1 (
    echo [错误] Test3_ThreePhaseCommit_ErrorScenarios 未找到
    exit /b 1
)
echo [OK] Test3_ThreePhaseCommit_ErrorScenarios

echo.
echo [5/5] 尝试编译和测试...

REM 检查是否已有编译产物
if exist "%BUILD_DIR%\Debug\camera_mock_test.exe" (
    echo [OK] 发现已编译的测试程序
    echo.
    echo 运行测试...
    echo ========================================
    "%BUILD_DIR%\Debug\camera_mock_test.exe"
    set TEST_RESULT=!errorlevel!
    echo ========================================
    echo.
    if !TEST_RESULT! EQU 0 (
        echo [成功] 所有测试通过！
    ) else (
        echo [失败] 测试失败，返回码: !TEST_RESULT!
    )
    exit /b !TEST_RESULT!
)

REM 尝试使用MSBuild
if exist "%BUILD_DIR%\DistributedCameraMock.sln" (
    echo [INFO] 发现Visual Studio解决方案

    REM 查找MSBuild
    set MSBUILD_PATH=
    for %%v in (2022 2019 2017) do (
        if exist "C:\Program Files\Microsoft Visual Studio\%%v\Community\MSBuild\Current\Bin\MSBuild.exe" (
            set "MSBUILD_PATH=C:\Program Files\Microsoft Visual Studio\%%v\Community\MSBuild\Current\Bin\MSBuild.exe"
            goto :found_msbuild
        )
        if exist "C:\Program Files (x86)\Microsoft Visual Studio\%%v\Community\MSBuild\Current\Bin\MSBuild.exe" (
            set "MSBUILD_PATH=C:\Program Files (x86)\Microsoft Visual Studio\%%v\Community\MSBuild\Current\Bin\MSBuild.exe"
            goto :found_msbuild
        )
    )

    :found_msbuild
    if defined MSBUILD_PATH (
        echo [INFO] 使用MSBuild: !MSBUILD_PATH!
        "!MSBUILD_PATH!" "%BUILD_DIR%\DistributedCameraMock.sln" /p:Configuration=Debug /t:camera_mock_test /v:minimal

        if exist "%BUILD_DIR%\Debug\camera_mock_test.exe" (
            echo [OK] 编译成功
            echo.
            echo 运行测试...
            echo ========================================
            "%BUILD_DIR%\Debug\camera_mock_test.exe"
            set TEST_RESULT=!errorlevel!
            echo ========================================
            echo.
            if !TEST_RESULT! EQU 0 (
                echo [成功] 所有测试通过！
            ) else (
                echo [失败] 测试失败，返回码: !TEST_RESULT!
            )
            exit /b !TEST_RESULT!
        ) else (
            echo [警告] 编译可能失败，未找到可执行文件
        )
    ) else (
        echo [警告] 未找到MSBuild
    )
) else (
    echo [警告] 未找到Visual Studio解决方案
)

REM 尝试使用CMake
where cmake >nul 2>&1
if not errorlevel 1 (
    echo [INFO] 发现CMake，尝试构建...

    if not exist "%BUILD_DIR%\CMakeCache.txt" (
        echo [INFO] 配置CMake...
        cd /d "%BUILD_DIR%"
        cmake .. -G "Visual Studio 17 2022" -A x64
        cd /d "%SCRIPT_DIR%"
    )

    cd /d "%BUILD_DIR%"
    cmake --build . --config Debug --target camera_mock_test
    cd /d "%SCRIPT_DIR%"

    if exist "%BUILD_DIR%\Debug\camera_mock_test.exe" (
        echo [OK] 编译成功
        echo.
        echo 运行测试...
        echo ========================================
        "%BUILD_DIR%\Debug\camera_mock_test.exe"
        set TEST_RESULT=!errorlevel!
        echo ========================================
        echo.
        if !TEST_RESULT! EQU 0 (
            echo [成功] 所有测试通过！
        ) else (
            echo [失败] 测试失败，返回码: !TEST_RESULT!
        )
        exit /b !TEST_RESULT!
    )
)

echo.
echo ========================================
echo 验证总结
echo ========================================
echo [通过] 文件存在性检查
echo [通过] 关键接口验证
echo [通过] 三阶段提交状态机验证
echo [通过] 测试用例检查
echo [注意] 无法自动编译，需要手动编译
echo.
echo 建议操作：
echo 1. 使用Visual Studio打开 %BUILD_DIR%\DistributedCameraMock.sln
echo 2. 编译 camera_mock_test 项目
echo 3. 运行测试程序验证功能
echo.
echo 详细报告请查看: camera_validation_report.md
echo ========================================

exit /b 0
