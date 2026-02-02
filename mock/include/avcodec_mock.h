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

#ifndef OHOS_AVCODEC_MOCK_H
#define OHOS_AVCODEC_MOCK_H

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>
#include <cstdint>
#include <functional>

namespace OHOS {
namespace MediaAVCodec {

// 前向声明
class AVCodecVideoEncoder;
class AVCodecVideoDecoder;
class MediaCodecCallback;
class AVCodecCallback;

// AVCodec错误类型
enum class AVCodecErrorType {
    ERROR_UNKNOWN = 0,
    ERROR_CODEC = 1,
    ERROR_RESOURCE = 2,
};

// 编解码缓冲区信息
struct AVCodecBufferInfo {
    int64_t presentationTimeUs;  // 显示时间戳（微秒）
    int32_t size;                // 数据大小（字节）
    int32_t offset;              // 数据偏移
};

// 编解码缓冲区标志
enum AVCodecBufferFlag : uint32_t {
    AVCODEC_BUFFER_FLAG_NONE = 0,
    AVCODEC_BUFFER_FLAG_EOS = 1 << 0,              // 流结束标志
    AVCODEC_BUFFER_FLAG_SYNC_FRAME = 1 << 1,       // 同步帧（关键帧）
    AVCODEC_BUFFER_FLAG_PARTIAL_FRAME = 1 << 2,    // 部分帧
    AVCODEC_BUFFER_FLAG_CODEC_DATA = 1 << 3,       // 编码器特定数据
};

// 像素格式枚举
enum PixelFormat {
    YUV420 = 0,
    NV12 = 1,
    NV21 = 2,
    RGBA_8888 = 3,
    JPEG = 4,
};

// Format类 - 用于配置编码器参数
class Format {
public:
    Format() = default;
    ~Format() = default;

    void PutIntValue(const std::string& key, int32_t value);
    void PutDoubleValue(const std::string& key, double value);
    void PutStringValue(const std::string& key, const std::string& value);

    int32_t GetIntValue(const std::string& key, int32_t defaultValue) const;
    double GetDoubleValue(const std::string& key, double defaultValue) const;
    std::string GetStringValue(const std::string& key, const std::string& defaultValue) const;

    bool Contains(const std::string& key) const;

private:
    std::map<std::string, int32_t> intValues_;
    std::map<std::string, double> doubleValues_;
    std::map<std::string, std::string> stringValues_;
};

// AVBuffer类 - 新版本的Buffer接口
class AVBuffer {
public:
    AVBuffer();
    explicit AVBuffer(size_t size);
    ~AVBuffer();

    void SetData(const uint8_t* data, size_t size);
    uint8_t* GetAddr();
    const uint8_t* GetAddr() const;
    size_t GetSize() const;

    void SetBufferAttr(const AVCodecBufferInfo& info);
    AVCodecBufferInfo GetBufferAttr() const;

private:
    std::vector<uint8_t> data_;
    AVCodecBufferInfo bufferAttr_;
};

// AVSharedMemory类 - 旧版本的共享内存接口
class AVSharedMemory {
public:
    AVSharedMemory();
    explicit AVSharedMemory(size_t size);
    ~AVSharedMemory();

    uint8_t* GetBase();
    const uint8_t* GetBase() const;
    size_t GetSize() const;

    int32_t GetFd() const;

private:
    std::vector<uint8_t> data_;
    int32_t fd_;
};

// Surface类前向声明（简化版本）
class Surface {
public:
    Surface();
    virtual ~Surface();

    void SetUserData(const std::string& key, void* data);
    void* GetUserData(const std::string& key);

private:
    std::map<std::string, void*> userData_;
};

using sptr = std::shared_ptr;

// MediaCodecCallback - 新回调风格（使用AVBuffer）
class MediaCodecCallback {
public:
    virtual ~MediaCodecCallback() = default;

    virtual void OnError(AVCodecErrorType errorType, int32_t errorCode) = 0;
    virtual void OnOutputFormatChanged(const Format& format) = 0;
    virtual void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) = 0;
    virtual void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) = 0;
};

// AVCodecCallback - 传统回调风格（使用AVSharedMemory）
class AVCodecCallback {
public:
    virtual ~AVCodecCallback() = default;

    virtual void OnError(AVCodecErrorType errorType, int32_t errorCode) = 0;
    virtual void OnOutputFormatChanged(const Format& format) = 0;
    virtual void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer) = 0;
    virtual void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info,
                                        AVCodecBufferFlag flag,
                                        std::shared_ptr<AVSharedMemory> buffer) = 0;
};

// AVCodecVideoEncoder接口 - Mock H.265编码器
class AVCodecVideoEncoder {
public:
    AVCodecVideoEncoder();
    virtual ~AVCodecVideoEncoder();

    // 配置编码器
    virtual int32_t Configure(const Format& format);

    // 准备编码器
    virtual int32_t Prepare();

    // 启动编码器
    virtual int32_t Start();

    // 停止编码器
    virtual int32_t Stop();

    // 刷新编码器
    virtual int32_t Flush();

    // 通知流结束
    virtual int32_t NotifyEos();

    // 重置编码器
    virtual int32_t Reset();

    // 释放编码器
    virtual int32_t Release();

    // 创建输入Surface（Surface模式）- 关键接口
    virtual sptr<Surface> CreateInputSurface();

    // 提交输入Buffer（Buffer模式）
    virtual int32_t QueueInputBuffer(uint32_t index, AVCodecBufferInfo info,
                                     AVCodecBufferFlag flag);

    // 获取输出格式
    virtual int32_t GetOutputFormat(Format& format);

    // 释放输出Buffer
    virtual int32_t ReleaseOutputBuffer(uint32_t index);

    // 设置参数
    virtual int32_t SetParameter(const Format& format);

    // 注册回调（AVCodecCallback风格）
    virtual int32_t SetCallback(const std::shared_ptr<AVCodecCallback>& callback);

    // 注册回调（MediaCodecCallback风格）
    virtual int32_t SetCallback(const std::shared_ptr<MediaCodecCallback>& callback);

    // 查询可用输入Buffer
    virtual int32_t QueryInputBuffer(uint32_t& index, int64_t timeoutUs);

    // 查询可用输出Buffer
    virtual int32_t QueryOutputBuffer(uint32_t& index, int64_t timeoutUs);

    // 获取输入Buffer
    virtual std::shared_ptr<AVBuffer> GetInputBuffer(uint32_t index);

    // 获取输出Buffer
    virtual std::shared_ptr<AVBuffer> GetOutputBuffer(uint32_t index);

    // Mock测试辅助方法
    bool IsConfigured() const { return configured_; }
    bool IsPrepared() const { return prepared_; }
    bool IsStarted() const { return started_; }
    int32_t GetConfigWidth() const { return width_; }
    int32_t GetConfigHeight() const { return height_; }
    double GetConfigFps() const { return fps_; }
    PixelFormat GetConfigFormat() const { return pixelFormat_; }

    // 模拟输出编码数据（用于测试）
    void SimulateEncodedOutput(uint32_t index, const std::vector<uint8_t>& data, int64_t pts);
    void SimulateError(AVCodecErrorType errorType, int32_t errorCode);

private:
    // 配置参数
    int32_t width_ = 1920;
    int32_t height_ = 1080;
    double fps_ = 30.0;
    int32_t bitrate_ = 5000000;
    PixelFormat pixelFormat_ = PixelFormat::NV12;

    // 状态标志
    std::atomic<bool> configured_{false};
    std::atomic<bool> prepared_{false};
    std::atomic<bool> started_{false};
    std::atomic<bool> released_{false};

    // Surface
    sptr<Surface> inputSurface_;

    // 回调
    std::shared_ptr<AVCodecCallback> avCodecCallback_;
    std::shared_ptr<MediaCodecCallback> mediaCodecCallback_;

    // Buffer管理
    std::vector<std::shared_ptr<AVBuffer>> inputBuffers_;
    std::vector<std::shared_ptr<AVBuffer>> outputBuffers_;
    uint32_t nextInputBufferIndex_ = 0;
    uint32_t nextOutputBufferIndex_ = 0;

    // 互斥锁
    mutable std::mutex stateMutex_;
    mutable std::mutex callbackMutex_;

    // 内部辅助方法
    void InitializeBuffers();
    std::vector<uint8_t> GenerateMockH265Frame(int32_t width, int32_t height, bool isKeyFrame);
};

// AVCodecVideoDecoder接口 - Mock H.265解码器
class AVCodecVideoDecoder {
public:
    AVCodecVideoDecoder();
    virtual ~AVCodecVideoDecoder();

    // 配置解码器
    virtual int32_t Configure(const Format& format);

    // 准备解码器
    virtual int32_t Prepare();

    // 启动解码器
    virtual int32_t Start();

    // 停止解码器
    virtual int32_t Stop();

    // 刷新解码器
    virtual int32_t Flush();

    // 重置解码器
    virtual int32_t Reset();

    // 释放解码器
    virtual int32_t Release();

    // 提交输入Buffer
    virtual int32_t QueueInputBuffer(uint32_t index, AVCodecBufferInfo info,
                                     AVCodecBufferFlag flag);

    // 获取输出格式
    virtual int32_t GetOutputFormat(Format& format);

    // 释放输出Buffer
    virtual int32_t ReleaseOutputBuffer(uint32_t index, bool render);

    // 设置参数
    virtual int32_t SetParameter(const Format& format);

    // 注册回调（AVCodecCallback风格）
    virtual int32_t SetCallback(const std::shared_ptr<AVCodecCallback>& callback);

    // 注册回调（MediaCodecCallback风格）
    virtual int32_t SetCallback(const std::shared_ptr<MediaCodecCallback>& callback);

    // 创建输出Surface
    virtual int32_t SetOutputSurface(sptr<Surface> surface);

    // 查询可用输入Buffer
    virtual int32_t QueryInputBuffer(uint32_t& index, int64_t timeoutUs);

    // 查询可用输出Buffer
    virtual int32_t QueryOutputBuffer(uint32_t& index, int64_t timeoutUs);

    // 获取输入Buffer
    virtual std::shared_ptr<AVBuffer> GetInputBuffer(uint32_t index);

    // 获取输出Buffer
    virtual std::shared_ptr<AVBuffer> GetOutputBuffer(uint32_t index);

    // Mock测试辅助方法
    bool IsConfigured() const { return configured_; }
    bool IsPrepared() const { return prepared_; }
    bool IsStarted() const { return started_; }

    // 模拟输出解码数据（用于测试）
    void SimulateDecodedOutput(uint32_t index, int32_t width, int32_t height, int64_t pts);
    void SimulateError(AVCodecErrorType errorType, int32_t errorCode);

private:
    // 配置参数
    int32_t width_ = 1920;
    int32_t height_ = 1080;
    PixelFormat pixelFormat_ = PixelFormat::NV12;

    // 状态标志
    std::atomic<bool> configured_{false};
    std::atomic<bool> prepared_{false};
    std::atomic<bool> started_{false};
    std::atomic<bool> released_{false};

    // Surface
    sptr<Surface> outputSurface_;

    // 回调
    std::shared_ptr<AVCodecCallback> avCodecCallback_;
    std::shared_ptr<MediaCodecCallback> mediaCodecCallback_;

    // Buffer管理
    std::vector<std::shared_ptr<AVBuffer>> inputBuffers_;
    std::vector<std::shared_ptr<AVBuffer>> outputBuffers_;
    uint32_t nextInputBufferIndex_ = 0;
    uint32_t nextOutputBufferIndex_ = 0;

    // 互斥锁
    mutable std::mutex stateMutex_;
    mutable std::mutex callbackMutex_;

    // 内部辅助方法
    void InitializeBuffers();
    std::vector<uint8_t> GenerateMockYUVFrame(int32_t width, int32_t height);
};

// VideoEncoderFactory - 编码器工厂
class VideoEncoderFactory {
public:
    // 通过MIME类型创建编码器
    static std::shared_ptr<AVCodecVideoEncoder> CreateByMime(const std::string& mime);

    // 通过名称创建编码器
    static std::shared_ptr<AVCodecVideoEncoder> CreateByName(const std::string& name);
};

// VideoDecoderFactory - 解码器工厂
class VideoDecoderFactory {
public:
    // 通过MIME类型创建解码器
    static std::shared_ptr<AVCodecVideoDecoder> CreateByMime(const std::string& mime);

    // 通过名称创建解码器
    static std::shared_ptr<AVCodecVideoDecoder> CreateByName(const std::string& name);
};

} // namespace MediaAVCodec
} // namespace OHOS

#endif // OHOS_AVCODEC_MOCK_H
