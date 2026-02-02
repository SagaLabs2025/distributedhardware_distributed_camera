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

#include "distributed_camera_sink_impl.h"
#include <iostream>
#include <thread>

namespace OHOS {
namespace DistributedHardware {

DistributedCameraSinkImpl::DistributedCameraSinkImpl()
    : mockManager_(std::make_shared<MockModuleManager>()) {
    std::cout << "[SINK] Creating DistributedCameraSinkImpl" << std::endl;
}

DistributedCameraSinkImpl::~DistributedCameraSinkImpl() {
    if (receiving_) {
        StopVideoReceiving();
    }
    if (initialized_) {
        ReleaseSink();
    }
    std::cout << "[SINK] Destroying DistributedCameraSinkImpl" << std::endl;
}

int32_t DistributedCameraSinkImpl::InitSink(const std::string& params,
                                          std::shared_ptr<IDCameraSinkCallback> callback) {
    std::lock_guard<std::mutex> lock(sinkLock_);

    if (initialized_) {
        std::cout << "[SINK] Already initialized" << std::endl;
        return -1;
    }

    callback_ = callback;
    mockManager_->InitializeMockEnvironment();

    initialized_ = true;
    std::cout << "[SINK] InitSink successful" << std::endl;
    return 0;
}

int32_t DistributedCameraSinkImpl::ReleaseSink() {
    std::lock_guard<std::mutex> lock(sinkLock_);

    if (!initialized_) {
        return 0;
    }

    if (receiving_) {
        StopVideoReceiving();
    }

    mockManager_->CleanupMockEnvironment();
    initialized_ = false;
    subscribed_ = false;

    std::cout << "[SINK] ReleaseSink successful" << std::endl;
    return 0;
}

int32_t DistributedCameraSinkImpl::SubscribeLocalHardware(const std::string& dhId,
                                                       const std::string& parameters) {
    std::lock_guard<std::mutex> lock(sinkLock_);

    if (!initialized_) {
        std::cout << "[SINK] Not initialized" << std::endl;
        return -1;
    }

    currentDhId_ = dhId;

    // Start video receiving simulation
    StartVideoReceiving();

    subscribed_ = true;
    std::cout << "[SINK] SubscribeLocalHardware successful: " << dhId << std::endl;
    return 0;
}

int32_t DistributedCameraSinkImpl::UnsubscribeLocalHardware(const std::string& dhId) {
    std::lock_guard<std::mutex> lock(sinkLock_);

    if (!subscribed_) {
        return 0;
    }

    if (currentDhId_ != dhId) {
        std::cout << "[SINK] Device ID mismatch" << std::endl;
        return -1;
    }

    StopVideoReceiving();
    subscribed_ = false;

    std::cout << "[SINK] UnsubscribeLocalHardware successful: " << dhId << std::endl;
    return 0;
}

int32_t DistributedCameraSinkImpl::StopCapture(const std::string& dhId) {
    std::lock_guard<std::mutex> lock(sinkLock_);

    if (!subscribed_ || currentDhId_ != dhId) {
        return -1;
    }

    StopVideoReceiving();
    std::cout << "[SINK] StopCapture successful: " << dhId << std::endl;
    return 0;
}

int32_t DistributedCameraSinkImpl::ChannelNeg(const std::string& dhId, std::string& channelInfo) {
    std::lock_guard<std::mutex> lock(sinkLock_);

    if (!subscribed_ || currentDhId_ != dhId) {
        return -1;
    }

    // Return channel negotiation info
    channelInfo = R"({"channelType":"TCP","port":50000,"format":"H264"})";
    std::cout << "[SINK] ChannelNeg successful" << std::endl;
    return 0;
}

int32_t DistributedCameraSinkImpl::GetCameraInfo(const std::string& dhId, std::string& cameraInfo) {
    std::lock_guard<std::mutex> lock(sinkLock_);

    if (!subscribed_ || currentDhId_ != dhId) {
        return -1;
    }

    // Return camera info
    cameraInfo = R"({"width":1280,"height":720,"fps":30,"format":"H264"})";
    std::cout << "[SINK] GetCameraInfo successful" << std::endl;
    return 0;
}

int32_t DistributedCameraSinkImpl::OpenChannel(const std::string& dhId, std::string& openInfo) {
    std::lock_guard<std::mutex> lock(sinkLock_);

    if (!subscribed_ || currentDhId_ != dhId) {
        return -1;
    }

    // Simulate channel opening
    openInfo = "CHANNEL_OPENED_SUCCESS";
    std::cout << "[SINK] OpenChannel successful" << std::endl;
    return 0;
}

int32_t DistributedCameraSinkImpl::CloseChannel(const std::string& dhId) {
    std::lock_guard<std::mutex> lock(sinkLock_);

    if (!subscribed_ || currentDhId_ != dhId) {
        return -1;
    }

    // Simulate channel closing
    std::cout << "[SINK] CloseChannel successful" << std::endl;
    return 0;
}

void DistributedCameraSinkImpl::StartVideoReceiving() {
    if (receiving_) {
        return;
    }

    // Simulate video receiving
    receiving_ = true;
    std::cout << "[SINK] Video receiving started" << std::endl;

    // Simulate receiving video data
    if (callback_) {
        // Create a dummy video buffer
        auto buffer = std::make_shared<DataBuffer>(1024);
        callback_->OnVideoDataReceived(buffer);

        // Simulate continuous video reception
        std::thread([this]() {
            for (int i = 0; i < 10; i++) {
                std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30fps
                if (!receiving_) break;
                auto dummyBuffer = std::make_shared<DataBuffer>(2048);
                if (callback_) {
                    callback_->OnVideoDataReceived(dummyBuffer);
                }
            }
        }).detach();
    }
}

void DistributedCameraSinkImpl::StopVideoReceiving() {
    if (!receiving_) {
        return;
    }

    receiving_ = false;
    std::cout << "[SINK] Video receiving stopped" << std::endl;
}

} // namespace DistributedHardware
} // namespace OHOS