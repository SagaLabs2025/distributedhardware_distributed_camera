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

#include "source_service_impl.h"
#include "socket_receiver.h"
#include "distributed_hardware_log.h"

SourceServiceImpl::SourceServiceImpl()
    : callback_(nullptr), initialized_(false), capturing_(false) {
    DHLOGI("[SOURCE_IMPL] SourceServiceImpl created");
}

SourceServiceImpl::~SourceServiceImpl() {
    ReleaseSource();
    DHLOGI("[SOURCE_IMPL] SourceServiceImpl destroyed");
}

int32_t SourceServiceImpl::InitSource(const std::string& params, ISourceCallback* callback) {
    DHLOGI("[SOURCE_IMPL] InitSource called, params: %{public}s", params.c_str());

    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_) {
        DHLOGE("[SOURCE_IMPL] Already initialized");
        return -1;
    }

    callback_ = callback;

    // 创建Socket接收器
    socketReceiver_ = std::make_unique<SocketReceiver>();
    if (socketReceiver_->Initialize(callback_) != 0) {
        DHLOGE("[SOURCE_IMPL] Failed to initialize socket receiver");
        return -1;
    }

    initialized_ = true;
    DHLOGI("[SOURCE_IMPL] InitSource success");
    return 0;
}

int32_t SourceServiceImpl::ReleaseSource() {
    DHLOGI("[SOURCE_IMPL] ReleaseSource called");

    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_) {
        DHLOGW("[SOURCE_IMPL] Not initialized");
        return 0;
    }

    StopCapture();

    socketReceiver_.reset();
    callback_ = nullptr;

    initialized_ = false;
    DHLOGI("[SOURCE_IMPL] ReleaseSource success");
    return 0;
}

int32_t SourceServiceImpl::RegisterDistributedHardware(const std::string& devId, const std::string& dhId) {
    DHLOGI("[SOURCE_IMPL] RegisterDistributedHardware called, devId: %{public}s, dhId: %{public}s",
            devId.c_str(), dhId.c_str());

    deviceId_ = devId;
    dhId_ = dhId;

    // 对应原始代码 dcamera_source_dev.cpp 的注册流程
    DHLOGI("[SOURCE_IMPL] RegisterDistributedHardware success");
    return 0;
}

int32_t SourceServiceImpl::UnregisterDistributedHardware(const std::string& devId, const std::string& dhId) {
    DHLOGI("[SOURCE_IMPL] UnregisterDistributedHardware called, devId: %{public}s, dhId: %{public}s",
            devId.c_str(), dhId.c_str());

    StopCapture();

    deviceId_.clear();
    dhId_.clear();

    DHLOGI("[SOURCE_IMPL] UnregisterDistributedHardware success");
    return 0;
}

int32_t SourceServiceImpl::StartCapture() {
    DHLOGI("[SOURCE_IMPL] StartCapture called");

    if (!initialized_) {
        DHLOGE("[SOURCE_IMPL] Not initialized");
        return -1;
    }

    if (capturing_) {
        DHLOGW("[SOURCE_IMPL] Already capturing");
        return 0;
    }

    // 模拟 HDF 回调流程
    // 对应原始代码 dcamera_source_data_process.cpp:150
    DHLOGI("[SOURCE_IMPL] DCameraSourceDataProcess StartCapture, dhId: %{public}s",
            dhId_.c_str());

    // 启动Socket接收器开始接收数据
    if (socketReceiver_->StartReceiving() != 0) {
        DHLOGE("[SOURCE_IMPL] Failed to start receiving");
        return -1;
    }

    capturing_ = true;

    // 通知回调
    if (callback_) {
        callback_->OnSourceStateChanged("CAPTURING");
    }

    DHLOGI("[SOURCE_IMPL] StartCapture success");
    return 0;
}

int32_t SourceServiceImpl::StopCapture() {
    DHLOGI("[SOURCE_IMPL] StopCapture called");

    if (!capturing_) {
        return 0;
    }

    if (socketReceiver_) {
        socketReceiver_->StopReceiving();
    }

    capturing_ = false;

    // 通知回调
    if (callback_) {
        callback_->OnSourceStateChanged("STOPPED");
    }

    DHLOGI("[SOURCE_IMPL] StopCapture success");
    return 0;
}

// ===== 导出工厂函数 =====

extern "C" __declspec(dllexport)
void* CreateSourceService() {
    DHLOGI("[SOURCE_IMPL] CreateSourceService called");
    return new SourceServiceImpl();
}

extern "C" __declspec(dllexport)
void DestroySourceService(void* instance) {
    DHLOGI("[SOURCE_IMPL] DestroySourceService called");
    delete static_cast<SourceServiceImpl*>(instance);
}

extern "C" __declspec(dllexport)
const char* GetSourceVersion() {
    static const char* version = "1.0.0";
    DHLOGI("[SOURCE_IMPL] GetSourceVersion called: %{public}s", version);
    return version;
}
