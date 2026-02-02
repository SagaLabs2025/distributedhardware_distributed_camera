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

#include "distributed_camera_source_impl.h"
#include "call_tracker.h"
#include <iostream>
#include <thread>

// 定义类名用于调用跟踪
static constexpr const char* CLASS_NAME = "DistributedCameraSourceImpl";

namespace OHOS {
namespace DistributedHardware {

DistributedCameraSourceImpl::DistributedCameraSourceImpl()
    : mockManager_(nullptr) {  // 暂时设置为nullptr，简化测试
    CallTracker::GetInstance().RecordCall(CLASS_NAME, "Constructor", "Create DistributedCameraSourceImpl");
    std::cout << "[SOURCE] >>>>> Creating DistributedCameraSourceImpl <<<<<" << std::endl;
}

DistributedCameraSourceImpl::~DistributedCameraSourceImpl() {
    if (streaming_) {
        StopVideoStreaming();
    }
    if (initialized_) {
        ReleaseSource();
    }
    std::cout << "[SOURCE] Destroying DistributedCameraSourceImpl" << std::endl;
}

int32_t DistributedCameraSourceImpl::InitSource(const std::string& params,
                                              std::shared_ptr<IDCameraSourceCallback> callback) {
    CallTracker::GetInstance().RecordCall(CLASS_NAME, "InitSource", "params=" + params);
    std::cout << "\n[SOURCE] >>>>> InitSource START <<<<<" << std::endl;

    std::lock_guard<std::mutex> lock(sourceLock_);

    if (initialized_) {
        std::cout << "[SOURCE] Already initialized" << std::endl;
        return -1;
    }

    callback_ = callback;
    // mockManager_->InitializeMockEnvironment();  // 暂时注释，简化测试
    videoSource_ = MockVideoSource::GetInstance();

    // Initialize video source with default config
    MockVideoSource::VideoConfig config;
    config.width = 1280;
    config.height = 720;
    config.fps = 30;
    config.format = "H264";

    CallTracker::GetInstance().RecordCall(CLASS_NAME, "InitSource", "Initialize MockVideoSource");
    if (!videoSource_->Initialize(config)) {
        std::cout << "[SOURCE] Failed to initialize video source" << std::endl;
        CallTracker::GetInstance().RecordCall(CLASS_NAME, "InitSource", "FAILED: video source init");
        return -1;
    }

    initialized_ = true;
    std::cout << "[SOURCE] >>>>> InitSource SUCCESS <<<<<\n" << std::endl;
    CallTracker::GetInstance().RecordCall(CLASS_NAME, "InitSource", "SUCCESS");
    return 0;
}

int32_t DistributedCameraSourceImpl::ReleaseSource() {
    CallTracker::GetInstance().RecordCall(CLASS_NAME, "ReleaseSource");
    std::cout << "[SOURCE] >>>>> ReleaseSource <<<<<" << std::endl;

    std::lock_guard<std::mutex> lock(sourceLock_);

    if (!initialized_) {
        return 0;
    }

    if (streaming_) {
        StopVideoStreaming();
    }

    if (videoSource_) {
        videoSource_.reset();
    }

    // mockManager_->CleanupMockEnvironment();  // 暂时注释，简化测试
    initialized_ = false;
    registered_ = false;

    std::cout << "[SOURCE] >>>>> ReleaseSource SUCCESS <<<<<" << std::endl;
    CallTracker::GetInstance().RecordCall(CLASS_NAME, "ReleaseSource", "SUCCESS");
    return 0;
}

int32_t DistributedCameraSourceImpl::RegisterDistributedHardware(const std::string& devId,
                                                              const std::string& dhId,
                                                              const std::string& reqId,
                                                              const std::string& param) {
    std::lock_guard<std::mutex> lock(sourceLock_);

    if (!initialized_) {
        std::cout << "[SOURCE] Not initialized" << std::endl;
        return -1;
    }

    currentDevId_ = devId;
    currentDhId_ = dhId;

    // Start video streaming
    StartVideoStreaming();

    registered_ = true;
    std::cout << "[SOURCE] RegisterDistributedHardware successful: " << dhId << std::endl;
    return 0;
}

int32_t DistributedCameraSourceImpl::UnregisterDistributedHardware(const std::string& devId,
                                                                const std::string& dhId,
                                                                const std::string& reqId) {
    std::lock_guard<std::mutex> lock(sourceLock_);

    if (!registered_) {
        return 0;
    }

    if (currentDevId_ != devId || currentDhId_ != dhId) {
        std::cout << "[SOURCE] Device ID mismatch" << std::endl;
        return -1;
    }

    StopVideoStreaming();
    registered_ = false;

    std::cout << "[SOURCE] UnregisterDistributedHardware successful: " << dhId << std::endl;
    return 0;
}

int32_t DistributedCameraSourceImpl::DCameraNotify(const std::string& devId,
                                                 const std::string& dhId,
                                                 std::string& events) {
    std::lock_guard<std::mutex> lock(sourceLock_);

    if (!registered_) {
        return -1;
    }

    if (currentDevId_ != devId || currentDhId_ != dhId) {
        return -1;
    }

    // Return current events
    events = "STATE_READY";
    std::cout << "[SOURCE] DCameraNotify: " << events << std::endl;
    return 0;
}

void DistributedCameraSourceImpl::StartVideoStreaming() {
    CallTracker::GetInstance().RecordCall(CLASS_NAME, "StartVideoStreaming");
    std::cout << "[SOURCE] >>>>> StartVideoStreaming <<<<<" << std::endl;

    if (streaming_) {
        return;
    }

    if (videoSource_ && videoSource_->IsInitialized()) {
        CallTracker::GetInstance().RecordCall(CLASS_NAME, "StartVideoStreaming", "Call MockVideoSource::StartStreaming");
        videoSource_->StartStreaming();
        streaming_ = true;
        std::cout << "[SOURCE] >>>>> Video streaming STARTED <<<<<" << std::endl;
    }
}

void DistributedCameraSourceImpl::StopVideoStreaming() {
    CallTracker::GetInstance().RecordCall(CLASS_NAME, "StopVideoStreaming");
    std::cout << "[SOURCE] >>>>> StopVideoStreaming <<<<<" << std::endl;

    if (!streaming_) {
        return;
    }

    if (videoSource_) {
        CallTracker::GetInstance().RecordCall(CLASS_NAME, "StopVideoStreaming", "Call MockVideoSource::StopStreaming");
        videoSource_->StopStreaming();
        streaming_ = false;
        std::cout << "[SOURCE] >>>>> Video streaming STOPPED <<<<<" << std::endl;
    }
}

} // namespace DistributedHardware
} // namespace OHOS