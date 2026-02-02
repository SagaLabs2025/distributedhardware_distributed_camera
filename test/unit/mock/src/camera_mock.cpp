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
#include <thread>
#include <chrono>

namespace OHOS {
namespace CameraStandard {

// ============================================
// CameraDevice 实现
// ============================================

CameraDevice::~CameraDevice()
{
}

// ============================================
// CameraInput 实现
// ============================================

CameraInput::~CameraInput()
{
}

// ============================================
// PreviewOutput 实现
// ============================================

PreviewOutput::~PreviewOutput()
{
}

// ============================================
// PhotoOutput 实现
// ============================================

PhotoOutput::~PhotoOutput()
{
}

// ============================================
// CaptureSession 实现
// ============================================

CaptureSession::~CaptureSession()
{
}

// ============================================
// CameraManager 实现
// ============================================

CameraManager::~CameraManager()
{
}

} // namespace CameraStandard
} // namespace OHOS

// ============================================
// 全局辅助函数（用于测试）
// ============================================

namespace OHOS {
namespace CameraStandard {
namespace TestHelper {

/**
 * @brief 初始化Mock相机，添加默认相机设备
 * @param cameraIds 要添加的相机ID列表
 */
void InitializeMockCameras(const std::vector<std::string>& cameraIds)
{
    auto manager = CameraManager::GetInstance();
    manager->ClearMockCameras();
    for (const auto& id : cameraIds) {
        manager->AddMockCamera(id);
    }
}

/**
 * @brief 重置Mock相机状态
 */
void ResetMockState()
{
    auto manager = CameraManager::GetInstance();

    // 停止并释放所有会话
    auto& sessions = manager->GetSessions();
    for (auto& session : sessions) {
        if (session) {
            session->Stop();
            session->Release();
        }
    }
    manager->ClearSessions();

    // 清除所有活跃输入
    manager->ClearActiveInputs();
}

/**
 * @brief 模拟视频帧输出到Surface
 * @param cameraId 相机ID
 * @param width 帧宽度
 * @param height 帧高度
 * @param format 帧格式
 * @param frameCount 要输出的帧数
 * @param fps 帧率
 */
void SimulateVideoFrameOutput(const std::string& cameraId,
                               int32_t width,
                               int32_t height,
                               CameraFormat format,
                               int32_t frameCount,
                               int32_t fps)
{
    auto manager = CameraManager::GetInstance();
    auto input = manager->GetActiveInput(cameraId);
    if (!input || !input->IsOpened()) {
        std::cerr << "[Mock] Camera " << cameraId << " is not opened" << std::endl;
        return;
    }

    auto& sessions = manager->GetSessions();
    for (auto& session : sessions) {
        if (session && session->GetInput() == input) {
            if (session->GetConfigState() != CaptureSession::CONFIG_STATE_STARTED) {
                std::cerr << "[Mock] Session is not started for camera " << cameraId << std::endl;
                return;
            }

            const auto& outputs = session->GetOutputs();
            if (outputs.empty()) {
                std::cerr << "[Mock] No outputs configured for camera " << cameraId << std::endl;
                return;
            }

            std::cout << "[Mock] Simulating " << frameCount << " frames at " << fps << " FPS "
                      << "for camera " << cameraId << " (" << width << "x" << height << ")" << std::endl;

            int32_t frameDelay = 1000 / fps; // 毫秒

            for (int32_t i = 0; i < frameCount; ++i) {
                for (auto& output : outputs) {
                    if (output && output->IsStarted()) {
                        // 这里可以触发实际的Surface数据写入
                        // 在实际使用中，可以通过回调通知测试代码
                        std::cout << "[Mock] Frame " << (i + 1) << "/" << frameCount
                                  << " sent to output" << std::endl;
                    }
                }

                // 模拟帧间隔
                if (fps > 0 && i < frameCount - 1) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(frameDelay));
                }
            }

            std::cout << "[Mock] Frame simulation complete for camera " << cameraId << std::endl;
            return;
        }
    }

    std::cerr << "[Mock] No active session found for camera " << cameraId << std::endl;
}

/**
 * @brief 创建一个完整的相机配置（用于快速测试）
 * @param cameraId 相机ID
 * @param width 视频宽度
 * @param height 视频高度
 * @param format 视频格式
 * @return 成功返回true
 */
bool SetupCompleteCameraPipeline(const std::string& cameraId,
                                  int32_t width,
                                  int32_t height,
                                  CameraFormat format)
{
    auto manager = CameraManager::GetInstance();

    // 1. 创建相机设备
    auto cameras = manager->GetSupportedCameras();
    std::shared_ptr<CameraDevice> targetDevice;
    for (auto& camera : cameras) {
        if (camera && camera->GetID() == cameraId) {
            targetDevice = camera;
            break;
        }
    }

    if (!targetDevice) {
        std::cerr << "[Mock] Camera " << cameraId << " not found" << std::endl;
        return false;
    }

    // 2. 创建相机输入
    std::shared_ptr<CameraInput> cameraInput;
    int32_t ret = manager->CreateCameraInput(targetDevice, &cameraInput);
    if (ret != CAMERA_OK) {
        std::cerr << "[Mock] Failed to create camera input: " << ret << std::endl;
        return false;
    }

    // 3. 打开相机
    ret = cameraInput->Open();
    if (ret != CAMERA_OK) {
        std::cerr << "[Mock] Failed to open camera: " << ret << std::endl;
        return false;
    }

    // 4. 创建捕获会话
    auto session = manager->CreateCaptureSession();
    if (!session) {
        std::cerr << "[Mock] Failed to create capture session" << std::endl;
        return false;
    }

    // 5. 创建预览输出（使用空的Surface，仅用于测试）
    Profile profile(format, Size(width, height));
    std::shared_ptr<PreviewOutput> previewOutput;
    ret = manager->CreatePreviewOutput(profile, nullptr, &previewOutput);
    if (ret != CAMERA_OK) {
        std::cerr << "[Mock] Failed to create preview output: " << ret << std::endl;
        return false;
    }

    // 6. 三阶段提交配置
    // 阶段1: BeginConfig
    ret = session->BeginConfig();
    if (ret != CAMERA_OK) {
        std::cerr << "[Mock] BeginConfig failed: " << ret << std::endl;
        return false;
    }

    // 阶段2: AddInput 和 AddOutput
    ret = session->AddInput(cameraInput);
    if (ret != CAMERA_OK) {
        std::cerr << "[Mock] AddInput failed: " << ret << std::endl;
        return false;
    }

    ret = session->AddOutput(previewOutput);
    if (ret != CAMERA_OK) {
        std::cerr << "[Mock] AddOutput failed: " << ret << std::endl;
        return false;
    }

    // 阶段3: CommitConfig
    ret = session->CommitConfig();
    if (ret != CAMERA_OK) {
        std::cerr << "[Mock] CommitConfig failed: " << ret << std::endl;
        return false;
    }

    // 7. 启动会话
    ret = session->Start();
    if (ret != CAMERA_OK) {
        std::cerr << "[Mock] Start failed: " << ret << std::endl;
        return false;
    }

    std::cout << "[Mock] Complete camera pipeline setup successfully for camera " << cameraId << std::endl;
    return true;
}

/**
 * @brief 验证三阶段提交的状态机
 * @param cameraId 相机ID
 * @return 验证成功返回true
 */
bool ValidateThreePhaseCommit(const std::string& cameraId)
{
    auto manager = CameraManager::GetInstance();

    // 创建基本的相机输入和会话
    auto cameras = manager->GetSupportedCameras();
    if (cameras.empty()) {
        std::cerr << "[Mock] No cameras available for validation" << std::endl;
        return false;
    }

    std::shared_ptr<CameraInput> cameraInput;
    int32_t ret = manager->CreateCameraInput(cameras[0], &cameraInput);
    if (ret != CAMERA_OK) {
        return false;
    }

    auto session = manager->CreateCaptureSession();
    if (!session) {
        return false;
    }

    // 验证初始状态
    if (session->GetConfigState() != CaptureSession::CONFIG_STATE_IDLE) {
        std::cerr << "[Mock] Initial state is not IDLE" << std::endl;
        return false;
    }

    // 验证BeginConfig后的状态
    ret = session->BeginConfig();
    if (ret != CAMERA_OK) {
        std::cerr << "[Mock] BeginConfig failed" << std::endl;
        return false;
    }

    if (session->GetConfigState() != CaptureSession::CONFIG_STATE_CONFIGURING) {
        std::cerr << "[Mock] State after BeginConfig is not CONFIGURING" << std::endl;
        return false;
    }

    // 验证AddInput后的状态（仍应是CONFIGURING）
    ret = session->AddInput(cameraInput);
    if (ret != CAMERA_OK) {
        std::cerr << "[Mock] AddInput failed" << std::endl;
        return false;
    }

    if (session->GetConfigState() != CaptureSession::CONFIG_STATE_CONFIGURING) {
        std::cerr << "[Mock] State after AddInput is not CONFIGURING" << std::endl;
        return false;
    }

    // 验证CommitConfig后的状态
    ret = session->CommitConfig();
    if (ret != CAMERA_OK) {
        std::cerr << "[Mock] CommitConfig failed" << std::endl;
        return false;
    }

    if (session->GetConfigState() != CaptureSession::CONFIG_STATE_CONFIGURED) {
        std::cerr << "[Mock] State after CommitConfig is not CONFIGURED" << std::endl;
        return false;
    }

    // 验证Start后的状态
    ret = session->Start();
    if (ret != CAMERA_OK) {
        std::cerr << "[Mock] Start failed" << std::endl;
        return false;
    }

    if (session->GetConfigState() != CaptureSession::CONFIG_STATE_STARTED) {
        std::cerr << "[Mock] State after Start is not STARTED" << std::endl;
        return false;
    }

    std::cout << "[Mock] Three-phase commit validation passed" << std::endl;
    return true;
}

/**
 * @brief 打印Mock相机的当前状态
 * @param cameraId 相机ID（可选，为空则打印所有）
 */
void PrintMockState(const std::string& cameraId = "")
{
    auto manager = CameraManager::GetInstance();

    std::cout << "=== Mock Camera State ===" << std::endl;

    if (!cameraId.empty()) {
        auto input = manager->GetActiveInput(cameraId);
        std::cout << "Camera: " << cameraId << std::endl;
        if (input) {
            std::cout << "  Status: " << (input->IsOpened() ? "OPENED" : "CLOSED") << std::endl;
        } else {
            std::cout << "  Status: NOT ACTIVE" << std::endl;
        }
    } else {
        // 打印所有会话信息
        const auto& sessions = manager->GetSessions();
        std::cout << "Total Sessions: " << sessions.size() << std::endl;

        for (size_t i = 0; i < sessions.size(); ++i) {
            auto session = sessions[i];
            if (session) {
                std::cout << "  Session " << i << ": ";
                switch (session->GetConfigState()) {
                    case CaptureSession::CONFIG_STATE_IDLE:
                        std::cout << "IDLE";
                        break;
                    case CaptureSession::CONFIG_STATE_CONFIGURING:
                        std::cout << "CONFIGURING";
                        break;
                    case CaptureSession::CONFIG_STATE_CONFIGURED:
                        std::cout << "CONFIGURED";
                        break;
                    case CaptureSession::CONFIG_STATE_STARTED:
                        std::cout << "STARTED";
                        break;
                }
                std::cout << std::endl;

                auto input = session->GetInput();
                if (input) {
                    std::cout << "    Input: " << input->GetCameraId()
                              << " (" << (input->IsOpened() ? "OPEN" : "CLOSED") << ")" << std::endl;
                }

                const auto& outputs = session->GetOutputs();
                std::cout << "    Outputs: " << outputs.size() << std::endl;
            }
        }
    }

    std::cout << "========================" << std::endl;
}

} // namespace TestHelper
} // namespace CameraStandard
} // namespace OHOS
