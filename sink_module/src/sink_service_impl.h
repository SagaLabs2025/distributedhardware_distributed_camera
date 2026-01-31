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

#ifndef SINK_SERVICE_IMPL_H
#define SINK_SERVICE_IMPL_H

#include "distributed_camera_sink.h"
#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>

// 前向声明
class YUVFileCamera;
class FFmpegEncoderWrapper;
class SocketSender;

class SinkServiceImpl : public IDistributedCameraSink {
public:
    SinkServiceImpl();
    ~SinkServiceImpl() override;

    // IDistributedCameraSink 实现
    int32_t InitSink(const std::string& params, ISinkCallback* callback) override;
    int32_t ReleaseSink() override;

    // 【内部方法】由Socket接收消息后调用
    int32_t OnStartCaptureMessage(const std::string& dhId, int width, int height);
    int32_t OnStopCaptureMessage(const std::string& dhId);

private:
    void StartSinkThread();
    void StopSinkThread();
    void SinkThreadProc();

    ISinkCallback* callback_;
    std::unique_ptr<YUVFileCamera> yuvCamera_;
    std::unique_ptr<FFmpegEncoderWrapper> encoder_;
    std::unique_ptr<SocketSender> socketSender_;

    std::thread sinkThread_;
    std::mutex mutex_;
    std::atomic<bool> running_;
    std::atomic<bool> initialized_;
};

#endif // SINK_SERVICE_IMPL_H
