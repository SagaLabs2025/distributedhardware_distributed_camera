# Camera Framework Mock - 快速开始

## 最简示例

```cpp
#include "camera_mock.h"

int main() {
    using namespace OHOS::CameraStandard;

    // 1. 添加模拟相机
    auto manager = CameraManager::GetInstance();
    manager->AddMockCamera("camera_0");

    // 2. 获取并使用相机
    auto cameras = manager->GetSupportedCameras();
    std::cout << "找到 " << cameras.size() << " 个相机" << std::endl;

    return 0;
}
```

## 三阶段提交示例

```cpp
#include "camera_mock.h"

int main() {
    using namespace OHOS::CameraStandard;

    auto manager = CameraManager::GetInstance();
    manager->AddMockCamera("camera_0");

    // 获取相机设备
    auto cameras = manager->GetSupportedCameras();
    auto camera = cameras[0];

    // 创建输入
    std::shared_ptr<CameraInput> input;
    manager->CreateCameraInput(camera, &input);
    input->Open();

    // 创建会话
    auto session = manager->CreateCaptureSession();

    // 创建输出
    Profile profile(CAMERA_FORMAT_YUV_420, Size(1920, 1080));
    std::shared_ptr<PreviewOutput> output;
    manager->CreatePreviewOutput(profile, nullptr, &output);

    // === 三阶段提交 ===
    // 1. BeginConfig
    session->BeginConfig();

    // 2. AddInput/AddOutput
    session->AddInput(input);
    session->AddOutput(output);

    // 3. CommitConfig
    session->CommitConfig();

    // 启动
    session->Start();

    // ... 使用相机 ...

    // 清理
    session->Stop();
    session->Release();

    return 0;
}
```

## 使用TestHelper

```cpp
#include "camera_mock.h"

int main() {
    using namespace OHOS::CameraStandard::TestHelper;

    // 一行代码完成设置
    SetupCompleteCameraPipeline("camera_0", 1920, 1080, CAMERA_FORMAT_YUV_420);

    // 模拟视频输出
    SimulateVideoFrameOutput("camera_0", 1920, 1080, CAMERA_FORMAT_YUV_420, 100, 30);

    // 查看状态
    PrintMockState();

    return 0;
}
```

## 编译运行

### CMake

```bash
cd mock/build
cmake ..
cmake --build .
./camera_mock_test
```

### GN

```bash
./build.sh --product-name rk3568 --build-target distributed_camera_mock
```

## 常见错误处理

```cpp
// 检查返回值
int32_t ret = session->BeginConfig();
if (ret != CAMERA_OK) {
    // 处理错误
    switch (ret) {
        case DEVICE_BUSY:
            // 设备忙碌
            break;
        case CAMERA_INVALID_ARG:
            // 参数无效
            break;
        default:
            // 其他错误
            break;
    }
}

// 检查状态
if (session->GetConfigState() == CaptureSession::CONFIG_STATE_CONFIGURING) {
    // 可以进行配置
}
```
