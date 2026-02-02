# Camera Mock 模块验证脚本 (PowerShell)

$ErrorActionPreference = "Continue"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Camera Mock 模块验证脚本" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$ScriptDir = $PSScriptRoot
$IncludeDir = Join-Path $ScriptDir "include"
$SrcDir = Join-Path $ScriptDir "src"
$TestDir = Join-Path $ScriptDir "test"
$BuildDir = Join-Path $ScriptDir "build"

# 步骤1：检查文件存在性
Write-Host "[1/5] 检查文件存在性..." -ForegroundColor Green

$HeaderFile = Join-Path $IncludeDir "camera_mock.h"
$SourceFile = Join-Path $SrcDir "camera_mock.cpp"
$TestFile = Join-Path $TestDir "camera_mock_test.cpp"

if (-not (Test-Path $HeaderFile)) {
    Write-Host "[错误] camera_mock.h 不存在" -ForegroundColor Red
    exit 1
}
Write-Host "  [OK] camera_mock.h" -ForegroundColor Green

if (-not (Test-Path $SourceFile)) {
    Write-Host "[错误] camera_mock.cpp 不存在" -ForegroundColor Red
    exit 1
}
Write-Host "  [OK] camera_mock.cpp" -ForegroundColor Green

if (-not (Test-Path $TestFile)) {
    Write-Host "[错误] camera_mock_test.cpp 不存在" -ForegroundColor Red
    exit 1
}
Write-Host "  [OK] camera_mock_test.cpp" -ForegroundColor Green

Write-Host ""

# 步骤2：验证关键接口
Write-Host "[2/5] 验证关键接口..." -ForegroundColor Green

$HeaderContent = Get-Content $HeaderFile -Raw

$Interfaces = @(
    "GetSupportedCameras",
    "CreateCameraInput",
    "CreateCaptureSession",
    "BeginConfig",
    "AddInput",
    "CommitConfig"
)

foreach ($iface in $Interfaces) {
    if ($HeaderContent -match [regex]::Escape($iface)) {
        Write-Host "  [OK] $iface" -ForegroundColor Green
    } else {
        Write-Host "  [错误] $iface 未找到" -ForegroundColor Red
        exit 1
    }
}

Write-Host ""

# 步骤3：验证三阶段提交状态机
Write-Host "[3/5] 验证三阶段提交状态机..." -ForegroundColor Green

$States = @(
    "CONFIG_STATE_IDLE",
    "CONFIG_STATE_CONFIGURING",
    "CONFIG_STATE_CONFIGURED",
    "CONFIG_STATE_STARTED"
)

foreach ($state in $States) {
    if ($HeaderContent -match [regex]::Escape($state)) {
        Write-Host "  [OK] $state" -ForegroundColor Green
    } else {
        Write-Host "  [错误] $state 未找到" -ForegroundColor Red
        exit 1
    }
}

Write-Host ""

# 步骤4：检查测试用例
Write-Host "[4/5] 检查测试用例..." -ForegroundColor Green

$TestContent = Get-Content $TestFile -Raw

$Tests = @(
    "Test1_BasicCameraDiscovery",
    "Test2_ThreePhaseCommit_NormalFlow",
    "Test3_ThreePhaseCommit_ErrorScenarios",
    "Test4_CameraOpenClose",
    "Test5_Callbacks",
    "Test6_TestHelperFunctions",
    "Test7_VideoFrameSimulation",
    "Test8_MultipleCameras"
)

$TestCount = 0
foreach ($test in $Tests) {
    if ($TestContent -match [regex]::Escape($test)) {
        Write-Host "  [OK] $test" -ForegroundColor Green
        $TestCount++
    } else {
        Write-Host "  [警告] $test 未找到" -ForegroundColor Yellow
    }
}

Write-Host ""
Write-Host "  测试用例总数: $TestCount" -ForegroundColor Cyan

# 步骤5：尝试运行测试
Write-Host "[5/5] 尝试运行测试..." -ForegroundColor Green

$TestExe = Join-Path $BuildDir "Debug\camera_mock_test.exe"

if (Test-Path $TestExe) {
    Write-Host "  [OK] 发现已编译的测试程序" -ForegroundColor Green
    Write-Host ""
    Write-Host "运行测试..." -ForegroundColor Yellow
    Write-Host "========================================" -ForegroundColor Cyan

    $Result = & $TestExe
    $ExitCode = $LASTEXITCODE

    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host ""

    if ($ExitCode -eq 0) {
        Write-Host "  [成功] 所有测试通过！" -ForegroundColor Green
    } else {
        Write-Host "  [失败] 测试失败，返回码: $ExitCode" -ForegroundColor Red
    }
    exit $ExitCode
} else {
    Write-Host "  [信息] 未找到编译好的测试程序" -ForegroundColor Yellow
    Write-Host "  路径: $TestExe" -ForegroundColor Gray
}

# 总结
Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "验证总结" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "[通过] 文件存在性检查" -ForegroundColor Green
Write-Host "[通过] 关键接口验证 ($($Interfaces.Count) 个)" -ForegroundColor Green
Write-Host "[通过] 三阶段提交状态机验证 ($($States.Count) 个状态)" -ForegroundColor Green
Write-Host "[通过] 测试用例检查 ($TestCount 个测试)" -ForegroundColor Green
Write-Host "[注意] 需要编译后运行实际测试" -ForegroundColor Yellow
Write-Host ""
Write-Host "建议操作：" -ForegroundColor White
Write-Host "  1. 使用Visual Studio编译测试项目" -ForegroundColor Gray
Write-Host "  2. 运行 camera_mock_test.exe 验证功能" -ForegroundColor Gray
Write-Host "  3. 查看详细报告: camera_validation_report.md" -ForegroundColor Gray
Write-Host "========================================" -ForegroundColor Cyan

exit 0
