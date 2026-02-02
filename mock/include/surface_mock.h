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

#ifndef OHOS_SURFACE_MOCK_H
#define OHOS_SURFACE_MOCK_H

#include <memory>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include <map>
#include <atomic>
#include <functional>

namespace OHOS {

// Forward declarations
class SurfaceBuffer;
class IBufferProducer;
struct BufferRequestConfig;
struct BufferFlushConfig;
struct Rect;

// GSError - 图形错误码定义
enum class GSError : int32_t {
    GSERROR_OK = 0,
    GSERROR_INVALID_ARGUMENTS = 1,
    GSERROR_NO_MEM = 2,
    GSERROR_NO_BUFFER = 3,
    GSERROR_INTERNAL = 4,
    GSERROR_INVALID_API = 5,
    GSERROR_INVALID_OPERATING = 6,
    GSERROR_TIMED_OUT = 7,
};

// SurfaceError - Surface错误码定义
enum class SurfaceError : int32_t {
    SURFACE_ERROR_OK = 0,
    SURFACE_ERROR_INVALID_PARAM = 1,
    SURFACE_ERROR_NO_MEMORY = 2,
    SURFACE_ERROR_NO_BUFFER = 3,
    SURFACE_ERROR_API_NOT_SUPPORTED = 4,
};

// BufferFormat - 缓冲区格式定义
enum class GraphColorSpace : int32_t {
    BT601_FULL = 0,
    BT601_LIMITED = 1,
    BT709_FULL = 2,
    BT709_LIMITED = 3,
};

enum class GraphicPixelFormat : uint32_t {
    PIXEL_FMT_RGBA_8888 = 0,
    PIXEL_FMT_RGBX_8888 = 1,
    PIXEL_FMT_RGB_565 = 2,
    PIXEL_FMT_BGRA_8888 = 3,
    PIXEL_FMT_YCBCR_420_SP = 4,      // NV12
    PIXEL_FMT_YCRCB_420_SP = 5,      // NV21
    PIXEL_FMT_YCBCR_422_SP = 6,
    PIXEL_FMT_YCRCB_422_SP = 7,
    PIXEL_FMT_YCBCR_420_P = 8,       // YUV420P (I420)
    PIXEL_FMT_YCRCB_420_P = 9,
    PIXEL_FMT_YCBCR_422_P = 10,
    PIXEL_FMT_YCRCB_422_P = 11,
    PIXEL_FMT_YCBCR_420_SP_HP = 12,
    PIXEL_FMT_RGBA_FP16 = 13,
    PIXEL_FMT_RGB_888 = 14,
    PIXEL_FMT_BGRA_FP16 = 15,
};

// BufferUsageConfig - 缓冲区用途配置
enum class BufferUsage : uint64_t {
    CPU_READ = 1ULL << 0,
    CPU_WRITE = 1ULL << 1,
    GPU_READ = 1ULL << 2,
    GPU_WRITE = 1ULL << 3,
    HARDWARE_ENCODER = 1ULL << 4,
    HARDWARE_DECODER = 1ULL << 5,
    HARDWARE_CAMERA = 1ULL << 6,
    HARDWARE_RENDER = 1ULL << 7,
    TEXTURE = 1ULL << 8,
    HW_COMPOSER = 1ULL << 9,
    HW_VIDEO_ENCODER = 1ULL << 10,
    HW_VIDEO_DECODER = 1ULL << 11,
};

// BufferRequestConfig - 缓冲区请求配置
struct BufferRequestConfig {
    int32_t width;
    int32_t height;
    int32_t strideAlignment;
    GraphicPixelFormat format;
    BufferUsage usage;
    int64_t timeout;
};

// BufferFlushConfig - 缓冲区刷新配置
struct BufferFlushConfig {
    Rect damage;
    int32_t timestamp;
};

// Rect - 矩形区域定义
struct Rect {
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
};

// BufferHandle - 缓冲区句柄定义
struct BufferHandle {
    int32_t fd;
    int32_t allocWidth;
    int32_t allocHeight;
    int32_t stride;
    uint32_t size;
    uint32_t format;
    uint64_t usage;
    void* virAddr;
    int32_t key;
    int32_t offset;
    uint64_t phyAddr;
    int32_t reserveFds;
    int32_t reserveInts;
    void* reserve[4];
};

// SyncFence - 同步栅栏（简化实现）
class SyncFence {
public:
    SyncFence(int32_t fd = -1);
    ~SyncFence();
    int32_t GetFd() const;
    void SyncWait();
    void SyncSignal();

private:
    int32_t fd_;
};

// SurfaceBufferExtraData - 缓冲区额外数据
class SurfaceBufferExtraData {
public:
    SurfaceBufferExtraData();
    ~SurfaceBufferExtraData();

    bool ExtraSet(const std::string& key, int64_t value);
    bool ExtraGet(const std::string& key, int64_t& value);
    bool ExtraClear();

private:
    std::map<std::string, int64_t> data_;
};

// SurfaceBuffer - Surface缓冲区实现
class SurfaceBuffer {
public:
    SurfaceBuffer(int32_t width, int32_t height, GraphicPixelFormat format);
    ~SurfaceBuffer();

    // 基础接口
    BufferHandle* GetBufferHandle() const;
    void* GetVirAddr();
    uint32_t GetSize() const;

    // 元数据接口
    GSError SetMetadata(uint32_t key, const std::vector<uint8_t>& value, bool enableCache = true);
    GSError GetMetadata(uint32_t key, std::vector<uint8_t>& value);

    // 额外数据接口
    SurfaceBufferExtraData* GetExtraData();

    // 属性访问
    int32_t GetWidth() const { return width_; }
    int32_t GetHeight() const { return height_; }
    GraphicPixelFormat GetFormat() const { return format_; }
    void* GetData() { return data_.data(); }

private:
    void AllocateBuffer();

    int32_t width_;
    int32_t height_;
    GraphicPixelFormat format_;
    std::vector<uint8_t> data_;
    BufferHandle bufferHandle_;
    std::unique_ptr<SurfaceBufferExtraData> extraData_;
    std::map<uint32_t, std::vector<uint8_t>> metadata_;
};

// IBufferProducer - 缓冲区生产者接口
class IBufferProducer : public std::enable_shared_from_this<IBufferProducer> {
public:
    virtual ~IBufferProducer() = default;

    virtual GSError RequestBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
                                  BufferRequestConfig& config) = 0;
    virtual GSError FlushBuffer(sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence,
                                BufferFlushConfig& config) = 0;
};

// IBufferConsumerListener - 缓冲区消费者监听器
class IBufferConsumerListener {
public:
    virtual ~IBufferConsumerListener() = default;
    virtual void OnBufferAvailable() = 0;
};

// IConsumerSurface - 消费者Surface接口
class IConsumerSurface {
public:
    virtual ~IConsumerSurface() = default;

    virtual GSError AcquireBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
                                  int64_t& timestamp, Rect& damage) = 0;
    virtual GSError ReleaseBuffer(sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence) = 0;
    virtual sptr<IBufferProducer> GetProducer() const = 0;
    virtual GSError RegisterConsumerListener(sptr<IBufferConsumerListener>& listener) = 0;
};

// Surface - 主Surface类
class Surface : public std::enable_shared_from_this<Surface> {
public:
    static sptr<Surface> CreateSurfaceAsConsumer(std::string name = "noname");
    static sptr<Surface> CreateSurfaceAsProducer(sptr<IBufferProducer>& producer);

    virtual ~Surface();

    // Producer接口
    virtual GSError RequestBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
                                  BufferRequestConfig& config);
    virtual GSError FlushBuffer(sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence,
                                BufferFlushConfig& config);

    // Consumer接口
    virtual GSError AcquireBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
                                  int64_t& timestamp, Rect& damage);
    virtual GSError ReleaseBuffer(sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence);

    // 通用接口
    GSError SetQueueSize(uint32_t queueSize);
    GSError GetQueueSize(uint32_t& queueSize);
    GSError SetDefaultUsage(BufferUsage usage);
    GSError SetDefaultWidthAndHeight(int32_t width, int32_t height);
    GSError SetDefaultFormat(GraphicPixelFormat format);
    GSError RegisterConsumerListener(sptr<IBufferConsumerListener>& listener);
    sptr<IBufferProducer> GetProducer() const;

    std::string GetName() const { return name_; }

protected:
    Surface(std::string name, bool isConsumer);
    void InitializeBufferQueue();

private:
    std::string name_;
    bool isConsumer_;

    // Buffer队列管理
    struct BufferQueueItem {
        sptr<SurfaceBuffer> buffer;
        sptr<SyncFence> fence;
        int64_t timestamp;
        Rect damage;
        bool inUse;
    };

    std::queue<BufferQueueItem> freeQueue_;
    std::queue<BufferQueueItem> filledQueue_;
    std::vector<BufferQueueItem> allBuffers_;
    mutable std::mutex queueMutex_;
    std::condition_variable queueCondition_;

    // 配置参数
    uint32_t queueSize_;
    BufferUsage defaultUsage_;
    int32_t defaultWidth_;
    int32_t defaultHeight_;
    GraphicPixelFormat defaultFormat_;

    // 监听器
    sptr<IBufferConsumerListener> consumerListener_;

    // Producer接口
    class BufferProducerProxy;
    sptr<BufferProducerProxy> producerProxy_;
};

// MockSurfaceFactory - Surface工厂类
class MockSurfaceFactory {
public:
    static sptr<Surface> CreateConsumerSurface(std::string name = "noname");
    static sptr<IConsumerSurface> CreateIConsumerSurface(std::string name = "noname");
    static void DestroySurface(const sptr<Surface>& surface);

    // 统计接口
    static uint32_t GetActiveSurfaceCount();
    static void Reset();

private:
    static std::vector<wp<Surface>> activeSurfaces_;
    static std::mutex factoryMutex_;
};

} // namespace OHOS

// 智能指针宏定义（兼容OpenHarmony）
template<typename T>
using sptr = std::shared_ptr<T>;

template<typename T>
using wp = std::weak_ptr<T>;

#endif // OHOS_SURFACE_MOCK_H
