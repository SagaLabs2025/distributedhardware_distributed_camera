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

#ifndef OHOS_MOCK_CAMERA_CLIENT_SINK_H
#define OHOS_MOCK_CAMERA_CLIENT_SINK_H

#include <memory>
#include <vector>
#include <string>
#include <atomic>
#include <mutex>

#include "dcamera_capture_info_cmd.h"
#include "surface_mock.h"

namespace OHOS {
namespace DistributedHardware {

/**
 * @brief Mock Sink端Camera客户端类
 *
 * 模拟Camera Framework三阶段提交流程
 */
class MockCameraClientSink {
public:
    /**
     * @brief CaptureSession状态枚举
     */
    enum class SessionState {
        IDLE = 0,
        CONFIGURING = 1,
        CONFIGURED = 2,
        STARTING = 3,
        RUNNING = 4,
        STOPPING = 5,
        ERROR = 6
    };

    MockCameraClientSink();
    ~MockCameraClientSink();

    /**
     * @brief 初始化Camera客户端
     */
    int32_t Init();

    /**
     * @brief 释放Camera客户端
     */
    int32_t Release();

    /**
     * @brief 创建Camera Input
     */
    int32_t CreateCameraInput(const std::string& cameraId);

    /**
     * @brief 创建CaptureSession（三阶段提交的第一阶段）
     */
    int32_t CreateCaptureSession();

    /**
     * @brief BeginConfig - 三阶段提交的第一步
     */
    int32_t BeginConfig();

    /**
     * @brief AddInput - 添加Camera Input
     */
    int32_t AddInput();

    /**
     * @brief AddOutput - 添加Preview Output（绑定AVCodec Surface）
     * @param surface AVCodec编码器的Surface
     */
    int32_t AddOutput(sptr<Surface> surface);

    /**
     * @brief CommitConfig - 三阶段提交的第二步
     */
    int32_t CommitConfig();

    /**
     * @brief Start - 三阶段提交的第三步，开始预览
     */
    int32_t Start();

    /**
     * @brief Stop - 停止预览
     */
    int32_t Stop();

    /**
     * @brief 释放CaptureSession
     */
    int32_t ReleaseCaptureSession();

    /**
     * @brief 获取当前Session状态
     */
    SessionState GetSessionState() const { return sessionState_; }

    /**
     * @brief 模拟YUV数据流
     */
    void SimulateYUVDataFlow();

    /**
     * @brief 设置回调函数（当有YUV数据时调用）
     */
    void SetYUVDataCallback(std::function<void(const uint8_t*, size_t)> callback);

    /**
     * @brief 获取配置的Surface
     */
    sptr<Surface> GetConfiguredSurface() const { return configuredSurface_; }

private:
    std::string cameraId_;
    SessionState sessionState_;
    sptr<Surface> configuredSurface_;
    std::atomic<bool> isInitialized_{false};
    std::atomic<bool> isSessionCreated_{false};
    std::function<void(const uint8_t*, size_t)> yuvDataCallback_;
    mutable std::mutex stateMutex_;
};

/**
 * @brief Mock Preview Output类
 */
class MockPreviewOutput {
public:
    MockPreviewOutput();
    ~MockPreviewOutput();

    int32_t Start();
    int32_t Stop();

    void SetSurface(sptr<Surface> surface);
    sptr<Surface> GetSurface() const { return surface_; }

private:
    sptr<Surface> surface_;
    std::atomic<bool> isStarted_{false};
};

} // namespace DistributedHardware
} // namespace OHOS

#endif // OHOS_MOCK_CAMERA_CLIENT_SINK_H
