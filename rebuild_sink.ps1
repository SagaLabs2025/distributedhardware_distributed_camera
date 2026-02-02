# PowerShell script to rebuild Sink.dll

$ErrorActionPreference = "Stop"

Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  重新编译 Sink.dll" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

# 设置路径
$CMAKE_PATH = "C:\Work\vcpkg\vcpkg\downloads\tools\cmake-3.31.10-windows\cmake-3.31.10-windows-x86_64\bin\cmake.exe"
$SRC_DIR = "C:\Work\OpenHarmony\distributedhardware_distributed_camera\sink_module"
$BUILD_DIR = "C:\Work\OpenHarmony\distributedhardware_distributed_camera\build_sink"
$DEST_DIR = "C:\Work\OpenHarmony\distributedhardware_distributed_camera\build_ui"

# 检查 CMake
if (-Not (Test-Path $CMAKE_PATH)) {
    Write-Host "[X] CMake 不在 $CMAKE_PATH" -ForegroundColor Red
    Write-Host "    请检查路径是否正确" -ForegroundColor Yellow
    pause
    exit 1
}

Write-Host "[*] Source: $SRC_DIR"
Write-Host "[*] Build:  $BUILD_DIR"
Write-Host "[*] Dest:   $DEST_DIR"
Write-Host ""

# 清理旧的构建目录
if (Test-Path $BUILD_DIR) {
    Write-Host "[*] 清理旧的构建目录..." -ForegroundColor Yellow
    Remove-Item -Path $BUILD_DIR -Recurse -Force
}

# 创建构建目录
Write-Host "[*] 创建构建目录..." -ForegroundColor Yellow
New-Item -Path $BUILD_DIR -ItemType Directory -Force | Out-Null

# 配置 CMake
Write-Host ""
Write-Host "[*] 配置 CMake 项目..." -ForegroundColor Yellow
& $CMAKE_PATH -S $SRC_DIR -B $BUILD_DIR -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) {
    Write-Host "[X] CMake 配置失败" -ForegroundColor Red
    pause
    exit 1
}

# 编译
Write-Host ""
Write-Host "[*] 编译 Sink.dll..." -ForegroundColor Yellow
& $CMAKE_PATH --build $BUILD_DIR --config Release -j 4
if ($LASTEXITCODE -ne 0) {
    Write-Host "[X] 编译失败" -ForegroundColor Red
    pause
    exit 1
}

Write-Host ""
Write-Host "[√] 编译成功！" -ForegroundColor Green
Write-Host ""

# 复制到 build_ui
Write-Host "[*] 复制到 $DEST_DIR..." -ForegroundColor Yellow
Copy-Item -Path "$BUILD_DIR\libSink.dll" -Destination "$DEST_DIR\Sink.dll" -Force
if ($LASTEXITCODE -ne 0) {
    Write-Host "[X] 复制失败" -ForegroundColor Red
    pause
    exit 1
}

Write-Host "[√] 完成！" -ForegroundColor Green
Write-Host ""
pause
