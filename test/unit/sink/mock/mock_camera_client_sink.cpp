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

#include "mock_camera_client_sink.h"
#include "distributed_hardware_log.h"
#include <thread>
#include <chrono>

namespace OHOS {
namespace DistributedHardware {

// ========== MockCameraClientSink ==========

MockCameraClientSink::MockCameraClientSink()
    : sessionState_(SessionState::IDLE)
{
    DHLOGI("[SINK] MockCameraClientSink created");
}

MockCameraClientSink::~MockCameraClientSink()
{
    if (isInitialized_.load()) {
        Release();
    }
    DHLOGI("[SINK] MockCameraClientSink destroyed");
}

int32_t MockCameraClientSink::Init()
{
    if (isInitialized_.load()) {
        DHLOGW("[SINK] MockCameraClientSink already initialized");
        return DCAMERA_OK;
    }

    sessionState_ = SessionState::IDLE;
    isInitialized_.store(true);
    DHLOGI("[SINK] CameraClient initialized SUCCESS");
    return DCAMERA_OK;
}

int32_t MockCameraClientSink::Release()
{
    if (!isInitialized_.load()) {
        return DCAMERA_OK;
    }

    if (isSessionCreated_.load()) {
        ReleaseCaptureSession();
    }

    isInitialized_.store(false);
    DHLOGI("[SINK] CameraClient released");
    return DCAMERA_OK;
}

int32_t MockCameraClientSink::CreateCameraInput(const std::string& cameraId)
{
    if (!isInitialized_.load()) {
        DHLOGE("[SINK] CameraClient not initialized");
        return DCAMERA_BAD_VALUE;
    }

    cameraId_ = cameraId;
    DHLOGI("[SINK] Created CameraInput for camera: %{public}s", cameraId.c_str());
    return DCAMERA_OK;
}

int32_t MockCameraClientSink::CreateCaptureSession()
{
    if (!isInitialized_.load()) {
        DHLOGE("[SINK] CameraClient not initialized");
        return DCAMERA_BAD_VALUE;
    }

    if (isSessionCreated_.load()) {
        DHLOGW("[SINK] CaptureSession already created");
        return DCAMERA_OK;
    }

    isSessionCreated_.store(true);
    sessionState_ = SessionState::IDLE;
    DHLOGI("[SINK] Created CaptureSession SUCCESS");
    return DCAMERA_OK;
}

int32_t MockCameraClientSink::BeginConfig()
{
    if (!isSessionCreated_.load()) {
        DHLOGE("[SINK] CaptureSession not created");
        return DCAMERA_BAD_VALUE;
    }

    std::lock_guard<std::mutex> lock(stateMutex_);
    if (sessionState_ != SessionState::IDLE) {
        DHLOGE("[SINK] Invalid state for BeginConfig: %{public}d",
               static_cast<int>(sessionState_));
        return DCAMERA_WRONG_STATE;
    }

    sessionState_ = SessionState::CONFIGURING;
    DHLOGI("[SINK] CaptureSession BeginConfig SUCCESS");
    return DCAMERA_OK;
}

int32_t MockCameraClientSink::AddInput()
{
    std::lock_guard<std::mutex> lock(stateMutex_);
    if (sessionState_ != SessionState::CONFIGURING) {
        DHLOGE("[SINK] Invalid state for AddInput: %{public}d",
               static_cast<int>(sessionState_));
        return DCAMERA_WRONG_STATE;
    }

    DHLOGI("[SINK] Added CameraInput to CaptureSession");
    return DCAMERA_OK;
}

int32_t MockCameraClientSink::AddOutput(sptr<Surface> surface)
{
    if (surface == nullptr) {
        DHLOGE("[SINK] AddOutput: surface is nullptr");
        return DCAMERA_BAD_VALUE;
    }

    std::lock_guard<std::mutex> lock(stateMutex_);
    if (sessionState_ != SessionState::CONFIGURING) {
        DHLOGE("[SINK] Invalid state for AddOutput: %{public}d",
               static_cast<int>(sessionState_));
        return DCAMERA_WRONG_STATE;
    }

    configuredSurface_ = surface;
    DHLOGI("[SINK] Added PreviewOutput to CaptureSession with Surface");
    return DCAMERA_OK;
}

int32_t MockCameraClientSink::CommitConfig()
{
    std::lock_guard<std::mutex> lock(stateMutex_);
    if (sessionState_ != SessionState::CONFIGURING) {
        DHLOGE("[SINK] Invalid state for CommitConfig: %{public}d",
               static_cast<int>(sessionState_));
        return DCAMERA_WRONG_STATE;
    }

    sessionState_ = SessionState::CONFIGURED;
    DHLOGI("[SINK] CaptureSession configured SUCCESS");
    return DCAMERA_OK;
}

int32_t MockCameraClientSink::Start()
{
    std::lock_guard<std::mutex> lock(stateMutex_);
    if (sessionState_ != SessionState::CONFIGURED) {
        DHLOGE("[SINK] Invalid state for Start: %{public}d",
               static_cast<int>(sessionState_));
        return DCAMERA_WRONG_STATE;
    }

    sessionState_ = SessionState::STARTING;
    DHLOGI("[SINK] Starting CaptureSession...");

    // 模拟启动过程
    sessionState_ = SessionState::RUNNING;
    DHLOGI("[SINK] CaptureSession started SUCCESS");
    return DCAMERA_OK;
}

int32_t MockCameraClientSink::Stop()
{
    std::lock_guard<std::mutex> lock(stateMutex_);
    if (sessionState_ != SessionState::RUNNING) {
        DHLOGW("[SINK] CaptureSession not running, current state: %{public}d",
               static_cast<int>(sessionState_));
        return DCAMERA_OK;
    }

    sessionState_ = SessionState::STOPPING;
    DHLOGI("[SINK] Stopping CaptureSession...");

    sessionState_ = SessionState::CONFIGURED;
    DHLOGI("[SINK] CaptureSession stopped SUCCESS");
    return DCAMERA_OK;
}

int32_t MockCameraClientSink::ReleaseCaptureSession()
{
    if (!isSessionCreated_.load()) {
        return DCAMERA_OK;
    }

    sessionState_ = SessionState::IDLE;
    configuredSurface_ = nullptr;
    isSessionCreated_.store(false);
    DHLOGI("[SINK] CaptureSession released");
    return DCAMERA_OK;
}

void MockCameraClientSink::SimulateYUVDataFlow()
{
    if (sessionState_ != SessionState::RUNNING) {
        DHLOGW("[SINK] Cannot simulate YUV flow, session not running");
        return;
    }

    // 模拟生成YUV数据 (1920x1080 NV12格式)
    constexpr size_t yuvSize = 1920 * 1080 * 3 / 2;
    std::vector<uint8_t> mockYUVData(yuvSize);

    // 填充模拟数据
    for (size_t i = 0; i < yuvSize; ++i) {
        mockYUVData[i] = static_cast<uint8_t>(i % 256);
    }

    DHLOGI("[SINK] Simulated YUV data generated, size: %{public}zu", yuvSize);

    // 调用回调函数
    if (yuvDataCallback_) {
        yuvDataCallback_(mockYUVData.data(), yuvSize);
    }
}

void MockCameraClientSink::SetYUVDataCallback(std::function<void(const uint8_t*, size_t)> callback)
{
    yuvDataCallback_ = callback;
    DHLOGI("[SINK] YUV data callback set");
}

// ========== MockPreviewOutput ==========

MockPreviewOutput::MockPreviewOutput()
{
    DHLOGI("[SINK] MockPreviewOutput created");
}

MockPreviewOutput::~MockPreviewOutput()
{
    if (isStarted_.load()) {
        Stop();
    }
}

int32_t MockPreviewOutput::Start()
{
    if (isStarted_.load()) {
        DHLOGW("[SINK] PreviewOutput already started");
        return DCAMERA_OK;
    }

    isStarted_.store(true);
    DHLOGI("[SINK] PreviewOutput started");
    return DCAMERA_OK;
}

int32_t MockPreviewOutput::Stop()
{
    if (!isStarted_.load()) {
        return DCAMERA_OK;
    }

    isStarted_.store(false);
    DHLOGI("[SINK] PreviewOutput stopped");
    return DCAMERA_OK;
}

void MockPreviewOutput::SetSurface(sptr<Surface> surface)
{
    surface_ = surface;
    DHLOGI("[SINK] PreviewOutput surface set");
}

} // namespace DistributedHardware
} // namespace OHOS
