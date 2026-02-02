/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "sink_service_impl.h"
#include "yuv_file_camera.h"
#include "ffmpeg_encoder_wrapper.h"
#include "socket_sender.h"

#include "distributed_hardware_log.h"

SinkServiceImpl::SinkServiceImpl()
    : callback_(nullptr), running_(false), initialized_(false) {
    DHLOGI("[SINK_IMPL] SinkServiceImpl created");
}

SinkServiceImpl::~SinkServiceImpl() {
    ReleaseSink();
    DHLOGI("[SINK_IMPL] SinkServiceImpl destroyed");
}

int32_t SinkServiceImpl::InitSink(const std::string& params, ISinkCallback* callback) {
    DHLOGI("[SINK_IMPL] InitSink called, params: %{public}s", params.c_str());

    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_) {
        DHLOGE("[SINK_IMPL] Already initialized");
        return -1;
    }

    callback_ = callback;

    // 创建组件
    yuvCamera_ = std::make_unique<YUVFileCamera>();
    encoder_ = std::make_unique<FFmpegEncoderWrapper>();
    socketSender_ = std::make_unique<SocketSender>();

    // 初始化组件
    if (yuvCamera_->Initialize() != 0) {
        DHLOGE("[SINK_IMPL] Failed to initialize YUV camera");
        return -1;
    }

    // 初始化FFmpeg编码器
    if (encoder_->Initialize(1920, 1080) != 0) {
        DHLOGE("[SINK_IMPL] Failed to initialize encoder, will send raw YUV data");
        // 不返回错误，继续运行但发送原始数据
        encoder_.reset();
    }

    if (socketSender_->Initialize() != 0) {
        DHLOGE("[SINK_IMPL] Failed to initialize socket sender");
        return -1;
    }

    initialized_ = true;
    DHLOGI("[SINK_IMPL] InitSink success (FFmpeg encoder disabled temporarily)");
    return 0;
}

int32_t SinkServiceImpl::ReleaseSink() {
    DHLOGI("[SINK_IMPL] ReleaseSink called");

    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_) {
        DHLOGW("[SINK_IMPL] Not initialized");
        return 0;
    }

    StopSinkThread();

    // encoder_.reset();  // 暂时禁用
    yuvCamera_.reset();
    socketSender_.reset();

    initialized_ = false;
    DHLOGI("[SINK_IMPL] ReleaseSink success");
    return 0;
}

int32_t SinkServiceImpl::StartCapture(const std::string& dhId, int width, int height) {
    DHLOGI("[SINK_IMPL] StartCapture called (test environment), dhId: %{public}s, width: %{public}d, height: %{public}d",
            dhId.c_str(), width, height);

    // 测试环境直接调用内部方法
    return OnStartCaptureMessage(dhId, width, height);
}

int32_t SinkServiceImpl::StopCapture(const std::string& dhId) {
    DHLOGI("[SINK_IMPL] StopCapture called (test environment), dhId: %{public}s", dhId.c_str());

    // 测试环境直接调用内部方法
    return OnStopCaptureMessage(dhId);
}

int32_t SinkServiceImpl::OnStartCaptureMessage(const std::string& dhId, int width, int height) {
    DHLOGI("[SINK_IMPL] OnStartCaptureMessage called, dhId: %{public}s, width: %{public}d, height: %{public}d",
            dhId.c_str(), width, height);

    // 这里将触发原始代码的 DHLOG 输出
    // 对应 dcamera_sink_data_process.cpp:72
    // DHLOGI("StartCapture dhId: %{public}s, width: %{public}d, height: %{public}d...")

    if (!initialized_) {
        DHLOGE("[SINK_IMPL] Not initialized");
        return -1;
    }

    // 启动Sink线程
    StartSinkThread();

    DHLOGI("[SINK_IMPL] OnStartCaptureMessage success");
    return 0;
}

int32_t SinkServiceImpl::OnStopCaptureMessage(const std::string& dhId) {
    DHLOGI("[SINK_IMPL] OnStopCaptureMessage called, dhId: %{public}s", dhId.c_str());
    StopSinkThread();
    return 0;
}

void SinkServiceImpl::StartSinkThread() {
    if (running_) {
        DHLOGW("[SINK_IMPL] Sink thread already running");
        return;
    }

    DHLOGI("[SINK_IMPL] Attempting to connect to Source at 127.0.0.1:8888...");

    // 连接到Source端 (使用默认端口8888)
    int connectResult = socketSender_->ConnectToSource("127.0.0.1", 8888);
    if (connectResult != 0) {
        DHLOGE("[SINK_IMPL] Failed to connect to Source (error: %d), retrying...", connectResult);
        // 给Source端一些时间启动服务器
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        connectResult = socketSender_->ConnectToSource("127.0.0.1", 8888);
        if (connectResult != 0) {
            DHLOGE("[SINK_IMPL] Still cannot connect to Source (error: %d)", connectResult);
            // 不要return，尝试继续（可能有其他问题）
        }
    }

    // 检查连接状态
    if (socketSender_->IsConnected()) {
        DHLOGI("[SINK_IMPL] Connected to Source successfully");
    } else {
        DHLOGW("[SINK_IMPL] Not connected, but will try to send data anyway");
    }

    running_ = true;
    sinkThread_ = std::thread(&SinkServiceImpl::SinkThreadProc, this);
    DHLOGI("[SINK_IMPL] Sink thread started");
}

void SinkServiceImpl::StopSinkThread() {
    if (!running_) {
        return;
    }

    running_ = false;
    if (sinkThread_.joinable()) {
        sinkThread_.join();
    }
    DHLOGI("[SINK_IMPL] Sink thread stopped");
}

void SinkServiceImpl::SinkThreadProc() {
    DHLOGI("[SINK_IMPL] Sink thread proc started");

    while (running_.load()) {
        // 从YUV文件读取帧
        std::vector<uint8_t> yuvData;
        if (yuvCamera_->ReadFrame(yuvData) != 0) {
            DHLOGE("[SINK_IMPL] Failed to read YUV frame");
            break;
        }

        // 编码为H.265 (如果编码器可用)
        std::vector<uint8_t> encodedData;
        if (encoder_) {
            if (encoder_->Encode(yuvData, encodedData) != 0) {
                DHLOGE("[SINK_IMPL] Failed to encode frame");
                break;
            }
        } else {
            // 编码器被禁用，直接发送原始YUV数据
            DHLOGI("[SINK_IMPL] Encoder disabled, sending raw YUV data");
            encodedData = yuvData;
        }

        // 发送给Source端
        if (socketSender_->SendData(encodedData) != 0) {
            DHLOGE("[SINK_IMPL] Failed to send encoded data");
            break;
        }
    }

    DHLOGI("[SINK_IMPL] Sink thread proc ended");
}

// ===== 导出工厂函数 =====

extern "C" __declspec(dllexport)
void* CreateSinkService() {
    DHLOGI("[SINK_IMPL] CreateSinkService called");
    return new SinkServiceImpl();
}

extern "C" __declspec(dllexport)
void DestroySinkService(void* instance) {
    DHLOGI("[SINK_IMPL] DestroySinkService called");
    delete static_cast<SinkServiceImpl*>(instance);
}

extern "C" __declspec(dllexport)
const char* GetSinkVersion() {
    static const char* version = "1.0.0";
    DHLOGI("[SINK_IMPL] GetSinkVersion called: %{public}s", version);
    return version;
}
