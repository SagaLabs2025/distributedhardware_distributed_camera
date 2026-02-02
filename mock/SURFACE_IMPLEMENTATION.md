# Surface Mock 实现总结

## 实现概述

本次任务成功实现了 OpenHarmony Surface 接口的完整 Mock 功能，用于分布式相机项目的测试和开发。

## 创建的文件

### 1. 头文件
**文件路径**: `c:\Work\OpenHarmony\distributedhardware_distributed_camera\mock\include\surface_mock.h`

包含以下核心类的定义：

- **GSError / SurfaceError**: 图形错误码枚举
- **GraphicPixelFormat**: 图像像素格式枚举（NV12、YUV420P、RGBA等）
- **BufferUsage**: 缓冲区用途配置枚举
- **BufferRequestConfig / BufferFlushConfig**: 缓冲区请求和刷新配置
- **BufferHandle**: 缓冲区句柄结构
- **Rect**: 矩形区域定义
- **SyncFence**: 同步栅栏类
- **SurfaceBufferExtraData**: 缓冲区额外数据类
- **SurfaceBuffer**: Surface缓冲区实现
- **IBufferProducer**: 缓冲区生产者接口
- **IBufferConsumerListener**: 缓冲区消费者监听器
- **IConsumerSurface**: 消费者Surface接口
- **Surface**: 主Surface类
- **MockSurfaceFactory**: Surface工厂类

### 2. 实现文件
**文件路径**: `c:\Work\OpenHarmony\distributedhardware_distributed_camera\mock\src\surface_mock.cpp`

实现了所有Surface相关类的功能，包括：

- **SyncFence**: 同步栅栏的基本操作
- **SurfaceBufferExtraData**: 额外数据的存储和检索
- **SurfaceBuffer**: 缓冲区分配、内存管理、元数据操作
- **Surface::BufferProducerProxy**: 生产者代理实现
- **Surface**: 完整的生产者-消费者队列管理
- **ConsumerSurfaceImpl**: 消费者Surface包装实现
- **MockSurfaceFactory**: Surface工厂和生命周期管理

### 3. 测试文件
**文件路径**: `c:\Work\OpenHarmony\distributedhardware_distributed_camera\mock\test\surface_mock_test.cpp`

包含以下测试用例：

- **TestProducerConsumerPattern**: 测试基本的生产者-消费者模式
- **TestYUV420Format**: 测试YUV420格式缓冲区
- **TestMultipleBuffers**: 测试多缓冲区队列
- **TestMetadata**: 测试元数据功能

### 4. 文档文件
**文件路径**: `c:\Work\OpenHarmony\distributedhardware_distributed_camera\mock\README_SURFACE.md`

包含完整的使用指南，涵盖：
- 架构设计说明
- 使用示例代码
- 支持的图像格式
- 错误码定义
- 与分布式相机的集成方法

## 核心功能特性

### 1. 零拷贝Buffer队列模拟
- 使用智能指针（sptr）管理缓冲区生命周期
- 避免数据拷贝，直接传递缓冲区指针
- 支持缓冲区重用机制

### 2. Producer-Consumer模式
```
生产者流程：
RequestBuffer → 获取空闲缓冲区
           → 填充数据
           → FlushBuffer → 提交到已填充队列

消费者流程：
OnBufferAvailable → 接收通知
              → AcquireBuffer → 获取已填充缓冲区
              → 处理数据
              → ReleaseBuffer → 释放回空闲队列
```

### 3. 支持的图像格式
| 格式 | 大小计算 | 说明 |
|------|----------|------|
| NV12 | width × height × 3/2 | Y + UV交错 |
| NV21 | width × height × 3/2 | Y + VU交错 |
| YUV420P | width × height × 3/2 | Y + U + V平面 |
| RGBA8888 | width × height × 4 | 32位RGBA |
| RGB565 | width × height × 2 | 16位RGB |

### 4. 线程安全
- 使用 `std::mutex` 保护队列操作
- 使用 `std::condition_variable` 实现等待/通知机制
- 支持多线程生产者和消费者

### 5. 元数据支持
- 缓冲区元数据（键值对存储）
- 额外数据（时间戳、帧号等）
- 支持自定义数据扩展

## 构建配置

### CMakeLists.txt 更新
```cmake
# 添加了以下文件到构建系统
set(MOCK_SOURCES
    ...
    src/surface_mock.cpp
)

set(MOCK_HEADERS
    ...
    include/surface_mock.h
)

# 添加了测试可执行文件
add_executable(surface_mock_test test/surface_mock_test.cpp)
target_link_libraries(surface_mock_test distributed_camera_mock)
```

## 与分布式相机的集成

### 兼容的组件
1. **EncodeDataProcess**: 编码器数据处理
   - 文件: `services/data_process/src/pipeline_node/multimedia_codec/encoder/encode_data_process.cpp`
   - 使用: `RequestBuffer`、`FlushBuffer`

2. **DecodeDataProcess**: 解码器数据处理
   - 文件: `services/data_process/src/pipeline_node/multimedia_codec/decoder/decode_data_process.cpp`
   - 使用: `AcquireBuffer`、`ReleaseBuffer`

3. **DCameraPhotoSurfaceListener**: 拍照监听器
   - 文件: `services/cameraservice/cameraoperator/client/src/listener/dcamera_photo_surface_listener.cpp`
   - 使用: `IConsumerSurface`、`OnBufferAvailable`

4. **DecodeSurfaceListener**: 解码Surface监听器
   - 文件: `services/data_process/src/pipeline_node/multimedia_codec/decoder/decode_surface_listener.cpp`
   - 使用: `IConsumerSurface`、缓冲区获取

### 使用示例

```cpp
// 创建Surface用于编码器
auto surface = MockSurfaceFactory::CreateIConsumerSurface("encoder");
surface->SetDefaultWidthAndHeight(1920, 1080);
surface->SetDefaultFormat(GraphicPixelFormat::PIXEL_FMT_YCBCR_420_SP);

// 注册到编码器
auto producer = surface->GetProducer();
encoder->SetEncodeProducerSurface(producer);

// 注册监听器
auto listener = std::make_shared<DecodeSurfaceListener>(surface, decoder);
surface->RegisterConsumerListener(listener);
```

## 测试验证

### 编译测试
```bash
cd mock/build
cmake --build .
```

### 运行测试
```bash
# Windows
.\Debug\surface_mock_test.exe

# Linux
./surface_mock_test
```

### 预期输出
```
Surface Mock Test Suite
=======================

=== Test Producer-Consumer Pattern ===
[Producer] Buffer requested successfully
  Buffer size: 3110400 bytes
  Width: 1920
  Height: 1080
[Producer] Data filled
[Producer] Extra data set
[Producer] Buffer flushed
[Consumer] Buffer acquired
  Timestamp: 12345678
  Damage: x=0 y=0 w=1920 h=1080
  First pixel value: 128
  Extra timestamp: 12345678
[Consumer] Buffer released
Buffer available notifications: 1

=== Test YUV420 Format ===
YUV420 buffer size: 460800 bytes
Expected size: 460800 bytes
YUV420 buffer flushed

=== Test Multiple Buffer Queue ===
Buffer 0 requested, size: 1382400 bytes
Buffer 0 flushed
Buffer 1 requested, size: 1382400 bytes
Buffer 1 flushed
Buffer 2 requested, size: 1382400 bytes
Buffer 2 flushed
Buffer 0 acquired, timestamp: 0
Buffer 0 released
Buffer 1 acquired, timestamp: 1000
Buffer 1 released
Buffer 2 acquired, timestamp: 2000
Buffer 2 released
Active surface count: 4

=== Test Metadata ===
Metadata set for keys 100 and 200
Metadata 100: 1 2 3 4 5
Metadata 200: 10 20 30 40

=======================
All tests completed!
Factory reset, active surfaces: 0
```

## 技术要点

### 1. 智能指针使用
```cpp
template<typename T>
using sptr = std::shared_ptr<T>;

template<typename T>
using wp = std::weak_ptr<T>;
```

### 2. 缓冲区队列管理
```cpp
struct BufferQueueItem {
    sptr<SurfaceBuffer> buffer;
    sptr<SyncFence> fence;
    int64_t timestamp;
    Rect damage;
    bool inUse;
};

std::queue<BufferQueueItem> freeQueue_;   // 空闲缓冲区队列
std::queue<BufferQueueItem> filledQueue_; // 已填充缓冲区队列
std::vector<BufferQueueItem> allBuffers_; // 所有缓冲区列表
```

### 3. 格式计算
```cpp
switch (format_) {
    case GraphicPixelFormat::PIXEL_FMT_YCBCR_420_SP:  // NV12
        bufferSize = width_ * height_ * 3 / 2;
        break;
    case GraphicPixelFormat::PIXEL_FMT_RGBA_8888:
        bufferSize = width_ * height_ * 4;
        break;
    // ...
}
```

## 未来扩展

### 可能的改进方向
1. **硬件加速支持**: 模拟GPU缓冲区和硬件编码器
2. **跨进程支持**: 实现真正的跨进程Buffer共享
3. **性能优化**: 添加缓冲区池和预分配机制
4. **更多格式**: 支持P010、RGBA1010102等高级格式
5. **Fence实现**: 完整的同步栅栏实现

### 建议的后续工作
1. 添加性能基准测试
2. 实现压力测试用例
3. 添加内存泄漏检测
4. 完善错误处理和边界条件

## 总结

本次实现的Surface Mock提供了：
- 完整的OpenHarmony Surface接口模拟
- 零拷贝的缓冲区管理
- 线程安全的生产者-消费者模式
- 支持主流图像格式（NV12、YUV420等）
- 完善的测试用例和文档

该实现可以满足分布式相机项目的测试需求，特别是编码器和解码器数据处理模块的单元测试和集成测试。
