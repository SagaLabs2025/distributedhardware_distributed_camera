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

#include "dcamera_source_sink_manager.h"
#include "distributed_hardware_log.h"

namespace OHOS {
namespace DistributedHardware {

DCameraSourceSinkManager& DCameraSourceSinkManager::GetInstance()
{
    static DCameraSourceSinkManager instance;
    return instance;
}

int32_t DCameraSourceSinkManager::InitSource(const std::string& params)
{
    if (sourceInitialized_.load()) {
        DHLOGI("Source already initialized");
        return 0;
    }

    sourceThread_ = std::make_unique<DCameraThreadIsolation>(ThreadRole::SOURCE);
    int32_t ret = sourceThread_->Start();
    if (ret != 0) {
        DHLOGE("Failed to start source thread, ret: %d", ret);
        sourceThread_.reset();
        return ret;
    }

    sourceInitialized_.store(true);
    DHLOGI("Source initialized successfully");
    return 0;
}

int32_t DCameraSourceSinkManager::InitSink(const std::string& params)
{
    if (sinkInitialized_.load()) {
        DHLOGI("Sink already initialized");
        return 0;
    }

    sinkThread_ = std::make_unique<DCameraThreadIsolation>(ThreadRole::SINK);
    int32_t ret = sinkThread_->Start();
    if (ret != 0) {
        DHLOGE("Failed to start sink thread, ret: %d", ret);
        sinkThread_.reset();
        return ret;
    }

    sinkInitialized_.store(true);
    DHLOGI("Sink initialized successfully");
    return 0;
}

int32_t DCameraSourceSinkManager::ReleaseSource()
{
    if (!sourceInitialized_.load()) {
        DHLOGI("Source not initialized");
        return 0;
    }

    if (sourceThread_) {
        sourceThread_->Stop();
        sourceThread_.reset();
    }

    sourceInitialized_.store(false);
    DHLOGI("Source released successfully");
    return 0;
}

int32_t DCameraSourceSinkManager::ReleaseSink()
{
    if (!sinkInitialized_.load()) {
        DHLOGI("Sink not initialized");
        return 0;
    }

    if (sinkThread_) {
        sinkThread_->Stop();
        sinkThread_.reset();
    }

    sinkInitialized_.store(false);
    DHLOGI("Sink released successfully");
    return 0;
}

void DCameraSourceSinkManager::PostSourceTask(std::function<void()> task)
{
    if (sourceThread_ && sourceInitialized_.load()) {
        sourceThread_->PostTask(task);
    } else {
        DHLOGE("Cannot post task to source thread, not initialized");
    }
}

void DCameraSourceSinkManager::PostSinkTask(std::function<void()> task)
{
    if (sinkThread_ && sinkInitialized_.load()) {
        sinkThread_->PostTask(task);
    } else {
        DHLOGE("Cannot post task to sink thread, not initialized");
    }
}

int32_t DCameraSourceSinkManager::SendData(DCameraSessionMode mode, std::shared_ptr<DataBuffer>& buffer)
{
    // 在實際實現中，這裡會根據模式選擇正確的通道發送數據
    // 目前只是示範雙線程架構
    if (!buffer) {
        DHLOGE("Invalid buffer");
        return -1;
    }

    DHLOGI("Sending data with mode: %d, buffer size: %zu",
           static_cast<int>(mode), buffer->Size());
    return 0;
}

void DCameraSourceSinkManager::SetChannelListener(std::shared_ptr<ICameraChannelListener>& listener)
{
    std::lock_guard<std::mutex> lock(listenerMutex_);
    channelListener_ = listener;
}

bool DCameraSourceSinkManager::IsSourceInitialized() const
{
    return sourceInitialized_.load();
}

bool DCameraSourceSinkManager::IsSinkInitialized() const
{
    return sinkInitialized_.load();
}

} // namespace DistributedHardware
} // namespace OHOS