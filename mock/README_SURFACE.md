# Surface Mock 使用指南

## 概述

Surface Mock 是分布式相机项目中的图形缓冲区管理模拟实现，提供了 OpenHarmony Surface 接口的完整 Mock 功能，用于单元测试和集成测试。

## 主要特性

1. **零拷贝Buffer队列模拟**：实现了生产者-消费者模式的缓冲区队列
2. **支持多种图像格式**：NV12、NV21、YUV420P (I420)、RGBA、RGB565 等
3. **完整的Surface接口**：RequestBuffer、FlushBuffer、AcquireBuffer、ReleaseBuffer
4. **线程安全**：使用互斥锁和条件变量保证多线程安全
5. **元数据支持**：支持缓冲区元数据的存储和检索

## 架构设计

### 核心类

```
Surface (主Surface类)
├── SurfaceBuffer (缓冲区实现)
├── SyncFence (同步栅栏)
├── SurfaceBufferExtraData (额外数据)
├── IBufferProducer (生产者接口)
├── IConsumerSurface (消费者接口)
└── IBufferConsumerListener (消费者监听器)
```

### 生产者-消费者模型

```
Producer                        Consumer
   |                              |
   |--- RequestBuffer --------->   |   (获取空闲缓冲区)
   |                              |
   |<-- (返回空闲缓冲区) ------    |
   |                              |
   |--- 填充数据                  |
   |                              |
   |--- FlushBuffer ---------->   |   (提交已填充缓冲区)
   |                              |
   |                         OnBufferAvailable()
   |                              |
   |                              |--- AcquireBuffer ---> (获取已填充缓冲区)
   |                              |
   |                              |<-- (返回已填充缓冲区) --
   |                              |
   |                              |--- 处理数据
   |                              |
   |                              |--- ReleaseBuffer ---> (释放缓冲区回空闲队列)
   |                              |
   |<-- (缓冲区可重用) ----------  |
```

## 使用示例

### 1. 基本的Producer-Consumer模式

```cpp
#include "surface_mock.h"

using namespace OHOS;

// 创建Consumer Surface
auto consumerSurface = MockSurfaceFactory::CreateIConsumerSurface("camera_surface");
consumerSurface->SetDefaultWidthAndHeight(1920, 1080);
consumerSurface->SetDefaultFormat(GraphicPixelFormat::PIXEL_FMT_YCBCR_420_SP);

// 注册监听器
auto listener = std::make_shared<MyBufferListener>();
consumerSurface->RegisterConsumerListener(listener);

// 获取Producer
auto producer = consumerSurface->GetProducer();

// Producer请求缓冲区
sptr<SurfaceBuffer> buffer;
sptr<SyncFence> fence;
BufferRequestConfig config = {
    .width = 1920,
    .height = 1080,
    .strideAlignment = 8,
    .format = GraphicPixelFormat::PIXEL_FMT_YCBCR_420_SP,
    .usage = static_cast<BufferUsage>(BufferUsage::CPU_READ | BufferUsage::CPU_WRITE),
    .timeout = 1000
};

GSError ret = producer->RequestBuffer(buffer, fence, config);
if (ret == GSError::GSERROR_OK) {
    // 填充数据（NV12格式）
    void* data = buffer->GetVirAddr();
    auto yPlane = static_cast<uint8_t*>(data);

    // 填充Y平面
    for (int i = 0; i < 1920 * 1080; i++) {
        yPlane[i] = 128;
    }

    // 填充UV平面
    auto uvPlane = yPlane + 1920 * 1080;
    for (int i = 0; i < 1920 * 1080 / 2; i++) {
        uvPlane[i] = 128;
    }

    // 刷新缓冲区
    BufferFlushConfig flushConfig = {
        .damage = {0, 0, 1920, 1080},
        .timestamp = GetCurrentTime()
    };
    producer->FlushBuffer(buffer, fence, flushConfig);
}

// Consumer获取缓冲区
sptr<SurfaceBuffer> consumerBuffer;
sptr<SyncFence> consumerFence;
int64_t timestamp = 0;
Rect damage;

ret = consumerSurface->AcquireBuffer(consumerBuffer, consumerFence, timestamp, damage);
if (ret == GSError::GSERROR_OK) {
    // 处理数据
    void* data = consumerBuffer->GetVirAddr();
    // ... 处理图像数据 ...

    // 释放缓冲区
    consumerSurface->ReleaseBuffer(consumerBuffer, consumerFence);
}
```

### 2. YUV420格式处理

```cpp
// 创建YUV420P (I420) Surface
auto surface = MockSurfaceFactory::CreateIConsumerSurface("yuv420_surface");
surface->SetDefaultWidthAndHeight(640, 480);
surface->SetDefaultFormat(GraphicPixelFormat::PIXEL_FMT_YCBCR_420_P);

auto producer = surface->GetProducer();

sptr<SurfaceBuffer> buffer;
sptr<SyncFence> fence;
BufferRequestConfig config = {
    .width = 640,
    .height = 480,
    .strideAlignment = 8,
    .format = GraphicPixelFormat::PIXEL_FMT_YCBCR_420_P,
    .usage = static_cast<BufferUsage>(BufferUsage::CPU_READ | BufferUsage::CPU_WRITE),
    .timeout = 1000
};

producer->RequestBuffer(buffer, fence, config);

// YUV420P布局: Y平面 + U平面 + V平面
void* data = buffer->GetVirAddr();
auto yPlane = static_cast<uint8_t*>(data);
auto uPlane = yPlane + 640 * 480;           // U平面从Y平面后开始
auto vPlane = uPlane + (640 * 480 / 4);     // V平面从U平面后开始

// 填充Y平面（亮度）
for (int i = 0; i < 640 * 480; i++) {
    yPlane[i] = 128;
}

// 填充U平面（色度）
for (int i = 0; i < 640 * 480 / 4; i++) {
    uPlane[i] = 64;
}

// 填充V平面（色度）
for (int i = 0; i < 640 * 480 / 4; i++) {
    vPlane[i] = 192;
}

BufferFlushConfig flushConfig = {
    .damage = {0, 0, 640, 480},
    .timestamp = 0
};
producer->FlushBuffer(buffer, fence, flushConfig);
```

### 3. 多缓冲区队列

```cpp
// 设置5个缓冲区的队列
auto surface = MockSurfaceFactory::CreateIConsumerSurface("multi_buffer_surface");
surface->SetQueueSize(5);
surface->SetDefaultWidthAndHeight(1280, 720);
surface->SetDefaultFormat(GraphicPixelFormat::PIXEL_FMT_YCBCR_420_SP);

auto producer = surface->GetProducer();

// 请求多个缓冲区（流水线处理）
std::vector<sptr<SurfaceBuffer>> buffers;

for (int i = 0; i < 3; i++) {
    sptr<SurfaceBuffer> buffer;
    sptr<SyncFence> fence;
    BufferRequestConfig config = {
        .width = 1280,
        .height = 720,
        .strideAlignment = 8,
        .format = GraphicPixelFormat::PIXEL_FMT_YCBCR_420_SP,
        .usage = static_cast<BufferUsage>(BufferUsage::CPU_READ | BufferUsage::CPU_WRITE),
        .timeout = 1000
    };

    if (producer->RequestBuffer(buffer, fence, config) == GSError::GSERROR_OK) {
        buffers.push_back(buffer);

        // 异步填充数据
        BufferFlushConfig flushConfig = {
            .damage = {0, 0, 1280, 720},
            .timestamp = i * 1000
        };
        producer->FlushBuffer(buffer, fence, flushConfig);
    }
}

// Consumer获取所有缓冲区
for (int i = 0; i < 3; i++) {
    sptr<SurfaceBuffer> buffer;
    sptr<SyncFence> fence;
    int64_t timestamp = 0;
    Rect damage;

    if (surface->AcquireBuffer(buffer, fence, timestamp, damage) == GSError::GSERROR_OK) {
        // 处理数据
        ProcessBuffer(buffer);

        // 释放缓冲区
        surface->ReleaseBuffer(buffer, fence);
    }
}
```

### 4. 使用元数据和额外数据

```cpp
auto surface = MockSurfaceFactory::CreateIConsumerSurface("metadata_surface");
auto producer = surface->GetProducer();

sptr<SurfaceBuffer> buffer;
sptr<SyncFence> fence;
BufferRequestConfig config = {
    .width = 1920,
    .height = 1080,
    .strideAlignment = 8,
    .format = GraphicPixelFormat::PIXEL_FMT_YCBCR_420_SP,
    .usage = static_cast<BufferUsage>(BufferUsage::CPU_READ | BufferUsage::CPU_WRITE),
    .timeout = 1000
};

producer->RequestBuffer(buffer, fence, config);

// 设置元数据
std::vector<uint8_t> cameraMetadata = {/* 相机元数据 */};
buffer->SetMetadata(100, cameraMetadata);

// 设置额外数据
buffer->GetExtraData()->ExtraSet("timeStamp", 12345678);
buffer->GetExtraData()->ExtraSet("frameNumber", 100);
buffer->GetExtraData()->ExtraSet("exposureTime", 33000);

// 刷新缓冲区
BufferFlushConfig flushConfig = {
    .damage = {0, 0, 1920, 1080},
    .timestamp = 12345678
};
producer->FlushBuffer(buffer, fence, flushConfig);

// Consumer读取元数据
sptr<SurfaceBuffer> consumerBuffer;
sptr<SyncFence> consumerFence;
int64_t timestamp = 0;
Rect damage;

surface->AcquireBuffer(consumerBuffer, consumerFence, timestamp, damage);

// 获取元数据
std::vector<uint8_t> value;
if (consumerBuffer->GetMetadata(100, value) == GSError::GSERROR_OK) {
    // 处理元数据
}

// 获取额外数据
int64_t ts;
consumerBuffer->GetExtraData()->ExtraGet("timeStamp", ts);
int64_t frameNum;
consumerBuffer->GetExtraData()->ExtraGet("frameNumber", frameNum);

surface->ReleaseBuffer(consumerBuffer, consumerFence);
```

## 支持的图像格式

| 格式 | 枚举值 | 描述 | 缓冲区大小计算 |
|------|--------|------|----------------|
| NV12 | `PIXEL_FMT_YCBCR_420_SP` | Y平面 + UV交错 | width × height × 3/2 |
| NV21 | `PIXEL_FMT_YCRCB_420_SP` | Y平面 + VU交错 | width × height × 3/2 |
| YUV420P | `PIXEL_FMT_YCBCR_420_P` | Y + U + V平面 | width × height × 3/2 |
| RGBA8888 | `PIXEL_FMT_RGBA_8888` | RGBA 32位 | width × height × 4 |
| RGBX8888 | `PIXEL_FMT_RGBX_8888` | RGBX 32位 | width × height × 4 |
| RGB565 | `PIXEL_FMT_RGB_565` | RGB 16位 | width × height × 2 |
| RGB888 | `PIXEL_FMT_RGB_888` | RGB 24位 | width × height × 3 |

## 错误码

| 错误码 | 值 | 描述 |
|--------|-----|------|
| `GSERROR_OK` | 0 | 成功 |
| `GSERROR_INVALID_ARGUMENTS` | 1 | 无效参数 |
| `GSERROR_NO_MEM` | 2 | 内存不足 |
| `GSERROR_NO_BUFFER` | 3 | 无可用缓冲区 |
| `GSERROR_INTERNAL` | 4 | 内部错误 |

## 集成测试

### 编译测试

```bash
cd mock
mkdir -p build && cd build
cmake ..
cmake --build .
```

### 运行测试

```bash
# Windows
.\Debug\surface_mock_test.exe

# Linux
./surface_mock_test
```

## 与分布式相机的集成

Surface Mock 可以用于测试以下分布式相机组件：

1. **EncodeDataProcess**：编码器数据处理
2. **DecodeDataProcess**：解码器数据处理
3. **DCameraPhotoSurfaceListener**：拍照Surface监听器
4. **PropertyCarrier**：属性载体

### 示例：测试编码器

```cpp
#include "encode_data_process.h"
#include "surface_mock.h"

// 创建Mock Surface
auto consumerSurface = MockSurfaceFactory::CreateIConsumerSurface("encoder_surface");
consumerSurface->SetDefaultWidthAndHeight(1920, 1080);
consumerSurface->SetDefaultFormat(GraphicPixelFormat::PIXEL_FMT_YCBCR_420_SP);

// 创建编码器
auto encoder = std::make_shared<EncodeDataProcess>();
encoder->SetEncodeProducerSurface(consumerSurface->GetProducer());

// 启动编码器
encoder->StartEncode();

// 模拟输入数据
auto producer = consumerSurface->GetProducer();
for (int i = 0; i < 100; i++) {
    sptr<SurfaceBuffer> buffer;
    sptr<SyncFence> fence;
    BufferRequestConfig config = {/* ... */};

    if (producer->RequestBuffer(buffer, fence, config) == GSError::GSERROR_OK) {
        // 填充数据
        FillYUVData(buffer);

        // 刷新
        BufferFlushConfig flushConfig = {/* ... */};
        producer->FlushBuffer(buffer, fence, flushConfig);
    }
}

// 停止编码器
encoder->StopEncode();
```

## 注意事项

1. **线程安全**：Surface实现是线程安全的，可以在多线程环境中使用
2. **缓冲区重用**：缓冲区会被重用，需要在Acquire后及时Release
3. **格式匹配**：确保Producer和Consumer使用相同的格式配置
4. **队列大小**：默认队列大小为3，可以根据需要调整
5. **生命周期管理**：使用MockSurfaceFactory管理Surface的生命周期

## 清理资源

```cpp
// 销毁Surface
MockSurfaceFactory::DestroySurface(surface);

// 重置工厂（清理所有Surface）
MockSurfaceFactory::Reset();

// 检查活跃Surface数量
uint32_t count = MockSurfaceFactory::GetActiveSurfaceCount();
```
