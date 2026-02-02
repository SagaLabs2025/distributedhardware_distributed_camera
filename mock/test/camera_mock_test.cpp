/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "camera_mock.h"
#include <iostream>
#include <cassert>

using namespace OHOS::CameraStandard;

// ============================================
// 测试回调类
// ============================================

class TestManagerCallback : public ManagerCallback {
public:
    void OnCameraStatusChanged(const std::string& cameraId, int32_t status) override {
        std::cout << "[Callback] Camera " << cameraId << " status changed to " << status << std::endl;
    }
};

class TestSessionCallback : public SessionCallback {
public:
    void OnError(int32_t errorCode) override {
        std::cout << "[Callback] Session error: " << errorCode << std::endl;
    }
};

class TestPreviewCallback : public PreviewOutputCallback {
public:
    void OnFrameStarted() override {
        std::cout << "[Callback] Preview frame started" << std::endl;
    }

    void OnFrameEnded(int32_t frameCount) override {
        std::cout << "[Callback] Preview frame ended, count: " << frameCount << std::endl;
    }
};

// ============================================
// 测试用例
// ============================================

/**
 * 测试1：基本的相机设备获取
 */
void Test1_BasicCameraDiscovery()
{
    std::cout << "\n=== Test 1: Basic Camera Discovery ===" << std::endl;

    auto manager = CameraManager::GetInstance();

    // 添加模拟相机
    manager->AddMockCamera("camera_0");
    manager->AddMockCamera("camera_1");

    // 获取相机列表
    auto cameras = manager->GetSupportedCameras();
    std::cout << "Found " << cameras.size() << " cameras" << std::endl;

    for (const auto& camera : cameras) {
        std::cout << "  - Camera ID: " << camera->GetID() << std::endl;
    }

    assert(cameras.size() == 2 && "Should find 2 cameras");
    std::cout << "Test 1 PASSED" << std::endl;
}

/**
 * 测试2：三阶段提交 - 正常流程
 */
void Test2_ThreePhaseCommit_NormalFlow()
{
    std::cout << "\n=== Test 2: Three-Phase Commit (Normal Flow) ===" << std::endl;

    auto manager = CameraManager::GetInstance();

    // 获取第一个相机
    auto cameras = manager->GetSupportedCameras();
    assert(!cameras.empty() && "No cameras available");

    // 创建相机输入
    std::shared_ptr<CameraInput> cameraInput;
    int32_t ret = manager->CreateCameraInput(cameras[0], &cameraInput);
    assert(ret == CAMERA_OK && "CreateCameraInput failed");
    std::cout << "CameraInput created successfully" << std::endl;

    // 打开相机
    ret = cameraInput->Open();
    assert(ret == CAMERA_OK && "Open failed");
    std::cout << "Camera opened successfully" << std::endl;

    // 创建捕获会话
    auto session = manager->CreateCaptureSession();
    assert(session != nullptr && "CreateCaptureSession failed");
    std::cout << "CaptureSession created successfully" << std::endl;

    // 创建预览输出
    Profile profile(CAMERA_FORMAT_YUV_420, Size(1920, 1080));
    std::shared_ptr<PreviewOutput> previewOutput;
    ret = manager->CreatePreviewOutput(profile, nullptr, &previewOutput);
    assert(ret == CAMERA_OK && "CreatePreviewOutput failed");
    std::cout << "PreviewOutput created successfully" << std::endl;

    // 阶段1: BeginConfig
    ret = session->BeginConfig();
    assert(ret == CAMERA_OK && "BeginConfig failed");
    assert(session->GetConfigState() == CaptureSession::CONFIG_STATE_CONFIGURING);
    std::cout << "BeginConfig successful, state: CONFIGURING" << std::endl;

    // 阶段2: AddInput 和 AddOutput
    ret = session->AddInput(cameraInput);
    assert(ret == CAMERA_OK && "AddInput failed");
    std::cout << "Input added successfully" << std::endl;

    ret = session->AddOutput(previewOutput);
    assert(ret == CAMERA_OK && "AddOutput failed");
    std::cout << "Output added successfully" << std::endl;

    // 阶段3: CommitConfig
    ret = session->CommitConfig();
    assert(ret == CAMERA_OK && "CommitConfig failed");
    assert(session->GetConfigState() == CaptureSession::CONFIG_STATE_CONFIGURED);
    std::cout << "CommitConfig successful, state: CONFIGURED" << std::endl;

    // 启动会话
    ret = session->Start();
    assert(ret == CAMERA_OK && "Start failed");
    assert(session->GetConfigState() == CaptureSession::CONFIG_STATE_STARTED);
    std::cout << "Session started successfully, state: STARTED" << std::endl;

    // 停止会话
    ret = session->Stop();
    assert(ret == CAMERA_OK && "Stop failed");
    std::cout << "Session stopped successfully" << std::endl;

    std::cout << "Test 2 PASSED" << std::endl;
}

/**
 * 测试3：三阶段提交 - 错误场景
 */
void Test3_ThreePhaseCommit_ErrorScenarios()
{
    std::cout << "\n=== Test 3: Three-Phase Commit (Error Scenarios) ===" << std::endl;

    auto manager = CameraManager::GetInstance();
    auto cameras = manager->GetSupportedCameras();

    std::shared_ptr<CameraInput> cameraInput;
    manager->CreateCameraInput(cameras[0], &cameraInput);

    auto session = manager->CreateCaptureSession();

    Profile profile(CAMERA_FORMAT_YUV_420, Size(1920, 1080));
    std::shared_ptr<PreviewOutput> previewOutput;
    manager->CreatePreviewOutput(profile, nullptr, &previewOutput);

    // 测试：在非CONFIGURING状态下调用AddInput
    int32_t ret = session->AddInput(cameraInput);
    assert(ret != CAMERA_OK && "AddInput should fail when not in CONFIGURING state");
    std::cout << "AddInput correctly failed when not in CONFIGURING state" << std::endl;

    // 测试：在非CONFIGURING状态下调用CommitConfig
    ret = session->CommitConfig();
    assert(ret != CAMERA_OK && "CommitConfig should fail when not in CONFIGURING state");
    std::cout << "CommitConfig correctly failed when not in CONFIGURING state" << std::endl;

    // 测试：空参数
    ret = session->AddInput(nullptr);
    assert(ret == CAMERA_INVALID_ARG && "AddInput should fail with null input");
    std::cout << "AddInput correctly failed with null input" << std::endl;

    // 测试：正常的BeginConfig流程
    session->BeginConfig();
    ret = session->AddInput(cameraInput);
    assert(ret == CAMERA_OK);

    // 测试：重复添加Input
    ret = session->AddInput(cameraInput);
    assert(ret == CONFLICT_CAMERA && "Adding same input twice should fail");
    std::cout << "Adding duplicate input correctly failed" << std::endl;

    // 测试：没有输入就CommitConfig
    auto session2 = manager->CreateCaptureSession();
    session2->BeginConfig();
    ret = session2->CommitConfig();
    assert(ret == CAMERA_INVALID_ARG && "CommitConfig should fail without input");
    std::cout << "CommitConfig correctly failed without input" << std::endl;

    std::cout << "Test 3 PASSED" << std::endl;
}

/**
 * 测试4：相机打开/关闭
 */
void Test4_CameraOpenClose()
{
    std::cout << "\n=== Test 4: Camera Open/Close ===" << std::endl;

    auto manager = CameraManager::GetInstance();
    auto cameras = manager->GetSupportedCameras();

    std::shared_ptr<CameraInput> cameraInput;
    manager->CreateCameraInput(cameras[0], &cameraInput);

    // 测试：重复打开
    int32_t ret = cameraInput->Open();
    assert(ret == CAMERA_OK);
    ret = cameraInput->Open();
    assert(ret == CAMERA_BUSY && "Opening same camera twice should fail");
    std::cout << "Duplicate Open correctly failed" << std::endl;

    // 测试：关闭
    ret = cameraInput->Close();
    assert(ret == CAMERA_OK);
    ret = cameraInput->Close();
    assert(ret == CAMERA_CLOSED && "Closing already closed camera should fail");
    std::cout << "Duplicate Close correctly failed" << std::endl;

    std::cout << "Test 4 PASSED" << std::endl;
}

/**
 * 测试5：回调设置
 */
void Test5_Callbacks()
{
    std::cout << "\n=== Test 5: Callbacks ===" << std::endl;

    auto manager = CameraManager::GetInstance();

    // 设置Manager回调
    auto managerCallback = std::make_shared<TestManagerCallback>();
    manager->SetCallback(managerCallback);
    std::cout << "Manager callback set" << std::endl;

    // 创建会话并设置回调
    auto session = manager->CreateCaptureSession();
    auto sessionCallback = std::make_shared<TestSessionCallback>();
    session->SetCallback(sessionCallback);
    std::cout << "Session callback set" << std::endl;

    // 创建预览输出并设置回调
    Profile profile(CAMERA_FORMAT_YUV_420, Size(1920, 1080));
    std::shared_ptr<PreviewOutput> previewOutput;
    manager->CreatePreviewOutput(profile, nullptr, &previewOutput);
    auto previewCallback = std::make_shared<TestPreviewCallback>();
    previewOutput->SetCallback(previewCallback);
    std::cout << "Preview callback set" << std::endl;

    std::cout << "Test 5 PASSED" << std::endl;
}

/**
 * 测试6：使用TestHelper快速设置
 */
void Test6_TestHelperFunctions()
{
    std::cout << "\n=== Test 6: TestHelper Functions ===" << std::endl;

    // 重置状态
    TestHelper::ResetMockState();
    std::cout << "State reset" << std::endl;

    // 添加新的相机
    TestHelper::InitializeMockCameras({"camera_main", "camera_front"});
    std::cout << "Mock cameras initialized" << std::endl;

    // 快速设置完整的相机管道
    bool success = TestHelper::SetupCompleteCameraPipeline(
        "camera_main",
        1920, 1080,
        CAMERA_FORMAT_YUV_420
    );
    assert(success && "SetupCompleteCameraPipeline failed");
    std::cout << "Complete pipeline setup successful" << std::endl;

    // 验证三阶段提交
    success = TestHelper::ValidateThreePhaseCommit("camera_front");
    assert(success && "ValidateThreePhaseCommit failed");
    std::cout << "Three-phase commit validation passed" << std::endl;

    // 打印状态
    TestHelper::PrintMockState();
    std::cout << "State printed" << std::endl;

    std::cout << "Test 6 PASSED" << std::endl;
}

/**
 * 测试7：模拟视频帧输出
 */
void Test7_VideoFrameSimulation()
{
    std::cout << "\n=== Test 7: Video Frame Simulation ===" << std::endl;

    // 设置相机管道
    TestHelper::ResetMockState();
    TestHelper::InitializeMockCameras({"camera_test"});

    bool success = TestHelper::SetupCompleteCameraPipeline(
        "camera_test",
        1280, 720,
        CAMERA_FORMAT_YUV_420
    );
    assert(success && "Setup failed");

    // 模拟视频帧输出
    TestHelper::SimulateVideoFrameOutput(
        "camera_test",
        1280, 720,
        CAMERA_FORMAT_YUV_420,
        10,  // 10 frames
        30   // 30 FPS
    );
    std::cout << "Video frame simulation completed" << std::endl;

    std::cout << "Test 7 PASSED" << std::endl;
}

/**
 * 测试8：多相机并发
 */
void Test8_MultipleCameras()
{
    std::cout << "\n=== Test 8: Multiple Cameras Concurrent ===" << std::endl;

    auto manager = CameraManager::GetInstance();

    // 重置并添加多个相机
    TestHelper::ResetMockState();
    TestHelper::InitializeMockCameras({"cam_0", "cam_1", "cam_2"});

    auto cameras = manager->GetSupportedCameras();
    assert(cameras.size() == 3 && "Should have 3 cameras");

    // 为每个相机创建输入和会话
    for (const auto& camera : cameras) {
        std::shared_ptr<CameraInput> input;
        int32_t ret = manager->CreateCameraInput(camera, &input);
        assert(ret == CAMERA_OK && "CreateCameraInput failed");

        ret = input->Open();
        assert(ret == CAMERA_OK && "Open failed");

        auto session = manager->CreateCaptureSession();

        Profile profile(CAMERA_FORMAT_YUV_420, Size(640, 480));
        std::shared_ptr<PreviewOutput> output;
        manager->CreatePreviewOutput(profile, nullptr, &output);

        session->BeginConfig();
        session->AddInput(input);
        session->AddOutput(output);
        session->CommitConfig();
        session->Start();

        std::cout << "Camera " << camera->GetID() << " started" << std::endl;
    }

    // 打印所有状态
    TestHelper::PrintMockState();

    std::cout << "Test 8 PASSED" << std::endl;
}

// ============================================
// 主函数
// ============================================

int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << "   Camera Framework Mock Test Suite    " << std::endl;
    std::cout << "========================================" << std::endl;

    try {
        Test1_BasicCameraDiscovery();
        Test2_ThreePhaseCommit_NormalFlow();
        Test3_ThreePhaseCommit_ErrorScenarios();
        Test4_CameraOpenClose();
        Test5_Callbacks();
        Test6_TestHelperFunctions();
        Test7_VideoFrameSimulation();
        Test8_MultipleCameras();

        std::cout << "\n========================================" << std::endl;
        std::cout << "   ALL TESTS PASSED!   " << std::endl;
        std::cout << "========================================" << std::endl;

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "\nTEST FAILED with exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "\nTEST FAILED with unknown exception" << std::endl;
        return 1;
    }
}
