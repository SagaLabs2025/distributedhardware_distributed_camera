# AVCodec Mock 功能使用说明

## 概述

AVCodec Mock 模拟了 OpenHarmony 的 `OHOS::MediaAVCodec` 接口，提供视频编码器和解码器的 Mock 实现，支持：

1. **H.265/HEVC 视频编码器** - 模拟 Sink 端编码场景
2. **H.264/AVC 视频编码器** - 兼容性支持
3. **H.265/HEVC 视频解码器** - 模拟 Source 端解码场景
4. **Surface 模式** - 支持与 Camera Framework 的无缝对接
5. **回调机制** - 支持 AVCodecCallback 和 MediaCodecCallback 两种风格

## 文件结构

```
mock/
├── include/
│   └── avcodec_mock.h          # AVCodec Mock 头文件
├── src/
│   └── avcodec_mock.cpp        # AVCodec Mock 实现
├── test/
│   └── avcodec_test.cpp        # AVCodec Mock 测试用例
└── CMakeLists.txt              # 已更新包含新文件
```

## 核心接口

### 1. 编码器工厂

```cpp
namespace OHOS::MediaAVCodec {

class VideoEncoderFactory {
public:
    // 通过 MIME 类型创建编码器
    static std::shared_ptr<AVCodecVideoEncoder> CreateByMime(const std::string& mime);

    // 通过名称创建编码器
    static std::shared_ptr<AVCodecVideoEncoder> CreateByName(const std::string& name);
};

}
```

**支持的 MIME 类型：**
- `"video/hevc"` - H.265 编码器
- `"video/h265"` - H.265 编码器（别名）
- `"video/avc"` - H.264 编码器
- `"image/jpeg"` - JPEG 编码器

### 2. AVCodecVideoEncoder 接口

```cpp
class AVCodecVideoEncoder {
public:
    // 配置编码器
    virtual int32_t Configure(const Format &format) = 0;

    // 准备编码器
    virtual int32_t Prepare() = 0;

    // 启动编码器
    virtual int32_t Start() = 0;

    // 停止编码器
    virtual int32_t Stop() = 0;

    // 刷新编码器
    virtual int32_t Flush() = 0;

    // 通知流结束
    virtual int32_t NotifyEos() = 0;

    // 释放编码器
    virtual int32_t Release() = 0;

    // 创建输入 Surface（Surface 模式）- 关键接口
    virtual sptr<Surface> CreateInputSurface() = 0;

    // 注册回调
    virtual int32_t SetCallback(const std::shared_ptr<MediaCodecCallback> &callback) = 0;
};
```

### 3. MediaCodecCallback 回调接口

```cpp
class MediaCodecCallback {
public:
    // 错误回调
    virtual void OnError(AVCodecErrorType errorType, int32_t errorCode) = 0;

    // 输出格式变化回调
    virtual void OnOutputFormatChanged(const Format &format) = 0;

    // 输入 Buffer 可用回调（Surface 模式下不需要处理）
    virtual void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) = 0;

    // 输出 Buffer 可用回调 - 获取编码后的 H.265 数据
    virtual void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) = 0;
};
```

### 4. Format 配置类

```cpp
class Format {
public:
    // 设置配置参数
    void PutIntValue(const std::string& key, int32_t value);
    void PutDoubleValue(const std::string& key, double value);
    void PutStringValue(const std::string& key, const std::string& value);

    // 获取配置参数
    int32_t GetIntValue(const std::string& key, int32_t defaultValue) const;
    double GetDoubleValue(const std::string& key, double defaultValue) const;
    std::string GetStringValue(const std::string& key, const std::string& defaultValue) const;
};
```

## 使用示例

### 基础编码器使用流程

```cpp
#include "avcodec_mock.h"
using namespace OHOS::MediaAVCodec;

// 1. 创建 H.265 编码器
auto encoder = VideoEncoderFactory::CreateByMime("video/hevc");
if (encoder == nullptr) {
    // 创建失败处理
    return -1;
}

// 2. 配置编码器
Format format;
format.PutIntValue("width", 1920);              // 必须配置
format.PutIntValue("height", 1080);             // 必须配置
format.PutIntValue("pixel_format", static_cast<int32_t>(PixelFormat::NV12));  // 必须配置
format.PutDoubleValue("frame_rate", 30.0);      // 可选，默认 30.0
format.PutIntValue("bitrate", 5000000);         // 可选，默认 5000000 (5Mbps)

int32_t result = encoder->Configure(format);
if (result != 0) {
    // 配置失败处理
    return -1;
}

// 3. 创建输入 Surface（关键：在 Prepare 之前）
sptr<Surface> inputSurface = encoder->CreateInputSurface();
if (inputSurface == nullptr) {
    // Surface 创建失败处理
    return -1;
}

// 4. 准备并启动编码器
result = encoder->Prepare();
if (result != 0) {
    // 准备失败处理
    return -1;
}

result = encoder->Start();
if (result != 0) {
    // 启动失败处理
    return -1;
}

// 5. inputSurface 可传递给 Camera Framework 使用
//    Camera Framework 输出的 YUV 数据会自动进入编码器

// ... 运行过程 ...

// 6. 停止并释放编码器
encoder->Stop();
encoder->Release();
```

### 完整的回调使用示例

```cpp
#include "avcodec_mock.h"
using namespace OHOS::MediaAVCodec;

// 定义回调类
class MyEncoderCallback : public MediaCodecCallback {
public:
    void OnError(AVCodecErrorType errorType, int32_t errorCode) override {
        // 处理编码错误
        printf("Encoder error: type=%d, code=%d\n", errorType, errorCode);
    }

    void OnOutputFormatChanged(const Format& format) override {
        // 处理输出格式变化
        printf("Output format changed\n");
    }

    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override {
        // Surface 模式下不需要处理输入 Buffer
    }

    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override {
        // 获取编码后的 H.265 数据
        auto attr = buffer->GetBufferAttr();
        uint8_t* data = buffer->GetAddr();
        int64_t pts = attr.presentationTimeUs;
        uint32_t size = attr.size;

        // 处理编码数据（通过通道发送给 Source 端）
        ProcessEncodedData(data, size, pts);

        // 释放 Buffer
        encoder_->ReleaseOutputBuffer(index);
    }

    void SetEncoder(std::shared_ptr<AVCodecVideoEncoder> encoder) {
        encoder_ = encoder;
    }

private:
    std::shared_ptr<AVCodecVideoEncoder> encoder_;
};

// 使用回调
int main() {
    // 创建编码器
    auto encoder = VideoEncoderFactory::CreateByMime("video/hevc");

    // 创建并注册回调
    auto callback = std::make_shared<MyEncoderCallback>();
    callback->SetEncoder(encoder);
    encoder->SetCallback(callback);

    // 配置编码器
    Format format;
    format.PutIntValue("width", 1920);
    format.PutIntValue("height", 1080);
    format.PutIntValue("pixel_format", static_cast<int32_t>(PixelFormat::NV12));

    encoder->Configure(format);

    // 创建 Surface
    auto surface = encoder->CreateInputSurface();

    // 准备并启动
    encoder->Prepare();
    encoder->Start();

    // ... 运行过程 ...

    // 清理
    encoder->Stop();
    encoder->Release();

    return 0;
}
```

### 测试辅助方法

AVCodec Mock 提供了测试辅助方法，可以模拟编码器输出：

```cpp
// 模拟输出编码数据（用于测试）
std::vector<uint8_t> mockH265Data = {
    0x00, 0x00, 0x00, 0x01, 0x20, 0x01, 0x00, 0x01  // H.265 NAL 头
};
encoder->SimulateEncodedOutput(0, mockH265Data, 0);

// 模拟错误（用于测试）
encoder->SimulateError(AVCodecErrorType::ERROR_CODEC, -1);

// 查询编码器状态
bool isConfigured = encoder->IsConfigured();
bool isPrepared = encoder->IsPrepared();
bool isStarted = encoder->IsStarted();
int32_t width = encoder->GetConfigWidth();
int32_t height = encoder->GetConfigHeight();
```

### 与 Camera Framework 集成示例

```cpp
// Sink 端完整集成示例

// 1. 创建 AVCodec 编码器并获取 Surface
auto encoder = VideoEncoderFactory::CreateByMime("video/hevc");

Format format;
format.PutIntValue("width", 1920);
format.PutIntValue("height", 1080);
format.PutIntValue("pixel_format", static_cast<int32_t>(PixelFormat::NV12));

encoder->Configure(format);
auto encoderSurface = encoder->CreateInputSurface();

// 2. 创建 Camera Framework 会话
auto cameraManager = OHOS::Camera::CameraManager::GetInstance();
auto cameraInput = cameraManager->CreateCameraInput(cameraId);
auto captureSession = std::make_shared<OHOS::Camera::CaptureSession>();

// 3. 创建 PreviewOutput 并绑定编码 Surface
auto previewSurface = Surface::CreateSurface();
previewSurface->SetNativeWindow(encoderSurface);  // 绑定
auto previewOutput = OHOS::Camera::PreviewOutput::Create(previewSurface);

// 4. 三阶段提交配置
captureSession->BeginConfig();
captureSession->AddInput(cameraInput);
captureSession->AddOutput(previewOutput);
captureSession->CommitConfig();

// 5. 准备并启动编码器
encoder->Prepare();
encoder->Start();

// 6. 启动相机会话
captureSession->Start();

// 7. 相机开始输出 YUV 数据，自动通过 Surface 进入编码器
// 8. 编码完成后，OnOutputBufferAvailable 回调被触发

// ... 运行过程 ...

// 9. 清理
captureSession->Stop();
encoder->Stop();
encoder->Release();
```

## 解码器使用

解码器的使用方式与编码器类似：

```cpp
// 创建 H.265 解码器
auto decoder = VideoDecoderFactory::CreateByMime("video/hevc");

// 配置解码器
Format format;
format.PutIntValue("width", 1920);
format.PutIntValue("height", 1080);
format.PutIntValue("pixel_format", static_cast<int32_t>(PixelFormat::NV12));

decoder->Configure(format);

// 注册回调
auto callback = std::make_shared<MyDecoderCallback>();
decoder->SetCallback(callback);

// 准备并启动
decoder->Prepare();
decoder->Start();

// 提交编码数据
uint32_t bufferIndex;
decoder->QueryInputBuffer(bufferIndex, 10000);
auto buffer = decoder->GetInputBuffer(bufferIndex);
buffer->SetData(encodedData, encodedSize);
decoder->QueueInputBuffer(bufferIndex, info, flag);

// 接收解码后的 YUV 数据在回调中处理
```

## 编译和测试

### 编译 Mock 库和测试程序

```bash
cd mock/build
cmake ..
cmake --build . --config Debug
```

### 运行测试程序

```bash
# Windows
.\Debug\avcodec_test.exe

# Linux
./avcodec_test
```

## 支持的配置参数

### 编码器配置

| 参数名 | 类型 | 必需 | 默认值 | 说明 |
|--------|------|------|--------|------|
| `width` | int32_t | 是 | 1920 | 视频宽度 |
| `height` | int32_t | 是 | 1080 | 视频高度 |
| `pixel_format` | int32_t | 是 | NV12 | 像素格式 |
| `frame_rate` | double | 否 | 30.0 | 帧率 |
| `bitrate` | int32_t | 否 | 5000000 | 比特率 (bps) |

### 支持的像素格式

- `PixelFormat::NV12` - YUV 420 semi-planar
- `PixelFormat::NV21` - YUV 420 semi-planar (交换)
- `PixelFormat::YUV420` - YUV 420 planar
- `PixelFormat::RGBA_8888` - RGBA 8-bit
- `PixelFormat::JPEG` - JPEG 格式

## 注意事项

1. **Surface 模式 vs Buffer 模式**
   - Surface 模式：使用 `CreateInputSurface()`，适合与 Camera Framework 集成
   - Buffer 模式：使用 `QueueInputBuffer()`，适合手动提交数据

2. **状态机**
   - Configure → Prepared → Started
   - 必须按顺序调用，不能跳过

3. **回调注册时机**
   - 建议在 `Configure()` 之前注册回调
   - 支持 `AVCodecCallback` 和 `MediaCodecCallback` 两种风格

4. **线程安全**
   - 所有接口都是线程安全的
   - 使用内部互斥锁保护状态

## 限制

- Mock 实现不进行真实的编解码操作
- Surface 仅供模拟，不涉及真实的图形缓冲区
- 适用于单元测试和集成测试场景
