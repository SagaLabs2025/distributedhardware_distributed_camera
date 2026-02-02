# AVCodec Mock 快速参考

## 编码器快速使用

```cpp
#include "avcodec_mock.h"
using namespace OHOS::MediaAVCodec;

// 创建 H.265 编码器
auto encoder = VideoEncoderFactory::CreateByMime("video/hevc");

// 配置
Format format;
format.PutIntValue("width", 1920);
format.PutIntValue("height", 1080);
format.PutIntValue("pixel_format", static_cast<int32_t>(PixelFormat::NV12));
encoder->Configure(format);

// 创建 Surface（关键接口）
auto surface = encoder->CreateInputSurface();

// 启动
encoder->Prepare();
encoder->Start();
```

## 支持的 MIME 类型

| MIME 类型 | 说明 |
|-----------|------|
| `video/hevc` | H.265/HEVC 编解码器 |
| `video/h265` | H.265 别名 |
| `video/avc` | H.264/AVC 编解码器 |
| `video/h264` | H.264 别名 |
| `image/jpeg` | JPEG 编码器 |

## 配置参数

```cpp
// 必须配置的参数
format.PutIntValue("width", 1920);              // 视频宽度
format.PutIntValue("height", 1080);             // 视频高度
format.PutIntValue("pixel_format", PixelFormat::NV12);  // 像素格式

// 可选参数
format.PutDoubleValue("frame_rate", 30.0);      // 帧率，默认 30.0
format.PutIntValue("bitrate", 5000000);         // 比特率，默认 5Mbps
```

## 回调实现

```cpp
class MyCallback : public MediaCodecCallback {
    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override {
        auto attr = buffer->GetBufferAttr();
        uint8_t* data = buffer->GetAddr();
        uint32_t size = attr.size;
        int64_t pts = attr.presentationTimeUs;

        // 处理编码数据
        ProcessData(data, size, pts);
    }

    void OnError(AVCodecErrorType errorType, int32_t errorCode) override {
        // 处理错误
    }

    void OnOutputFormatChanged(const Format& format) override {
        // 处理格式变化
    }

    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override {
        // Surface 模式下不需要处理
    }
};
```

## 测试辅助方法

```cpp
// 模拟编码输出
std::vector<uint8_t> data = {0x00, 0x00, 0x00, 0x01, 0x20};
encoder->SimulateEncodedOutput(0, data, 0);

// 模拟错误
encoder->SimulateError(AVCodecErrorType::ERROR_CODEC, -1);

// 查询状态
bool config = encoder->IsConfigured();
bool prepared = encoder->IsPrepared();
bool started = encoder->IsStarted();
int32_t w = encoder->GetConfigWidth();
int32_t h = encoder->GetConfigHeight();
```

## 生命周期

```
Released
    ↓ Configure
Configured
    ↓ CreateInputSurface (可选)
    ↓ Prepare
Prepared
    ↓ Start
Executing
    ↓ Stop
Prepared
    ↓ Release
Released
```

## 文件清单

| 文件 | 说明 |
|------|------|
| `include/avcodec_mock.h` | 头文件，408 行 |
| `src/avcodec_mock.cpp` | 实现文件，882 行 |
| `test/avcodec_test.cpp` | 测试文件，356 行 |
| `AVCODEC_MOCK_USAGE.md` | 详细使用文档 |
| `AVCODEC_MOCK_SUMMARY.md` | 实现总结文档 |

## 构建命令

```bash
# CMake 构建
cd mock/build
cmake ..
cmake --build . --target avcodec_test

# 运行测试
./avcodec_test
```
