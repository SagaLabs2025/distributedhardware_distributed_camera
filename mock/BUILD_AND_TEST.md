# Surface Mock 构建和测试指南

## 快速开始

### 1. 构建项目

#### Windows (Visual Studio)
```bash
cd mock
mkdir build
cd build
cmake ..
cmake --build . --config Debug
```

#### Linux (GCC/Clang)
```bash
cd mock
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### 2. 运行测试

#### Windows
```bash
# 进入build目录
cd mock\build\Debug

# 运行Surface测试
.\surface_mock_test.exe
```

#### Linux
```bash
# 进入build目录
cd mock/build

# 运行Surface测试
./surface_mock_test
```

## 预期输出

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
...

=======================
All tests completed!
```

## 故障排除

### 编译错误

#### 问题: 找不到 `surface_mock.h`
**解决方案**: 确保在 `mock` 目录下运行 cmake，并且 include 目录存在。

#### 问题: 链接错误
**解决方案**: 检查 CMakeLists.txt 中是否正确添加了 surface_mock.cpp 到源文件列表。

### 运行时错误

#### 问题: 测试程序崩溃
**解决方案**:
1. 检查是否有足够的内存分配
2. 确认线程库正确链接
3. 使用调试器定位具体崩溃位置

#### 问题: 测试输出不符合预期
**解决方案**:
1. 确认队列大小设置正确（默认为3）
2. 检查缓冲区格式匹配
3. 验证生产者和消费者使用相同的配置

## 集成到现有项目

### 在测试代码中使用

```cpp
// 在你的测试文件中
#include "surface_mock.h"

TEST(DistributedCameraTest, SurfaceMockTest) {
    // 创建Consumer Surface
    auto surface = MockSurfaceFactory::CreateIConsumerSurface("test");
    surface->SetDefaultWidthAndHeight(1920, 1080);
    surface->SetDefaultFormat(GraphicPixelFormat::PIXEL_FMT_YCBCR_420_SP);

    // 获取Producer
    auto producer = surface->GetProducer();

    // 使用Surface...
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

    ASSERT_EQ(producer->RequestBuffer(buffer, fence, config), GSError::GSERROR_OK);
    ASSERT_NE(buffer, nullptr);
    ASSERT_EQ(buffer->GetSize(), 1920 * 1080 * 3 / 2);

    // 清理
    MockSurfaceFactory::Reset();
}
```

### 与现有Mock组件集成

Surface Mock 可以与其他 Mock 组件配合使用：

```cpp
#include "surface_mock.h"
#include "camera_mock.h"
#include "avcodec_mock.h"

// 创建完整的测试环境
auto surface = MockSurfaceFactory::CreateIConsumerSurface("camera");
auto camera = MockCameraFactory::CreateCamera("camera0");
auto codec = MockAVCodecFactory::CreateEncoder();

// 连接组件
camera->SetSurface(surface->GetProducer());
codec->SetInputSurface(surface);
```

## 性能测试

### 基准测试代码

```cpp
#include "surface_mock.h"
#include <chrono>

void BenchmarkSurface() {
    const int iterations = 1000;
    auto surface = MockSurfaceFactory::CreateIConsumerSurface("bench");
    auto producer = surface->GetProducer();

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; i++) {
        sptr<SurfaceBuffer> buffer;
        sptr<SyncFence> fence;
        BufferRequestConfig config = {/*...*/};

        producer->RequestBuffer(buffer, fence, config);
        producer->FlushBuffer(buffer, fence, {/*...*/});

        sptr<SurfaceBuffer> outBuffer;
        surface->AcquireBuffer(outBuffer, fence, {/*...*/});
        surface->ReleaseBuffer(outBuffer, fence);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Completed " << iterations << " iterations in "
              << duration.count() << "ms" << std::endl;
    std::cout << "Average: " << (duration.count() * 1000000.0 / iterations)
              << " microseconds per iteration" << std::endl;
}
```

## 清理

```bash
# 删除构建产物
cd mock/build
cmake --build . --target clean

# 或完全删除build目录
cd mock
rm -rf build
```

## 下一步

- 阅读 `README_SURFACE.md` 了解详细的使用说明
- 查看 `SURFACE_IMPLEMENTATION.md` 了解实现细节
- 运行 `surface_mock_test` 查看完整示例
- 集成到你的测试用例中

## 支持

如有问题，请检查：
1. 编译器版本 (需要 C++17 支持)
2. CMake 版本 (需要 3.16 或更高)
3. 系统库是否完整 (pthread 等)
