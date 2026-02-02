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

#include "surface_mock.h"
#include <cstring>
#include <chrono>
#include <algorithm>

namespace OHOS {

// ============================================================================
// SyncFence 实现
// ============================================================================

SyncFence::SyncFence(int32_t fd) : fd_(fd) {
}

SyncFence::~SyncFence() {
}

int32_t SyncFence::GetFd() const {
    return fd_;
}

void SyncFence::SyncWait() {
    // Mock实现 - 不实际等待
}

void SyncFence::SyncSignal() {
    // Mock实现 - 不实际发送信号
}

// ============================================================================
// SurfaceBufferExtraData 实现
// ============================================================================

SurfaceBufferExtraData::SurfaceBufferExtraData() {
}

SurfaceBufferExtraData::~SurfaceBufferExtraData() {
}

bool SurfaceBufferExtraData::ExtraSet(const std::string& key, int64_t value) {
    data_[key] = value;
    return true;
}

bool SurfaceBufferExtraData::ExtraGet(const std::string& key, int64_t& value) {
    auto it = data_.find(key);
    if (it != data_.end()) {
        value = it->second;
        return true;
    }
    return false;
}

bool SurfaceBufferExtraData::ExtraClear() {
    data_.clear();
    return true;
}

// ============================================================================
// SurfaceBuffer 实现
// ============================================================================

SurfaceBuffer::SurfaceBuffer(int32_t width, int32_t height, GraphicPixelFormat format)
    : width_(width), height_(height), format_(format), extraData_(new SurfaceBufferExtraData()) {
    AllocateBuffer();
}

SurfaceBuffer::~SurfaceBuffer() {
    if (bufferHandle_.virAddr != nullptr) {
        // 清理虚拟地址（Mock实现不需要实际munmap）
        bufferHandle_.virAddr = nullptr;
    }
}

void SurfaceBuffer::AllocateBuffer() {
    // 根据格式计算缓冲区大小
    uint32_t bufferSize = 0;

    switch (format_) {
        case GraphicPixelFormat::PIXEL_FMT_YCBCR_420_SP:  // NV12
        case GraphicPixelFormat::PIXEL_FMT_YCRCB_420_SP:  // NV21
            // Y平面 + UV交错平面
            bufferSize = width_ * height_ * 3 / 2;
            break;

        case GraphicPixelFormat::PIXEL_FMT_YCBCR_420_P:   // YUV420P (I420)
        case GraphicPixelFormat::PIXEL_FMT_YCRCB_420_P:
            // Y平面 + U平面 + V平面
            bufferSize = width_ * height_ * 3 / 2;
            break;

        case GraphicPixelFormat::PIXEL_FMT_RGBA_8888:
        case GraphicPixelFormat::PIXEL_FMT_BGRA_8888:
        case GraphicPixelFormat::PIXEL_FMT_RGBX_8888:
            bufferSize = width_ * height_ * 4;
            break;

        case GraphicPixelFormat::PIXEL_FMT_RGB_565:
            bufferSize = width_ * height_ * 2;
            break;

        case GraphicPixelFormat::PIXEL_FMT_RGB_888:
            bufferSize = width_ * height_ * 3;
            break;

        default:
            // 默认分配RGBA格式大小
            bufferSize = width_ * height_ * 4;
            break;
    }

    // 分配内存
    data_.resize(bufferSize);
    std::fill(data_.begin(), data_.end(), 0);

    // 初始化BufferHandle
    bufferHandle_.fd = -1;
    bufferHandle_.allocWidth = width_;
    bufferHandle_.allocHeight = height_;
    bufferHandle_.stride = width_;
    bufferHandle_.size = bufferSize;
    bufferHandle_.format = static_cast<uint32_t>(format_);
    bufferHandle_.usage = 0;
    bufferHandle_.virAddr = data_.data();
    bufferHandle_.key = 0;
    bufferHandle_.offset = 0;
    bufferHandle_.phyAddr = 0;
    bufferHandle_.reserveFds = 0;
    bufferHandle_.reserveInts = 0;
    for (int i = 0; i < 4; i++) {
        bufferHandle_.reserve[i] = nullptr;
    }
}

BufferHandle* SurfaceBuffer::GetBufferHandle() const {
    return const_cast<BufferHandle*>(&bufferHandle_);
}

void* SurfaceBuffer::GetVirAddr() {
    return bufferHandle_.virAddr;
}

uint32_t SurfaceBuffer::GetSize() const {
    return bufferHandle_.size;
}

GSError SurfaceBuffer::SetMetadata(uint32_t key, const std::vector<uint8_t>& value, bool enableCache) {
    metadata_[key] = value;
    return GSError::GSERROR_OK;
}

GSError SurfaceBuffer::GetMetadata(uint32_t key, std::vector<uint8_t>& value) {
    auto it = metadata_.find(key);
    if (it != metadata_.end()) {
        value = it->second;
        return GSError::GSERROR_OK;
    }
    return GSError::GSERROR_INVALID_ARGUMENTS;
}

SurfaceBufferExtraData* SurfaceBuffer::GetExtraData() {
    return extraData_.get();
}

// ============================================================================
// Surface::BufferProducerProxy 实现
// ============================================================================

class Surface::BufferProducerProxy : public IBufferProducer {
public:
    explicit BufferProducerProxy(sptr<Surface> surface) : surface_(surface) {}

    GSError RequestBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
                         BufferRequestConfig& config) override {
        return surface_->RequestBuffer(buffer, fence, config);
    }

    GSError FlushBuffer(sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence,
                       BufferFlushConfig& config) override {
        return surface_->FlushBuffer(buffer, fence, config);
    }

private:
    sptr<Surface> surface_;
};

// ============================================================================
// Surface 实现
// ============================================================================

Surface::Surface(std::string name, bool isConsumer)
    : name_(std::move(name)),
      isConsumer_(isConsumer),
      queueSize_(3),  // 默认3个缓冲区
      defaultUsage_(static_cast<BufferUsage>(BufferUsage::CPU_READ | BufferUsage::CPU_WRITE | BufferUsage::HARDWARE_CAMERA)),
      defaultWidth_(1920),
      defaultHeight_(1080),
      defaultFormat_(GraphicPixelFormat::PIXEL_FMT_YCBCR_420_SP) {
    InitializeBufferQueue();
}

Surface::~Surface() {
    std::lock_guard<std::mutex> lock(queueMutex_);

    // 清空队列
    while (!freeQueue_.empty()) {
        freeQueue_.pop();
    }
    while (!filledQueue_.empty()) {
        filledQueue_.pop();
    }
    allBuffers_.clear();
}

void Surface::InitializeBufferQueue() {
    std::lock_guard<std::mutex> lock(queueMutex_);

    // 预分配缓冲区
    for (uint32_t i = 0; i < queueSize_; i++) {
        auto buffer = std::make_shared<SurfaceBuffer>(defaultWidth_, defaultHeight_, defaultFormat_);
        auto fence = std::make_shared<SyncFence>();

        BufferQueueItem item;
        item.buffer = buffer;
        item.fence = fence;
        item.timestamp = 0;
        item.damage = {0, 0, defaultWidth_, defaultHeight_};
        item.inUse = false;

        freeQueue_.push(item);
        allBuffers_.push_back(item);
    }

    // 创建Producer代理
    if (isConsumer_) {
        producerProxy_ = std::make_shared<BufferProducerProxy>(shared_from_this());
    }
}

sptr<Surface> Surface::CreateSurfaceAsConsumer(std::string name) {
    auto surface = std::make_shared<Surface>(name, true);
    return surface;
}

sptr<Surface> Surface::CreateSurfaceAsProducer(sptr<IBufferProducer>& producer) {
    // Mock实现 - 返回一个producer模式的surface
    auto surface = std::make_shared<Surface>("producer", false);
    return surface;
}

GSError Surface::RequestBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
                              BufferRequestConfig& config) {
    std::unique_lock<std::mutex> lock(queueMutex_);

    // 从空闲队列获取缓冲区
    if (freeQueue_.empty()) {
        // 可选：等待或返回错误
        return GSError::GSERROR_NO_BUFFER;
    }

    BufferQueueItem item = freeQueue_.front();
    freeQueue_.pop();

    // 更新缓冲区配置（如果需要重新分配）
    if (item.buffer->GetWidth() != config.width ||
        item.buffer->GetHeight() != config.height ||
        static_cast<uint32_t>(item.buffer->GetFormat()) != static_cast<uint32_t>(config.format)) {
        // 重新分配缓冲区
        item.buffer = std::make_shared<SurfaceBuffer>(
            config.width > 0 ? config.width : defaultWidth_,
            config.height > 0 ? config.height : defaultHeight_,
            config.format != GraphicPixelFormat::PIXEL_FMT_RGBA_8888 ? config.format : defaultFormat_
        );
    }

    buffer = item.buffer;
    fence = item.fence;
    item.inUse = true;

    // 将其放回所有缓冲区列表（标记为使用中）
    for (auto& buf : allBuffers_) {
        if (buf.buffer == buffer) {
            buf.inUse = true;
            break;
        }
    }

    return GSError::GSERROR_OK;
}

GSError Surface::FlushBuffer(sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence,
                            BufferFlushConfig& config) {
    std::lock_guard<std::mutex> lock(queueMutex_);

    // 查找缓冲区并移动到已填充队列
    for (auto& item : allBuffers_) {
        if (item.buffer == buffer && item.inUse) {
            item.inUse = false;
            item.damage = config.damage;
            item.timestamp = config.timestamp;
            filledQueue_.push(item);

            // 通知消费者监听器
            if (consumerListener_ != nullptr) {
                consumerListener_->OnBufferAvailable();
            }

            queueCondition_.notify_one();
            return GSError::GSERROR_OK;
        }
    }

    return GSError::GSERROR_INVALID_ARGUMENTS;
}

GSError Surface::AcquireBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
                              int64_t& timestamp, Rect& damage) {
    std::unique_lock<std::mutex> lock(queueMutex_);

    // 等待可用的缓冲区
    if (filledQueue_.empty()) {
        return GSError::GSERROR_NO_BUFFER;
    }

    BufferQueueItem item = filledQueue_.front();
    filledQueue_.pop();

    buffer = item.buffer;
    fence = item.fence;
    timestamp = item.timestamp;
    damage = item.damage;
    item.inUse = true;

    return GSError::GSERROR_OK;
}

GSError Surface::ReleaseBuffer(sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence) {
    std::lock_guard<std::mutex> lock(queueMutex_);

    // 查找缓冲区并释放回空闲队列
    for (auto& item : allBuffers_) {
        if (item.buffer == buffer && item.inUse) {
            item.inUse = false;
            freeQueue_.push(item);
            return GSError::GSERROR_OK;
        }
    }

    return GSError::GSERROR_INVALID_ARGUMENTS;
}

GSError Surface::SetQueueSize(uint32_t queueSize) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    if (queueSize < 1 || queueSize > 16) {
        return GSError::GSERROR_INVALID_ARGUMENTS;
    }
    queueSize_ = queueSize;
    return GSError::GSERROR_OK;
}

GSError Surface::GetQueueSize(uint32_t& queueSize) {
    queueSize = queueSize_;
    return GSError::GSERROR_OK;
}

GSError Surface::SetDefaultUsage(BufferUsage usage) {
    defaultUsage_ = usage;
    return GSError::GSERROR_OK;
}

GSError Surface::SetDefaultWidthAndHeight(int32_t width, int32_t height) {
    if (width <= 0 || height <= 0) {
        return GSError::GSERROR_INVALID_ARGUMENTS;
    }
    defaultWidth_ = width;
    defaultHeight_ = height;
    return GSError::GSERROR_OK;
}

GSError Surface::SetDefaultFormat(GraphicPixelFormat format) {
    defaultFormat_ = format;
    return GSError::GSERROR_OK;
}

GSError Surface::RegisterConsumerListener(sptr<IBufferConsumerListener>& listener) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    consumerListener_ = listener;
    return GSError::GSERROR_OK;
}

sptr<IBufferProducer> Surface::GetProducer() const {
    return producerProxy_;
}

// ============================================================================
// IConsumerSurface 实现（包装类）
// ============================================================================

class ConsumerSurfaceImpl : public IConsumerSurface {
public:
    explicit ConsumerSurfaceImpl(sptr<Surface> surface) : surface_(surface) {}

    GSError AcquireBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
                         int64_t& timestamp, Rect& damage) override {
        return surface_->AcquireBuffer(buffer, fence, timestamp, damage);
    }

    GSError ReleaseBuffer(sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence) override {
        return surface_->ReleaseBuffer(buffer, fence);
    }

    sptr<IBufferProducer> GetProducer() const override {
        return surface_->GetProducer();
    }

    GSError RegisterConsumerListener(sptr<IBufferConsumerListener>& listener) override {
        return surface_->RegisterConsumerListener(listener);
    }

private:
    sptr<Surface> surface_;
};

// ============================================================================
// MockSurfaceFactory 实现
// ============================================================================

std::vector<wp<Surface>> MockSurfaceFactory::activeSurfaces_;
std::mutex MockSurfaceFactory::factoryMutex_;

sptr<Surface> MockSurfaceFactory::CreateConsumerSurface(std::string name) {
    std::lock_guard<std::mutex> lock(factoryMutex_);

    auto surface = Surface::CreateSurfaceAsConsumer(name);
    activeSurfaces_.push_back(surface);

    return surface;
}

sptr<IConsumerSurface> MockSurfaceFactory::CreateIConsumerSurface(std::string name) {
    std::lock_guard<std::mutex> lock(factoryMutex_);

    auto surface = Surface::CreateSurfaceAsConsumer(name);
    activeSurfaces_.push_back(surface);

    return std::make_shared<ConsumerSurfaceImpl>(surface);
}

void MockSurfaceFactory::DestroySurface(const sptr<Surface>& surface) {
    std::lock_guard<std::mutex> lock(factoryMutex_);

    auto it = std::remove_if(activeSurfaces_.begin(), activeSurfaces_.end(),
        [&surface](const wp<Surface>& weak) {
            auto locked = weak.lock();
            return locked == surface;
        });
    activeSurfaces_.erase(it, activeSurfaces_.end());
}

uint32_t MockSurfaceFactory::GetActiveSurfaceCount() {
    std::lock_guard<std::mutex> lock(factoryMutex_);

    uint32_t count = 0;
    for (const auto& weak : activeSurfaces_) {
        if (!weak.expired()) {
            count++;
        }
    }
    return count;
}

void MockSurfaceFactory::Reset() {
    std::lock_guard<std::mutex> lock(factoryMutex_);
    activeSurfaces_.clear();
}

} // namespace OHOS
