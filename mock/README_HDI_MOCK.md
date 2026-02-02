# HDF/HDI接口Mock实现

## 概述

本模块实现了分布式相机HDF驱动和HDI接口的Mock功能，用于单元测试和集成测试。

## 核心功能

### 1. 虚拟相机HDF设备模拟

- **EnableDCameraDevice**: 使能分布式相机设备
- **DisableDCameraDevice**: 去使能分布式相机设备
- **AcquireBuffer**: 获取显示缓冲区（零拷贝）
- **ShutterBuffer**: 提交填充后的缓冲区（零拷贝）

### 2. 三通道流配置

支持三种通道类型的模拟：

| 通道类型 | 流ID | 分辨率 | 编码格式 | 用途 |
|---------|------|--------|---------|------|
| Control通道 | 0 | N/A | N/A | 控制命令传输 |
| Snapshot通道 | 1 | 4096×3072 | JPEG | 高分辨率拍照 |
| Continuous通道 | 2 | 1920×1080 | H.265 | 实时视频流 |

### 3. 零拷贝数据传输

- 直接内存访问，避免数据拷贝
- 基于vector的内存管理
- 自动化的缓冲区生命周期管理

## 类结构

### MockHdiProvider

HDI Provider接口的Mock实现，提供以下功能：

```cpp
class MockHdiProvider : public IDCameraProvider {
    // 基本操作
    int32_t EnableDCameraDevice(const DHBase& dhBase,
                                const std::string& abilityInfo,
                                const std::shared_ptr<IDCameraProviderCallback>& callbackObj);
    int32_t DisableDCameraDevice(const DHBase& dhBase);
    int32_t AcquireBuffer(const DHBase& dhBase, int streamId, DCameraBuffer& buffer);
    int32_t ShutterBuffer(const DHBase& dhBase, int streamId, const DCameraBuffer& buffer);

    // 测试控制
    void SetEnableResult(int32_t result);
    void SetAcquireBufferResult(int32_t result);
    void SetShutterBufferResult(int32_t result);

    // 触发回调
    int32_t TriggerOpenSession(const DHBase& dhBase);
    int32_t TriggerConfigureStreams(const DHBase& dhBase,
                                    const std::vector<DCStreamInfo>& streamInfos);
    int32_t TriggerStartCapture(const DHBase& dhBase,
                                const std::vector<DCCaptureInfo>& captureInfos);

    // 状态查询
    bool IsDeviceEnabled(const std::string& dhId) const;
    size_t GetActiveStreamCount() const;
    size_t GetBufferAcquireCount() const;
};
```

### MockProviderCallback

HDI Provider回调接口的Mock实现：

```cpp
class MockProviderCallback : public IDCameraProviderCallback {
    // 回调验证
    bool IsSessionOpen() const;
    bool IsStreamsConfigured() const;
    bool IsCaptureStarted() const;

    // 统计信息
    size_t GetOpenSessionCount() const;
    size_t GetConfigureStreamsCount() const;
    size_t GetStartCaptureCount() const;

    // 获取最后配置
    std::vector<DCStreamInfo> GetLastStreamInfos() const;
    std::vector<DCCaptureInfo> GetLastCaptureInfos() const;
};
```

### TripleStreamConfig

三通道流配置工具类：

```cpp
class TripleStreamConfig {
    // 创建默认三通道流配置
    static std::vector<DCStreamInfo> CreateDefaultTripleStreams();

    // 创建自定义分辨率的三通道流
    static std::vector<DCStreamInfo> CreateCustomTripleStreams(
        int snapshotWidth, int snapshotHeight,
        int continuousWidth, int continuousHeight);

    // 单独创建各通道
    static DCStreamInfo CreateControlStream();
    static DCStreamInfo CreateSnapshotStream(int width, int height);
    static DCStreamInfo CreateContinuousStream(int width, int height);
};
```

### ZeroCopyBufferManager

零拷贝缓冲区管理器：

```cpp
class ZeroCopyBufferManager {
    // 创建/获取缓冲区
    DCameraBuffer CreateBuffer(size_t size);
    DCameraBuffer AcquireBuffer(int streamId, size_t size);
    int32_t ReleaseBuffer(const DCameraBuffer& buffer);

    // 零拷贝数据访问
    void* GetBufferData(const DCameraBuffer& buffer);
    int32_t SetBufferData(const DCameraBuffer& buffer, const void* data, size_t size);

    // 统计信息
    size_t GetActiveBufferCount() const;
    size_t GetTotalAllocatedSize() const;
};
```

## 使用示例

### 基本使用流程

```cpp
#include "hdi_mock.h"

using namespace OHOS::DistributedHardware;

// 1. 获取Mock实例
auto mockProvider = MockHdiProvider::GetInstance();
auto mockCallback = std::make_shared<MockProviderCallback>();

// 2. 配置分布式硬件基础信息
DHBase dhBase;
dhBase.deviceId_ = "network_id_001";
dhBase.dhId_ = "dh_id_001";

// 3. 使能虚拟相机设备
std::string abilityInfo = "{}";  // JSON格式的相机能力
int32_t result = mockProvider->EnableDCameraDevice(dhBase, abilityInfo, mockCallback);

// 4. 配置三通道流
auto streamInfos = TripleStreamConfig::CreateDefaultTripleStreams();
mockProvider->TriggerConfigureStreams(dhBase, streamInfos);

// 5. 开始捕获
std::vector<DCCaptureInfo> captureInfos = {/* ... */};
mockProvider->TriggerStartCapture(dhBase, captureInfos);

// 6. 获取和提交缓冲区（零拷贝）
DCameraBuffer buffer;
mockProvider->AcquireBuffer(dhBase, TripleStreamConfig::CONTINUOUS_STREAM_ID, buffer);

// 直接操作内存（零拷贝）
uint8_t* data = static_cast<uint8_t*>(buffer.virAddr_);
// ... 填充数据 ...

mockProvider->ShutterBuffer(dhBase, TripleStreamConfig::CONTINUOUS_STREAM_ID, buffer);

// 7. 停止捕获和清理
std::vector<int> streamIds = {1, 2};
mockProvider->TriggerStopCapture(dhBase, streamIds);
mockProvider->DisableDCameraDevice(dhBase);
```

### 自定义流配置

```cpp
// 创建自定义分辨率的三通道流
auto streamInfos = TripleStreamConfig::CreateCustomTripleStreams(
    1920, 1080,   // Snapshot: 1920x1080
    1280, 720     // Continuous: 1280x720
);

// 或者单独创建每个流
DCStreamInfo controlStream = TripleStreamConfig::CreateControlStream();
DCStreamInfo snapshotStream = TripleStreamConfig::CreateSnapshotStream(2048, 1536);
DCStreamInfo continuousStream = TripleStreamConfig::CreateContinuousStream(1280, 720);

std::vector<DCStreamInfo> customStreams = {
    controlStream,
    snapshotStream,
    continuousStream
};
```

### 零拷贝缓冲区管理

```cpp
auto& bufferMgr = ZeroCopyBufferManager::GetInstance();

// 创建缓冲区
size_t bufferSize = 1920 * 1080 * 3 / 2;  // NV12格式
DCameraBuffer buffer = bufferMgr.CreateBuffer(bufferSize);

// 设置数据
std::vector<uint8_t> testData(bufferSize, 0x42);
bufferMgr.SetBufferData(buffer, testData.data(), testData.size());

// 获取数据指针（零拷贝访问）
void* data = bufferMgr.GetBufferData(buffer);
if (data) {
    uint8_t* byteData = static_cast<uint8_t*>(data);
    // 直接操作内存...
}

// 释放缓冲区
bufferMgr.ReleaseBuffer(buffer);
```

### 测试验证

```cpp
// 验证设备状态
bool isEnabled = mockProvider->IsDeviceEnabled("dh_id_001");
size_t streamCount = mockProvider->GetActiveStreamCount();

// 验证回调调用
size_t openCount = mockCallback->GetOpenSessionCount();
size_t configCount = mockCallback->GetConfigureStreamsCount();
bool isConfigured = mockCallback->IsStreamsConfigured();

// 获取最后配置的流信息
auto lastStreams = mockCallback->GetLastStreamInfos();
for (const auto& stream : lastStreams) {
    std::cout << "Stream ID: " << stream.streamId_ << std::endl;
    std::cout << "Resolution: " << stream.width_ << "x" << stream.height_ << std::endl;
}

// 验证缓冲区操作
size_t acquireCount = mockProvider->GetBufferAcquireCount();
size_t shutterCount = mockProvider->GetBufferShutterCount();
```

## 数据结构

### DCamRetCode - 返回码

```cpp
enum class DCamRetCode : int32_t {
    SUCCESS = 0,
    CAMERA_BUSY = 1,
    INVALID_ARGUMENT = 2,
    METHOD_NOT_SUPPORTED = 3,
    CAMERA_OFFLINE = 4,
    EXCEED_MAX_NUMBER = 5,
    DEVICE_NOT_INIT = 6,
    FAILED = 7,
};
```

### DCEncodeType - 编码类型

```cpp
enum class DCEncodeType : int32_t {
    ENCODE_TYPE_NULL = 0,
    ENCODE_TYPE_H264 = 1,
    ENCODE_TYPE_H265 = 2,
    ENCODE_TYPE_JPEG = 3,
    ENCODE_TYPE_MPEG4_ES = 4,
};
```

### DCStreamType - 流类型

```cpp
enum class DCStreamType : int32_t {
    CONTINUOUS_FRAME = 0,  // 连续捕获流
    SNAPSHOT_FRAME = 1,    // 单次捕获流
};
```

### DCStreamInfo - 流信息

```cpp
struct DCStreamInfo {
    int streamId_;              // 流ID
    int width_;                 // 图像宽度
    int height_;                // 图像高度
    int stride_;                // 图像步长
    int format_;                // 图像格式
    int dataspace_;             // 色彩空间
    DCEncodeType encodeType_;   // 编码类型
    DCStreamType type_;         // 流类型
};
```

### DCameraBuffer - 相机缓冲区

```cpp
struct DCameraBuffer {
    int index_;                 // 缓冲区索引
    unsigned int size_;         // 缓冲区大小
    BufferHandle* bufferHandle_; // Native缓冲区句柄
    void* virAddr_;             // 虚拟地址（零拷贝使用）
};
```

## 编译和测试

### 使用GN构建

```bash
# 在OpenHarmony根目录下
./build.sh --product-name {product_name} --build-target distributed_camera_mock
```

### 使用CMake构建（独立测试）

```bash
cd mock
mkdir build && cd build
cmake ..
cmake --build .

# 运行测试
./hdi_mock_test
```

## 测试用例

`hdi_mock_test.cpp` 包含以下测试用例：

1. **TestEnableDCameraDevice**: 测试虚拟相机设备使能
2. **TestConfigureTripleStreams**: 测试三通道流配置
3. **TestAcquireAndShutterBuffer**: 测试零拷贝缓冲区操作
4. **TestCaptureWorkflow**: 测试完整捕获流程
5. **TestZeroCopyBufferManager**: 测试零拷贝缓冲区管理器
6. **TestCustomStreamConfiguration**: 测试自定义流配置
7. **TestErrorScenarios**: 测试错误场景处理

## 注意事项

1. **线程安全**: Mock实现使用了互斥锁保护共享状态，支持多线程测试
2. **资源管理**: 使用完毕后建议调用`Reset()`清理状态
3. **零拷贝**: 缓冲区通过`virAddr_`指针直接访问，使用时需注意生命周期
4. **错误模拟**: 可以通过`SetXxxResult()`方法设置特定操作的返回值来测试错误处理

## 文件位置

- 头文件: `mock/include/hdi_mock.h`
- 源文件: `mock/src/hdi_mock.cpp`
- 测试文件: `mock/test/hdi_mock_test.cpp`
