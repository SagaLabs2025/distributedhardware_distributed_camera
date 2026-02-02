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

#include "hdi_mock.h"
#include <algorithm>
#include <cstring>
#include "distributed_hardware_log.h"

namespace OHOS {
namespace DistributedHardware {

// ==================== MockHdiProvider 实现 ====================

std::shared_ptr<MockHdiProvider> MockHdiProvider::GetInstance() {
    static std::shared_ptr<MockHdiProvider> instance(new MockHdiProvider());
    return instance;
}

int32_t MockHdiProvider::EnableDCameraDevice(
    const DHBase& dhBase,
    const std::string& abilityInfo,
    const std::shared_ptr<IDCameraProviderCallback>& callbackObj) {

    std::lock_guard<std::mutex> lock(mutex_);

    DHLOGI("MockHdiProvider::EnableDCameraDevice dhId=%s", dhBase.dhId_.c_str());

    // 验证参数
    if (dhBase.dhId_.empty()) {
        DHLOGE("MockHdiProvider::EnableDCameraDevice dhId is empty");
        return static_cast<int32_t>(DCamRetCode::INVALID_ARGUMENT);
    }

    // 检查设备是否已启用
    if (enabledDevices_.find(dhBase.dhId_) != enabledDevices_.end()) {
        DHLOGW("MockHdiProvider::EnableDCameraDevice device already enabled, dhId=%s",
               dhBase.dhId_.c_str());
        return static_cast<int32_t>(DCamRetCode::SUCCESS);
    }

    // 保存回调对象
    callback_ = callbackObj;

    // 标记设备为已启用
    enabledDevices_[dhBase.dhId_] = true;

    DHLOGI("MockHdiProvider::EnableDCameraDevice success, dhId=%s", dhBase.dhId_.c_str());

    return enableResult_;
}

int32_t MockHdiProvider::DisableDCameraDevice(const DHBase& dhBase) {
    std::lock_guard<std::mutex> lock(mutex_);

    DHLOGI("MockHdiProvider::DisableDCameraDevice dhId=%s", dhBase.dhId_.c_str());

    // 验证参数
    if (dhBase.dhId_.empty()) {
        DHLOGE("MockHdiProvider::DisableDCameraDevice dhId is empty");
        return static_cast<int32_t>(DCamRetCode::INVALID_ARGUMENT);
    }

    // 检查设备是否存在
    auto it = enabledDevices_.find(dhBase.dhId_);
    if (it == enabledDevices_.end()) {
        DHLOGW("MockHdiProvider::DisableDCameraDevice device not found, dhId=%s",
               dhBase.dhId_.c_str());
        return static_cast<int32_t>(DCamRetCode::DEVICE_NOT_INIT);
    }

    // 清理流状态
    streams_.clear();

    // 移除设备
    enabledDevices_.erase(it);

    DHLOGI("MockHdiProvider::DisableDCameraDevice success, dhId=%s", dhBase.dhId_.c_str());

    return static_cast<int32_t>(DCamRetCode::SUCCESS);
}

int32_t MockHdiProvider::AcquireBuffer(const DHBase& dhBase, int streamId, DCameraBuffer& buffer) {
    std::lock_guard<std::mutex> lock(mutex_);

    DHLOGI("MockHdiProvider::AcquireBuffer dhId=%s streamId=%d", dhBase.dhId_.c_str(), streamId);

    // 验证参数
    if (dhBase.dhId_.empty()) {
        DHLOGE("MockHdiProvider::AcquireBuffer dhId is empty");
        return static_cast<int32_t>(DCamRetCode::INVALID_ARGUMENT);
    }

    // 检查设备是否已启用
    if (enabledDevices_.find(dhBase.dhId_) == enabledDevices_.end()) {
        DHLOGE("MockHdiProvider::AcquireBuffer device not enabled, dhId=%s",
               dhBase.dhId_.c_str());
        return static_cast<int32_t>(DCamRetCode::DEVICE_NOT_INIT);
    }

    // 检查流是否存在
    auto streamIt = streams_.find(streamId);
    if (streamIt == streams_.end()) {
        DHLOGE("MockHdiProvider::AcquireBuffer stream not found, streamId=%d", streamId);
        return static_cast<int32_t>(DCamRetCode::INVALID_ARGUMENT);
    }

    // 检查缓冲区数量限制
    auto& streamState = streamIt->second;
    if (streamState.bufferCount >= streamState.maxBuffers) {
        DHLOGW("MockHdiProvider::AcquireBuffer buffer count exceeded, streamId=%d", streamId);
        return static_cast<int32_t>(DCamRetCode::CAMERA_BUSY);
    }

    // 计算缓冲区大小
    size_t bufferSize = streamState.streamInfo.width_ * streamState.streamInfo.height_ * 3 / 2;
    if (streamState.streamInfo.encodeType_ == DCEncodeType::ENCODE_TYPE_JPEG) {
        // JPEG编码，分配更大的缓冲区
        bufferSize = streamState.streamInfo.width_ * streamState.streamInfo.height_ * 2;
    }

    // 创建缓冲区（零拷贝实现）
    buffer = CreateMockBuffer(streamId, bufferSize);

    // 更新统计
    streamState.bufferCount++;
    bufferAcquireCount_++;

    DHLOGI("MockHdiProvider::AcquireBuffer success, streamId=%d, index=%d, size=%u",
           streamId, buffer.index_, buffer.size_);

    return acquireBufferResult_;
}

int32_t MockHdiProvider::ShutterBuffer(const DHBase& dhBase, int streamId,
                                        const DCameraBuffer& buffer) {
    std::lock_guard<std::mutex> lock(mutex_);

    DHLOGI("MockHdiProvider::ShutterBuffer dhId=%s streamId=%d bufferIndex=%d",
           dhBase.dhId_.c_str(), streamId, buffer.index_);

    // 验证参数
    if (dhBase.dhId_.empty()) {
        DHLOGE("MockHdiProvider::ShutterBuffer dhId is empty");
        return static_cast<int32_t>(DCamRetCode::INVALID_ARGUMENT);
    }

    // 检查设备是否已启用
    if (enabledDevices_.find(dhBase.dhId_) == enabledDevices_.end()) {
        DHLOGE("MockHdiProvider::ShutterBuffer device not enabled, dhId=%s",
               dhBase.dhId_.c_str());
        return static_cast<int32_t>(DCamRetCode::DEVICE_NOT_INIT);
    }

    // 检查流是否存在
    auto streamIt = streams_.find(streamId);
    if (streamIt == streams_.end()) {
        DHLOGE("MockHdiProvider::ShutterBuffer stream not found, streamId=%d", streamId);
        return static_cast<int32_t>(DCamRetCode::INVALID_ARGUMENT);
    }

    // 减少缓冲区计数
    auto& streamState = streamIt->second;
    if (streamState.bufferCount > 0) {
        streamState.bufferCount--;
    }

    // 释放缓冲区资源（零拷贝实现）
    ReleaseMockBuffer(buffer);

    // 更新统计
    bufferShutterCount_++;

    DHLOGI("MockHdiProvider::ShutterBuffer success, streamId=%d, bufferIndex=%d",
           streamId, buffer.index_);

    return shutterBufferResult_;
}

int32_t MockHdiProvider::OnSettingsResult(const DHBase& dhBase,
                                           const DCameraSettings& result) {
    DHLOGI("MockHdiProvider::OnSettingsResult dhId=%s type=%d",
           dhBase.dhId_.c_str(), static_cast<int>(result.type_));

    // 验证参数
    if (dhBase.dhId_.empty()) {
        DHLOGE("MockHdiProvider::OnSettingsResult dhId is empty");
        return static_cast<int32_t>(DCamRetCode::INVALID_ARGUMENT);
    }

    // 检查设备是否已启用
    if (enabledDevices_.find(dhBase.dhId_) == enabledDevices_.end()) {
        DHLOGE("MockHdiProvider::OnSettingsResult device not enabled, dhId=%s",
               dhBase.dhId_.c_str());
        return static_cast<int32_t>(DCamRetCode::DEVICE_NOT_INIT);
    }

    // 这里可以触发回调通知设置结果
    if (callback_) {
        std::vector<DCameraSettings> settings = {result};
        callback_->UpdateSettings(dhBase, settings);
    }

    return static_cast<int32_t>(DCamRetCode::SUCCESS);
}

int32_t MockHdiProvider::Notify(const DHBase& dhBase, const DCameraHDFEvent& event) {
    DHLOGI("MockHdiProvider::Notify dhId=%s eventType=%d result=%d",
           dhBase.dhId_.c_str(), event.type_, event.result_);

    // 验证参数
    if (dhBase.dhId_.empty()) {
        DHLOGE("MockHdiProvider::Notify dhId is empty");
        return static_cast<int32_t>(DCamRetCode::INVALID_ARGUMENT);
    }

    // 检查设备是否已启用
    if (enabledDevices_.find(dhBase.dhId_) == enabledDevices_.end()) {
        DHLOGE("MockHdiProvider::Notify device not enabled, dhId=%s",
               dhBase.dhId_.c_str());
        return static_cast<int32_t>(DCamRetCode::DEVICE_NOT_INIT);
    }

    // 事件处理可以在这里添加
    // 例如：错误事件、状态变化事件等

    return static_cast<int32_t>(DCamRetCode::SUCCESS);
}

void MockHdiProvider::SetEnableResult(int32_t result) {
    enableResult_ = result;
}

void MockHdiProvider::SetAcquireBufferResult(int32_t result) {
    acquireBufferResult_ = result;
}

void MockHdiProvider::SetShutterBufferResult(int32_t result) {
    shutterBufferResult_ = result;
}

void MockHdiProvider::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    enabledDevices_.clear();
    callback_.reset();
    streams_.clear();
    nextBufferIndex_ = 0;
    bufferAcquireCount_ = 0;
    bufferShutterCount_ = 0;
    enableResult_ = static_cast<int32_t>(DCamRetCode::SUCCESS);
    acquireBufferResult_ = static_cast<int32_t>(DCamRetCode::SUCCESS);
    shutterBufferResult_ = static_cast<int32_t>(DCamRetCode::SUCCESS);
}

int32_t MockHdiProvider::TriggerOpenSession(const DHBase& dhBase) {
    DHLOGI("MockHdiProvider::TriggerOpenSession dhId=%s", dhBase.dhId_.c_str());

    if (!callback_) {
        DHLOGE("MockHdiProvider::TriggerOpenSession callback is null");
        return static_cast<int32_t>(DCamRetCode::FAILED);
    }

    return callback_->OpenSession(dhBase);
}

int32_t MockHdiProvider::TriggerCloseSession(const DHBase& dhBase) {
    DHLOGI("MockHdiProvider::TriggerCloseSession dhId=%s", dhBase.dhId_.c_str());

    if (!callback_) {
        DHLOGE("MockHdiProvider::TriggerCloseSession callback is null");
        return static_cast<int32_t>(DCamRetCode::FAILED);
    }

    return callback_->CloseSession(dhBase);
}

int32_t MockHdiProvider::TriggerConfigureStreams(
    const DHBase& dhBase,
    const std::vector<DCStreamInfo>& streamInfos) {

    DHLOGI("MockHdiProvider::TriggerConfigureStreams dhId=%s streamCount=%zu",
           dhBase.dhId_.c_str(), streamInfos.size());

    if (!callback_) {
        DHLOGE("MockHdiProvider::TriggerConfigureStreams callback is null");
        return static_cast<int32_t>(DCamRetCode::FAILED);
    }

    // 保存流配置
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& streamInfo : streamInfos) {
        StreamState state;
        state.streamInfo = streamInfo;
        state.isActive = true;
        state.bufferCount = 0;
        state.maxBuffers = 8;  // 默认最大缓冲区数
        streams_[streamInfo.streamId_] = state;
    }

    return callback_->ConfigureStreams(dhBase, streamInfos);
}

int32_t MockHdiProvider::TriggerReleaseStreams(
    const DHBase& dhBase,
    const std::vector<int>& streamIds) {

    DHLOGI("MockHdiProvider::TriggerReleaseStreams dhId=%s streamCount=%zu",
           dhBase.dhId_.c_str(), streamIds.size());

    if (!callback_) {
        DHLOGE("MockHdiProvider::TriggerReleaseStreams callback is null");
        return static_cast<int32_t>(DCamRetCode::FAILED);
    }

    // 移除流配置
    std::lock_guard<std::mutex> lock(mutex_);
    for (int streamId : streamIds) {
        streams_.erase(streamId);
    }

    return callback_->ReleaseStreams(dhBase, streamIds);
}

int32_t MockHdiProvider::TriggerStartCapture(
    const DHBase& dhBase,
    const std::vector<DCCaptureInfo>& captureInfos) {

    DHLOGI("MockHdiProvider::TriggerStartCapture dhId=%s captureCount=%zu",
           dhBase.dhId_.c_str(), captureInfos.size());

    if (!callback_) {
        DHLOGE("MockHdiProvider::TriggerStartCapture callback is null");
        return static_cast<int32_t>(DCamRetCode::FAILED);
    }

    return callback_->StartCapture(dhBase, captureInfos);
}

int32_t MockHdiProvider::TriggerStopCapture(
    const DHBase& dhBase,
    const std::vector<int>& streamIds) {

    DHLOGI("MockHdiProvider::TriggerStopCapture dhId=%s streamCount=%zu",
           dhBase.dhId_.c_str(), streamIds.size());

    if (!callback_) {
        DHLOGE("MockHdiProvider::TriggerStopCapture callback is null");
        return static_cast<int32_t>(DCamRetCode::FAILED);
    }

    return callback_->StopCapture(dhBase, streamIds);
}

int32_t MockHdiProvider::TriggerUpdateSettings(
    const DHBase& dhBase,
    const std::vector<DCameraSettings>& settings) {

    DHLOGI("MockHdiProvider::TriggerUpdateSettings dhId=%s settingsCount=%zu",
           dhBase.dhId_.c_str(), settings.size());

    if (!callback_) {
        DHLOGE("MockHdiProvider::TriggerUpdateSettings callback is null");
        return static_cast<int32_t>(DCamRetCode::FAILED);
    }

    return callback_->UpdateSettings(dhBase, settings);
}

bool MockHdiProvider::IsDeviceEnabled(const std::string& dhId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return enabledDevices_.find(dhId) != enabledDevices_.end();
}

size_t MockHdiProvider::GetActiveStreamCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return streams_.size();
}

size_t MockHdiProvider::GetBufferAcquireCount() const {
    return bufferAcquireCount_.load();
}

size_t MockHdiProvider::GetBufferShutterCount() const {
    return bufferShutterCount_.load();
}

std::vector<int> MockHdiProvider::GetConfiguredStreamIds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<int> streamIds;
    for (const auto& pair : streams_) {
        streamIds.push_back(pair.first);
    }
    return streamIds;
}

void* MockHdiProvider::GetBufferData(const DCameraBuffer& buffer) {
    std::lock_guard<std::mutex> lock(bufferPoolMutex_);
    auto it = bufferDataPool_.find(buffer.index_);
    if (it != bufferDataPool_.end()) {
        return it->second.data();
    }
    return nullptr;
}

size_t MockHdiProvider::GetBufferSize(const DCameraBuffer& buffer) {
    return buffer.size_;
}

DCameraBuffer MockHdiProvider::CreateMockBuffer(int streamId, size_t size) {
    std::lock_guard<std::mutex> lock(bufferPoolMutex_);

    DCameraBuffer buffer;
    buffer.index_ = nextBufferIndex_++;
    buffer.size_ = size;
    buffer.bufferHandle_ = nullptr;

    // 零拷贝实现：直接分配内存并返回指针
    bufferDataPool_[buffer.index_].resize(size);
    buffer.virAddr_ = bufferDataPool_[buffer.index_].data();

    return buffer;
}

void MockHdiProvider::ReleaseMockBuffer(const DCameraBuffer& buffer) {
    std::lock_guard<std::mutex> lock(bufferPoolMutex_);
    // 零拷贝实现：不需要特殊释放，vector会自动管理内存
    bufferDataPool_.erase(buffer.index_);
}

// ==================== MockProviderCallback 实现 ====================

MockProviderCallback::MockProviderCallback() {
    DHLOGI("MockProviderCallback constructor");
}

int32_t MockProviderCallback::OpenSession(const DHBase& dhBase) {
    std::lock_guard<std::mutex> lock(mutex_);

    DHLOGI("MockProviderCallback::OpenSession dhId=%s", dhBase.dhId_.c_str());

    openSessionCount_++;
    sessionOpen_ = true;

    return callbackResult_;
}

int32_t MockProviderCallback::CloseSession(const DHBase& dhBase) {
    std::lock_guard<std::mutex> lock(mutex_);

    DHLOGI("MockProviderCallback::CloseSession dhId=%s", dhBase.dhId_.c_str());

    closeSessionCount_++;
    sessionOpen_ = false;
    streamsConfigured_ = false;
    captureStarted_ = false;

    return callbackResult_;
}

int32_t MockProviderCallback::ConfigureStreams(
    const DHBase& dhBase,
    const std::vector<DCStreamInfo>& streamInfos) {

    std::lock_guard<std::mutex> lock(mutex_);

    DHLOGI("MockProviderCallback::ConfigureStreams dhId=%s streamCount=%zu",
           dhBase.dhId_.c_str(), streamInfos.size());

    configureStreamsCount_++;
    lastStreamInfos_ = streamInfos;
    streamsConfigured_ = true;

    return callbackResult_;
}

int32_t MockProviderCallback::ReleaseStreams(
    const DHBase& dhBase,
    const std::vector<int>& streamIds) {

    std::lock_guard<std::mutex> lock(mutex_);

    DHLOGI("MockProviderCallback::ReleaseStreams dhId=%s streamCount=%zu",
           dhBase.dhId_.c_str(), streamIds.size());

    releaseStreamsCount_++;

    return callbackResult_;
}

int32_t MockProviderCallback::StartCapture(
    const DHBase& dhBase,
    const std::vector<DCCaptureInfo>& captureInfos) {

    std::lock_guard<std::mutex> lock(mutex_);

    DHLOGI("MockProviderCallback::StartCapture dhId=%s captureCount=%zu",
           dhBase.dhId_.c_str(), captureInfos.size());

    startCaptureCount_++;
    lastCaptureInfos_ = captureInfos;
    captureStarted_ = true;

    return callbackResult_;
}

int32_t MockProviderCallback::StopCapture(
    const DHBase& dhBase,
    const std::vector<int>& streamIds) {

    std::lock_guard<std::mutex> lock(mutex_);

    DHLOGI("MockProviderCallback::StopCapture dhId=%s streamCount=%zu",
           dhBase.dhId_.c_str(), streamIds.size());

    stopCaptureCount_++;
    captureStarted_ = false;

    return callbackResult_;
}

int32_t MockProviderCallback::UpdateSettings(
    const DHBase& dhBase,
    const std::vector<DCameraSettings>& settings) {

    std::lock_guard<std::mutex> lock(mutex_);

    DHLOGI("MockProviderCallback::UpdateSettings dhId=%s settingsCount=%zu",
           dhBase.dhId_.c_str(), settings.size());

    updateSettingsCount_++;

    return callbackResult_;
}

void MockProviderCallback::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    openSessionCount_ = 0;
    closeSessionCount_ = 0;
    configureStreamsCount_ = 0;
    releaseStreamsCount_ = 0;
    startCaptureCount_ = 0;
    stopCaptureCount_ = 0;
    updateSettingsCount_ = 0;
    sessionOpen_ = false;
    streamsConfigured_ = false;
    captureStarted_ = false;
    lastStreamInfos_.clear();
    lastCaptureInfos_.clear();
    callbackResult_ = static_cast<int32_t>(DCamRetCode::SUCCESS);
}

void MockProviderCallback::SetCallbackResult(int32_t result) {
    callbackResult_ = result;
}

// ==================== TripleStreamConfig 实现 ====================

std::vector<DCStreamInfo> TripleStreamConfig::CreateDefaultTripleStreams() {
    std::vector<DCStreamInfo> streams;

    streams.push_back(CreateControlStream());
    streams.push_back(CreateSnapshotStream(SNAPSHOT_MAX_WIDTH, SNAPSHOT_MAX_HEIGHT));
    streams.push_back(CreateContinuousStream(CONTINUOUS_MAX_WIDTH, CONTINUOUS_MAX_HEIGHT));

    return streams;
}

std::vector<DCStreamInfo> TripleStreamConfig::CreateCustomTripleStreams(
    int snapshotWidth, int snapshotHeight,
    int continuousWidth, int continuousHeight) {

    std::vector<DCStreamInfo> streams;

    streams.push_back(CreateControlStream());
    streams.push_back(CreateSnapshotStream(snapshotWidth, snapshotHeight));
    streams.push_back(CreateContinuousStream(continuousWidth, continuousHeight));

    return streams;
}

DCStreamInfo TripleStreamConfig::CreateControlStream() {
    DCStreamInfo stream;
    stream.streamId_ = CONTROL_STREAM_ID;
    stream.width_ = 0;
    stream.height_ = 0;
    stream.stride_ = 0;
    stream.format_ = 0;
    stream.dataspace_ = 0;
    stream.encodeType_ = DCEncodeType::ENCODE_TYPE_NULL;
    stream.type_ = DCStreamType::CONTINUOUS_FRAME;
    return stream;
}

DCStreamInfo TripleStreamConfig::CreateSnapshotStream(int width, int height) {
    DCStreamInfo stream;
    stream.streamId_ = SNAPSHOT_STREAM_ID;
    stream.width_ = width;
    stream.height_ = height;
    stream.stride_ = width;
    stream.format_ = 1;  // JPEG format
    stream.dataspace_ = 0;
    stream.encodeType_ = SNAPSHOT_ENCODE_TYPE;
    stream.type_ = DCStreamType::SNAPSHOT_FRAME;
    return stream;
}

DCStreamInfo TripleStreamConfig::CreateContinuousStream(int width, int height) {
    DCStreamInfo stream;
    stream.streamId_ = CONTINUOUS_STREAM_ID;
    stream.width_ = width;
    stream.height_ = height;
    stream.stride_ = width;
    stream.format_ = 2;  // NV12 format
    stream.dataspace_ = 0;
    stream.encodeType_ = CONTINUOUS_ENCODE_TYPE;
    stream.type_ = DCStreamType::CONTINUOUS_FRAME;
    return stream;
}

// ==================== ZeroCopyBufferManager 实现 ====================

ZeroCopyBufferManager& ZeroCopyBufferManager::GetInstance() {
    static ZeroCopyBufferManager instance;
    return instance;
}

ZeroCopyBufferManager::~ZeroCopyBufferManager() {
    Reset();
}

DCameraBuffer ZeroCopyBufferManager::CreateBuffer(size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);

    DCameraBuffer buffer;
    buffer.index_ = nextBufferId_++;
    buffer.size_ = size;
    buffer.bufferHandle_ = nullptr;

    // 零拷贝：直接分配内存
    bufferDataMap_[buffer.index_].resize(size);
    buffer.virAddr_ = bufferDataMap_[buffer.index_].data();

    totalAllocatedSize_ += size;

    DHLOGI("ZeroCopyBufferManager::CreateBuffer index=%d size=%zu", buffer.index_, size);

    return buffer;
}

DCameraBuffer ZeroCopyBufferManager::AcquireBuffer(int streamId, size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);

    DCameraBuffer buffer;
    buffer.index_ = nextBufferId_++;
    buffer.size_ = size;
    buffer.bufferHandle_ = nullptr;

    // 零拷贝：直接分配内存
    bufferDataMap_[buffer.index_].resize(size);
    buffer.virAddr_ = bufferDataMap_[buffer.index_].data();

    totalAllocatedSize_ += size;

    DHLOGI("ZeroCopyBufferManager::AcquireBuffer streamId=%d index=%d size=%zu",
           streamId, buffer.index_, size);

    return buffer;
}

int32_t ZeroCopyBufferManager::ReleaseBuffer(const DCameraBuffer& buffer) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = bufferDataMap_.find(buffer.index_);
    if (it == bufferDataMap_.end()) {
        DHLOGE("ZeroCopyBufferManager::ReleaseBuffer buffer not found, index=%d", buffer.index_);
        return static_cast<int32_t>(DCamRetCode::INVALID_ARGUMENT);
    }

    totalAllocatedSize_ -= it->second.size();
    bufferDataMap_.erase(it);

    DHLOGI("ZeroCopyBufferManager::ReleaseBuffer index=%d", buffer.index_);

    return static_cast<int32_t>(DCamRetCode::SUCCESS);
}

void* ZeroCopyBufferManager::GetBufferData(const DCameraBuffer& buffer) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = bufferDataMap_.find(buffer.index_);
    if (it != bufferDataMap_.end()) {
        return it->second.data();
    }

    return nullptr;
}

size_t ZeroCopyBufferManager::GetBufferSize(const DCameraBuffer& buffer) {
    return buffer.size_;
}

int32_t ZeroCopyBufferManager::SetBufferData(const DCameraBuffer& buffer,
                                              const void* data, size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = bufferDataMap_.find(buffer.index_);
    if (it == bufferDataMap_.end()) {
        DHLOGE("ZeroCopyBufferManager::SetBufferData buffer not found, index=%d", buffer.index_);
        return static_cast<int32_t>(DCamRetCode::INVALID_ARGUMENT);
    }

    if (size > buffer.size_) {
        DHLOGE("ZeroCopyBufferManager::SetBufferData size exceeds buffer size, size=%zu buffer=%u",
               size, buffer.size_);
        return static_cast<int32_t>(DCamRetCode::INVALID_ARGUMENT);
    }

    // 零拷贝：直接内存拷贝
    std::memcpy(it->second.data(), data, size);

    return static_cast<int32_t>(DCamRetCode::SUCCESS);
}

size_t ZeroCopyBufferManager::GetActiveBufferCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return bufferDataMap_.size();
}

size_t ZeroCopyBufferManager::GetTotalAllocatedSize() const {
    return totalAllocatedSize_.load();
}

void ZeroCopyBufferManager::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    bufferDataMap_.clear();
    nextBufferId_ = 0;
    totalAllocatedSize_ = 0;
}

} // namespace DistributedHardware
} // namespace OHOS
