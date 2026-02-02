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

#include "avcodec_mock.h"
#include <cstring>
#include <random>
#include <chrono>
#include <algorithm>

namespace OHOS {
namespace MediaAVCodec {

// ============================================================================
// Format实现
// ============================================================================

void Format::PutIntValue(const std::string& key, int32_t value) {
    intValues_[key] = value;
}

void Format::PutDoubleValue(const std::string& key, double value) {
    doubleValues_[key] = value;
}

void Format::PutStringValue(const std::string& key, const std::string& value) {
    stringValues_[key] = value;
}

int32_t Format::GetIntValue(const std::string& key, int32_t defaultValue) const {
    auto it = intValues_.find(key);
    if (it != intValues_.end()) {
        return it->second;
    }
    return defaultValue;
}

double Format::GetDoubleValue(const std::string& key, double defaultValue) const {
    auto it = doubleValues_.find(key);
    if (it != doubleValues_.end()) {
        return it->second;
    }
    return defaultValue;
}

std::string Format::GetStringValue(const std::string& key, const std::string& defaultValue) const {
    auto it = stringValues_.find(key);
    if (it != stringValues_.end()) {
        return it->second;
    }
    return defaultValue;
}

bool Format::Contains(const std::string& key) const {
    return intValues_.find(key) != intValues_.end() ||
           doubleValues_.find(key) != doubleValues_.end() ||
           stringValues_.find(key) != stringValues_.end();
}

// ============================================================================
// AVBuffer实现
// ============================================================================

AVBuffer::AVBuffer() : bufferAttr_{} {
    bufferAttr_.presentationTimeUs = 0;
    bufferAttr_.size = 0;
    bufferAttr_.offset = 0;
}

AVBuffer::AVBuffer(size_t size) : data_(size), bufferAttr_{} {
    bufferAttr_.presentationTimeUs = 0;
    bufferAttr_.size = static_cast<int32_t>(size);
    bufferAttr_.offset = 0;
}

AVBuffer::~AVBuffer() = default;

void AVBuffer::SetData(const uint8_t* data, size_t size) {
    if (size > data_.capacity()) {
        data_.resize(size);
    }
    std::memcpy(data_.data(), data, size);
    bufferAttr_.size = static_cast<int32_t>(size);
}

uint8_t* AVBuffer::GetAddr() {
    return data_.data();
}

const uint8_t* AVBuffer::GetAddr() const {
    return data_.data();
}

size_t AVBuffer::GetSize() const {
    return data_.size();
}

void AVBuffer::SetBufferAttr(const AVCodecBufferInfo& info) {
    bufferAttr_ = info;
}

AVCodecBufferInfo AVBuffer::GetBufferAttr() const {
    return bufferAttr_;
}

// ============================================================================
// AVSharedMemory实现
// ============================================================================

AVSharedMemory::AVSharedMemory() : data_(0), fd_(-1) {}

AVSharedMemory::AVSharedMemory(size_t size) : data_(size), fd_(-1) {}

AVSharedMemory::~AVSharedMemory() = default;

uint8_t* AVSharedMemory::GetBase() {
    return data_.data();
}

const uint8_t* AVSharedMemory::GetBase() const {
    return data_.data();
}

size_t AVSharedMemory::GetSize() const {
    return data_.size();
}

int32_t AVSharedMemory::GetFd() const {
    return fd_;
}

// ============================================================================
// Surface实现
// ============================================================================

Surface::Surface() = default;

Surface::~Surface() = default;

void Surface::SetUserData(const std::string& key, void* data) {
    userData_[key] = data;
}

void* Surface::GetUserData(const std::string& key) {
    auto it = userData_.find(key);
    if (it != userData_.end()) {
        return it->second;
    }
    return nullptr;
}

// ============================================================================
// AVCodecVideoEncoder实现
// ============================================================================

AVCodecVideoEncoder::AVCodecVideoEncoder() = default;

AVCodecVideoEncoder::~AVCodecVideoEncoder() {
    Release();
}

int32_t AVCodecVideoEncoder::Configure(const Format& format) {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (configured_) {
        return -1; // Already configured
    }

    // 解析配置参数
    width_ = format.GetIntValue("width", 1920);
    height_ = format.GetIntValue("height", 1080);
    fps_ = format.GetDoubleValue("frame_rate", 30.0);
    bitrate_ = format.GetIntValue("bitrate", 5000000);

    int32_t formatValue = format.GetIntValue("pixel_format", static_cast<int32_t>(PixelFormat::NV12));
    pixelFormat_ = static_cast<PixelFormat>(formatValue);

    // 验证参数范围
    if (width_ <= 0 || height_ <= 0 || fps_ <= 0.0) {
        return -2; // Invalid parameters
    }

    configured_ = true;
    return 0; // Success
}

int32_t AVCodecVideoEncoder::Prepare() {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (!configured_) {
        return -1; // Not configured
    }

    if (prepared_) {
        return 0; // Already prepared
    }

    // 初始化Buffer池
    InitializeBuffers();

    prepared_ = true;
    return 0; // Success
}

int32_t AVCodecVideoEncoder::Start() {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (!prepared_) {
        return -1; // Not prepared
    }

    if (started_) {
        return 0; // Already started
    }

    started_ = true;
    return 0; // Success
}

int32_t AVCodecVideoEncoder::Stop() {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (!started_) {
        return 0; // Already stopped
    }

    started_ = false;
    return 0; // Success
}

int32_t AVCodecVideoEncoder::Flush() {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (!started_) {
        return -1; // Not started
    }

    // 重置Buffer索引
    nextInputBufferIndex_ = 0;
    nextOutputBufferIndex_ = 0;

    return 0; // Success
}

int32_t AVCodecVideoEncoder::NotifyEos() {
    std::lock_guard<std::mutex> lock(callbackMutex_);

    if (mediaCodecCallback_) {
        // 创建EOS标志的Buffer
        auto buffer = std::make_shared<AVBuffer>();
        AVCodecBufferInfo info{};
        info.presentationTimeUs = 0;
        info.size = 0;
        info.offset = 0;
        buffer->SetBufferAttr(info);

        // 触发EOS回调
        mediaCodecCallback_->OnOutputBufferAvailable(nextOutputBufferIndex_++, buffer);
    } else if (avCodecCallback_) {
        // 触发旧版本的EOS回调
        AVCodecBufferInfo info{};
        AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_EOS;
        auto buffer = std::make_shared<AVSharedMemory>(0);
        avCodecCallback_->OnOutputBufferAvailable(nextOutputBufferIndex_++, info, flag, buffer);
    }

    return 0; // Success
}

int32_t AVCodecVideoEncoder::Reset() {
    std::lock_guard<std::mutex> lock(stateMutex_);

    Stop();
    prepared_ = false;
    configured_ = false;
    nextInputBufferIndex_ = 0;
    nextOutputBufferIndex_ = 0;

    return 0; // Success
}

int32_t AVCodecVideoEncoder::Release() {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (released_) {
        return 0; // Already released
    }

    Reset();
    released_ = true;

    // 清理资源
    inputBuffers_.clear();
    outputBuffers_.clear();
    inputSurface_.reset();

    return 0; // Success
}

sptr<Surface> AVCodecVideoEncoder::CreateInputSurface() {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (!configured_) {
        return nullptr; // Not configured
    }

    if (!inputSurface_) {
        inputSurface_ = std::make_shared<Surface>();
    }

    return inputSurface_;
}

int32_t AVCodecVideoEncoder::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info,
                                              AVCodecBufferFlag flag) {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (!started_) {
        return -1; // Not started
    }

    if (index >= inputBuffers_.size()) {
        return -2; // Invalid index
    }

    // Mock实现：不实际处理数据，只记录
    return 0; // Success
}

int32_t AVCodecVideoEncoder::GetOutputFormat(Format& format) {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (!configured_) {
        return -1; // Not configured
    }

    format.PutIntValue("width", width_);
    format.PutIntValue("height", height_);
    format.PutDoubleValue("frame_rate", fps_);
    format.PutIntValue("bitrate", bitrate_);
    format.PutIntValue("pixel_format", static_cast<int32_t>(pixelFormat_));

    return 0; // Success
}

int32_t AVCodecVideoEncoder::ReleaseOutputBuffer(uint32_t index) {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (index >= outputBuffers_.size()) {
        return -2; // Invalid index
    }

    return 0; // Success
}

int32_t AVCodecVideoEncoder::SetParameter(const Format& format) {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (!started_) {
        return -1; // Not started
    }

    // 动态更新参数
    if (format.Contains("bitrate")) {
        bitrate_ = format.GetIntValue("bitrate", bitrate_);
    }

    return 0; // Success
}

int32_t AVCodecVideoEncoder::SetCallback(const std::shared_ptr<AVCodecCallback>& callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    avCodecCallback_ = callback;
    return 0; // Success
}

int32_t AVCodecVideoEncoder::SetCallback(const std::shared_ptr<MediaCodecCallback>& callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    mediaCodecCallback_ = callback;
    return 0; // Success
}

int32_t AVCodecVideoEncoder::QueryInputBuffer(uint32_t& index, int64_t timeoutUs) {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (!started_) {
        return -1; // Not started
    }

    if (nextInputBufferIndex_ >= inputBuffers_.size()) {
        return -2; // No available buffer
    }

    index = nextInputBufferIndex_++;
    return 0; // Success
}

int32_t AVCodecVideoEncoder::QueryOutputBuffer(uint32_t& index, int64_t timeoutUs) {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (!started_) {
        return -1; // Not started
    }

    if (nextOutputBufferIndex_ >= outputBuffers_.size()) {
        return -2; // No available buffer
    }

    index = nextOutputBufferIndex_++;
    return 0; // Success
}

std::shared_ptr<AVBuffer> AVCodecVideoEncoder::GetInputBuffer(uint32_t index) {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (index >= inputBuffers_.size()) {
        return nullptr;
    }

    return inputBuffers_[index];
}

std::shared_ptr<AVBuffer> AVCodecVideoEncoder::GetOutputBuffer(uint32_t index) {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (index >= outputBuffers_.size()) {
        return nullptr;
    }

    return outputBuffers_[index];
}

void AVCodecVideoEncoder::SimulateEncodedOutput(uint32_t index, const std::vector<uint8_t>& data,
                                                int64_t pts) {
    std::lock_guard<std::mutex> lock(callbackMutex_);

    auto buffer = std::make_shared<AVBuffer>(data.size());
    buffer->SetData(data.data(), data.size());

    AVCodecBufferInfo info{};
    info.presentationTimeUs = pts;
    info.size = static_cast<int32_t>(data.size());
    info.offset = 0;
    buffer->SetBufferAttr(info);

    if (mediaCodecCallback_) {
        mediaCodecCallback_->OnOutputBufferAvailable(index, buffer);
    } else if (avCodecCallback_) {
        auto sharedMem = std::make_shared<AVSharedMemory>(data.size());
        std::memcpy(sharedMem->GetBase(), data.data(), data.size());
        avCodecCallback_->OnOutputBufferAvailable(index, info,
                                                   AVCODEC_BUFFER_FLAG_SYNC_FRAME, sharedMem);
    }
}

void AVCodecVideoEncoder::SimulateError(AVCodecErrorType errorType, int32_t errorCode) {
    std::lock_guard<std::mutex> lock(callbackMutex_);

    if (mediaCodecCallback_) {
        mediaCodecCallback_->OnError(errorType, errorCode);
    } else if (avCodecCallback_) {
        avCodecCallback_->OnError(errorType, errorCode);
    }
}

void AVCodecVideoEncoder::InitializeBuffers() {
    const int32_t bufferCount = 8; // 标准Buffer数量

    // 初始化输入Buffer
    for (int32_t i = 0; i < bufferCount; ++i) {
        size_t bufferSize = width_ * height_ * 3 / 2; // NV12格式
        inputBuffers_.push_back(std::make_shared<AVBuffer>(bufferSize));
    }

    // 初始化输出Buffer（H.265编码后的数据，假设最大2MB）
    for (int32_t i = 0; i < bufferCount; ++i) {
        size_t bufferSize = 2 * 1024 * 1024;
        outputBuffers_.push_back(std::make_shared<AVBuffer>(bufferSize));
    }
}

std::vector<uint8_t> AVCodecVideoEncoder::GenerateMockH265Frame(int32_t width, int32_t height,
                                                                  bool isKeyFrame) {
    // H.265 NAL Unit起始码
    std::vector<uint8_t> frame = {0x00, 0x00, 0x00, 0x01};

    // NAL Header（简化版）
    if (isKeyFrame) {
        // IDR帧 NAL type = 20 (IDR_W_RADL)
        frame.push_back(0x20 | (1 << 6)); // forbidden_zero_bit + nal_unit_type
    } else {
        // P帧 NAL type = 1 (TRAIL_R)
        frame.push_back(0x01 | (1 << 6));
    }

    // 添加一些随机数据模拟编码内容
    size_t dataSize = width * height / 16; // 简化的大小估算
    frame.reserve(frame.size() + dataSize);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dis(0, 255);

    for (size_t i = 0; i < dataSize; ++i) {
        frame.push_back(dis(gen));
    }

    return frame;
}

// ============================================================================
// AVCodecVideoDecoder实现
// ============================================================================

AVCodecVideoDecoder::AVCodecVideoDecoder() = default;

AVCodecVideoDecoder::~AVCodecVideoDecoder() {
    Release();
}

int32_t AVCodecVideoDecoder::Configure(const Format& format) {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (configured_) {
        return -1; // Already configured
    }

    width_ = format.GetIntValue("width", 1920);
    height_ = format.GetIntValue("height", 1080);

    int32_t formatValue = format.GetIntValue("pixel_format", static_cast<int32_t>(PixelFormat::NV12));
    pixelFormat_ = static_cast<PixelFormat>(formatValue);

    if (width_ <= 0 || height_ <= 0) {
        return -2; // Invalid parameters
    }

    configured_ = true;
    return 0; // Success
}

int32_t AVCodecVideoDecoder::Prepare() {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (!configured_) {
        return -1; // Not configured
    }

    if (prepared_) {
        return 0; // Already prepared
    }

    InitializeBuffers();
    prepared_ = true;
    return 0; // Success
}

int32_t AVCodecVideoDecoder::Start() {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (!prepared_) {
        return -1; // Not prepared
    }

    if (started_) {
        return 0; // Already started
    }

    started_ = true;
    return 0; // Success
}

int32_t AVCodecVideoDecoder::Stop() {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (!started_) {
        return 0; // Already stopped
    }

    started_ = false;
    return 0; // Success
}

int32_t AVCodecVideoDecoder::Flush() {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (!started_) {
        return -1; // Not started
    }

    nextInputBufferIndex_ = 0;
    nextOutputBufferIndex_ = 0;
    return 0; // Success
}

int32_t AVCodecVideoDecoder::Reset() {
    std::lock_guard<std::mutex> lock(stateMutex_);

    Stop();
    prepared_ = false;
    configured_ = false;
    nextInputBufferIndex_ = 0;
    nextOutputBufferIndex_ = 0;
    return 0; // Success
}

int32_t AVCodecVideoDecoder::Release() {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (released_) {
        return 0; // Already released
    }

    Reset();
    released_ = true;

    inputBuffers_.clear();
    outputBuffers_.clear();
    outputSurface_.reset();

    return 0; // Success
}

int32_t AVCodecVideoDecoder::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info,
                                              AVCodecBufferFlag flag) {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (!started_) {
        return -1; // Not started
    }

    if (index >= inputBuffers_.size()) {
        return -2; // Invalid index
    }

    return 0; // Success
}

int32_t AVCodecVideoDecoder::GetOutputFormat(Format& format) {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (!configured_) {
        return -1; // Not configured
    }

    format.PutIntValue("width", width_);
    format.PutIntValue("height", height_);
    format.PutIntValue("pixel_format", static_cast<int32_t>(pixelFormat_));

    return 0; // Success
}

int32_t AVCodecVideoDecoder::ReleaseOutputBuffer(uint32_t index, bool render) {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (index >= outputBuffers_.size()) {
        return -2; // Invalid index
    }

    return 0; // Success
}

int32_t AVCodecVideoDecoder::SetParameter(const Format& format) {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (!started_) {
        return -1; // Not started
    }

    return 0; // Success
}

int32_t AVCodecVideoDecoder::SetCallback(const std::shared_ptr<AVCodecCallback>& callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    avCodecCallback_ = callback;
    return 0; // Success
}

int32_t AVCodecVideoDecoder::SetCallback(const std::shared_ptr<MediaCodecCallback>& callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    mediaCodecCallback_ = callback;
    return 0; // Success
}

int32_t AVCodecVideoDecoder::SetOutputSurface(sptr<Surface> surface) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    outputSurface_ = surface;
    return 0; // Success
}

int32_t AVCodecVideoDecoder::QueryInputBuffer(uint32_t& index, int64_t timeoutUs) {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (!started_) {
        return -1; // Not started
    }

    if (nextInputBufferIndex_ >= inputBuffers_.size()) {
        return -2; // No available buffer
    }

    index = nextInputBufferIndex_++;
    return 0; // Success
}

int32_t AVCodecVideoDecoder::QueryOutputBuffer(uint32_t& index, int64_t timeoutUs) {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (!started_) {
        return -1; // Not started
    }

    if (nextOutputBufferIndex_ >= outputBuffers_.size()) {
        return -2; // No available buffer
    }

    index = nextOutputBufferIndex_++;
    return 0; // Success
}

std::shared_ptr<AVBuffer> AVCodecVideoDecoder::GetInputBuffer(uint32_t index) {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (index >= inputBuffers_.size()) {
        return nullptr;
    }

    return inputBuffers_[index];
}

std::shared_ptr<AVBuffer> AVCodecVideoDecoder::GetOutputBuffer(uint32_t index) {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (index >= outputBuffers_.size()) {
        return nullptr;
    }

    return outputBuffers_[index];
}

void AVCodecVideoDecoder::SimulateDecodedOutput(uint32_t index, int32_t width, int32_t height,
                                                 int64_t pts) {
    std::lock_guard<std::mutex> lock(callbackMutex_);

    auto yuvData = GenerateMockYUVFrame(width, height);
    auto buffer = std::make_shared<AVBuffer>(yuvData.size());
    buffer->SetData(yuvData.data(), yuvData.size());

    AVCodecBufferInfo info{};
    info.presentationTimeUs = pts;
    info.size = static_cast<int32_t>(yuvData.size());
    info.offset = 0;
    buffer->SetBufferAttr(info);

    if (mediaCodecCallback_) {
        mediaCodecCallback_->OnOutputBufferAvailable(index, buffer);
    } else if (avCodecCallback_) {
        auto sharedMem = std::make_shared<AVSharedMemory>(yuvData.size());
        std::memcpy(sharedMem->GetBase(), yuvData.data(), yuvData.size());
        avCodecCallback_->OnOutputBufferAvailable(index, info,
                                                   AVCODEC_BUFFER_FLAG_NONE, sharedMem);
    }
}

void AVCodecVideoDecoder::SimulateError(AVCodecErrorType errorType, int32_t errorCode) {
    std::lock_guard<std::mutex> lock(callbackMutex_);

    if (mediaCodecCallback_) {
        mediaCodecCallback_->OnError(errorType, errorCode);
    } else if (avCodecCallback_) {
        avCodecCallback_->OnError(errorType, errorCode);
    }
}

void AVCodecVideoDecoder::InitializeBuffers() {
    const int32_t bufferCount = 8;

    // 初始化输入Buffer（H.265编码数据）
    for (int32_t i = 0; i < bufferCount; ++i) {
        size_t bufferSize = 2 * 1024 * 1024; // 最大2MB
        inputBuffers_.push_back(std::make_shared<AVBuffer>(bufferSize));
    }

    // 初始化输出Buffer（YUV数据）
    for (int32_t i = 0; i < bufferCount; ++i) {
        size_t bufferSize = width_ * height_ * 3 / 2; // NV12格式
        outputBuffers_.push_back(std::make_shared<AVBuffer>(bufferSize));
    }
}

std::vector<uint8_t> AVCodecVideoDecoder::GenerateMockYUVFrame(int32_t width, int32_t height) {
    size_t ySize = width * height;
    size_t uvSize = ySize / 2;
    size_t totalSize = ySize + uvSize;

    std::vector<uint8_t> yuvData(totalSize);

    // 生成Y平面（灰度渐变）
    for (int32_t y = 0; y < height; ++y) {
        for (int32_t x = 0; x < width; ++x) {
            uint8_t value = static_cast<uint8_t>((x + y) * 255 / (width + height));
            yuvData[y * width + x] = value;
        }
    }

    // 生成UV平面（简化：使用固定值）
    for (size_t i = ySize; i < totalSize; ++i) {
        yuvData[i] = 128;
    }

    return yuvData;
}

// ============================================================================
// VideoEncoderFactory实现
// ============================================================================

std::shared_ptr<AVCodecVideoEncoder> VideoEncoderFactory::CreateByMime(const std::string& mime) {
    // 支持的MIME类型
    if (mime == "video/hevc" || mime == "video/h265") {
        return std::make_shared<AVCodecVideoEncoder>();
    }
    if (mime == "video/avc" || mime == "video/h264") {
        return std::make_shared<AVCodecVideoEncoder>();
    }
    if (mime == "image/jpeg") {
        return std::make_shared<AVCodecVideoEncoder>();
    }

    return nullptr;
}

std::shared_ptr<AVCodecVideoEncoder> VideoEncoderFactory::CreateByName(const std::string& name) {
    // 支持的编码器名称
    if (name.find("OMX.hisi.video.encoder.hevc") != std::string::npos ||
        name.find("OMX.hisi.video.encoder.avc") != std::string::npos ||
        name.find("OMX.hisi.image.encoder.jpeg") != std::string::npos) {
        return std::make_shared<AVCodecVideoEncoder>();
    }

    return nullptr;
}

// ============================================================================
// VideoDecoderFactory实现
// ============================================================================

std::shared_ptr<AVCodecVideoDecoder> VideoDecoderFactory::CreateByMime(const std::string& mime) {
    // 支持的MIME类型
    if (mime == "video/hevc" || mime == "video/h265") {
        return std::make_shared<AVCodecVideoDecoder>();
    }
    if (mime == "video/avc" || mime == "video/h264") {
        return std::make_shared<AVCodecVideoDecoder>();
    }

    return nullptr;
}

std::shared_ptr<AVCodecVideoDecoder> VideoDecoderFactory::CreateByName(const std::string& name) {
    // 支持的解码器名称
    if (name.find("OMX.hisi.video.decoder.hevc") != std::string::npos ||
        name.find("OMX.hisi.video.decoder.avc") != std::string::npos) {
        return std::make_shared<AVCodecVideoDecoder>();
    }

    return nullptr;
}

} // namespace MediaAVCodec
} // namespace OHOS
