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

#ifndef SOCKET_ADAPTER_H
#define SOCKET_ADAPTER_H

#include "distributed_camera_source.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>

/**
 * Socket适配器
 * 模拟 SoftBus 三通道通信:
 * - Control通道: 相机命令、配置、元数据协商
 * - Snapshot通道: 高分辨率静态图片 (JPEG)
 * - Continuous通道: 实时视频流 (H.265)
 */
class SocketAdapter {
public:
    // 数据回调函数类型
    using DataCallback = std::function<void(const uint8_t*, int)>;

    SocketAdapter();
    ~SocketAdapter();

    // 初始化
    int32_t Initialize();

    // 连接到Sink端
    int32_t ConnectToSink();

    // 发送配置消息 (Control通道)
    int32_t SendConfigureMessage(const std::vector<DCStreamInfo>& streamInfos);

    // 发送启动捕获消息 (Control通道)
    int32_t SendStartCaptureMessage(const std::vector<DCCaptureInfo>& captureInfos);

    // 发送停止捕获消息 (Control通道)
    int32_t SendStopCaptureMessage();

    // 开始接收数据 (Continuous通道)
    int32_t StartReceiving();

    // 停止接收
    void StopReceiving();

    // 设置数据回调
    void SetDataCallback(DataCallback callback) {
        dataCallback_ = callback;
    }

    // 是否已连接
    bool IsConnected() const { return connected_.load(); }

private:
    void ReceiveThreadProc();
    int32_t SendControlMessage(const std::string& message);
    int32_t ReceiveData();
    std::string SerializeStreamInfos(const std::vector<DCStreamInfo>& streamInfos);
    std::string SerializeCaptureInfos(const std::vector<DCCaptureInfo>& captureInfos);

    int sockfd_;
    std::atomic<bool> connected_;
    std::atomic<bool> receiving_;
    std::atomic<bool> initialized_;

    std::thread receiveThread_;
    std::mutex mutex_;

    DataCallback dataCallback_;

    static const int DEFAULT_PORT = 8888;
    static const char* DEFAULT_HOST;
};

#endif // SOCKET_ADAPTER_H
