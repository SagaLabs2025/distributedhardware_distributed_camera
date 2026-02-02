#!/bin/bash
# Camera Mock 快速验证脚本

echo "========================================"
echo "Camera Mock 模块快速验证"
echo "========================================"
echo ""

cd "$(dirname "$0")"

# 统计文件信息
echo "[文件统计]"
echo "  头文件: $(wc -l < include/camera_mock.h) 行"
echo "  实现文件: $(wc -l < src/camera_mock.cpp) 行"
echo "  测试文件: $(wc -l < test/camera_mock_test.cpp) 行"
echo ""

# 检查关键接口
echo "[关键接口检查]"
grep -q "GetSupportedCameras" include/camera_mock.h && echo "  ✓ GetSupportedCameras" || echo "  ✗ GetSupportedCameras 未找到"
grep -q "CreateCameraInput" include/camera_mock.h && echo "  ✓ CreateCameraInput" || echo "  ✗ CreateCameraInput 未找到"
grep -q "CreateCaptureSession" include/camera_mock.h && echo "  ✓ CreateCaptureSession" || echo "  ✗ CreateCaptureSession 未找到"
echo ""

# 检查三阶段提交
echo "[三阶段提交检查]"
grep -q "BeginConfig" include/camera_mock.h && echo "  ✓ BeginConfig" || echo "  ✗ BeginConfig 未找到"
grep -q "AddInput" include/camera_mock.h && echo "  ✓ AddInput" || echo "  ✗ AddInput 未找到"
grep -q "CommitConfig" include/camera_mock.h && echo "  ✓ CommitConfig" || echo "  ✗ CommitConfig 未找到"
echo ""

# 检查状态机
echo "[状态机检查]"
grep -q "CONFIG_STATE_IDLE" include/camera_mock.h && echo "  ✓ CONFIG_STATE_IDLE" || echo "  ✗ CONFIG_STATE_IDLE 未找到"
grep -q "CONFIG_STATE_CONFIGURING" include/camera_mock.h && echo "  ✓ CONFIG_STATE_CONFIGURING" || echo "  ✗ CONFIG_STATE_CONFIGURING 未找到"
grep -q "CONFIG_STATE_CONFIGURED" include/camera_mock.h && echo "  ✓ CONFIG_STATE_CONFIGURED" || echo "  ✗ CONFIG_STATE_CONFIGURED 未找到"
grep -q "CONFIG_STATE_STARTED" include/camera_mock.h && echo "  ✓ CONFIG_STATE_STARTED" || echo "  ✗ CONFIG_STATE_STARTED 未找到"
echo ""

# 统计类定义
echo "[核心类统计]"
CLASS_COUNT=$(grep -c "^class.*{" include/camera_mock.h)
echo "  定义类数量: $CLASS_COUNT"
echo ""

# 统计测试用例
echo "[测试用例统计]"
TEST_COUNT=$(grep -c "void Test.*(" test/camera_mock_test.cpp)
echo "  测试函数数量: $TEST_COUNT"
echo ""

# 检查TestHelper函数
echo "[TestHelper函数]"
grep -q "InitializeMockCameras" src/camera_mock.cpp && echo "  ✓ InitializeMockCameras" || echo "  ✗ InitializeMockCameras 未找到"
grep -q "SetupCompleteCameraPipeline" src/camera_mock.cpp && echo "  ✓ SetupCompleteCameraPipeline" || echo "  ✗ SetupCompleteCameraPipeline 未找到"
grep -q "ValidateThreePhaseCommit" src/camera_mock.cpp && echo "  ✓ ValidateThreePhaseCommit" || echo "  ✗ ValidateThreePhaseCommit 未找到"
echo ""

echo "========================================"
echo "验证完成！"
echo "========================================"
echo ""
echo "详细报告: camera_validation_report.md"
echo ""
