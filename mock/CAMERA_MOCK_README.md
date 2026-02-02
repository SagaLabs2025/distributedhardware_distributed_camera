# Camera Framework Mock 实现文档

## 概述

本Mock实现提供了OpenHarmony Camera Framework的完整模拟，用于分布式相机组件的单元测试和集成测试。该实现支持核心的相机操作，包括三阶段提交、视频帧输出模拟等功能。

## 文件结构

```
mock/
├── include/
│   └── camera_mock.h          # Camera Framework Mock 头文件
├── src/
│   └── camera_mock.cpp        # Camera Framework Mock 实现
└── test/
    └── camera_mock_test.cpp   # 单元测试
```

## 核心组件

### 1. CameraManager（相机管理器）

单例类，提供相机设备的查询和创建功能。

```cpp
// 获取单例实例
CameraManager* manager = CameraManager::GetInstance();

// 获取支持的相机列表
std::vector<std::shared_ptr<CameraDevice>> cameras = manager->GetSupportedCameras();

// 创建相机输入
std::shared_ptr<CameraInput> cameraInput;
int32_t ret = manager->CreateCameraInput(cameraDevice, &cameraInput);

// 创建捕获会话
std::shared_ptr<CaptureSession> session = manager->CreateCaptureSession();

// 创建预览输出
Profile profile(CAMERA_FORMAT_YUV_420, Size(1920, 1080));
std::shared_ptr<PreviewOutput> previewOutput;
ret = manager->CreatePreviewOutput(profile, surface, &previewOutput);

// 创建拍照输出
std::shared_ptr<PhotoOutput> photoOutput;
ret = manager->CreatePhotoOutput(profile, producer, &photoOutput);
```

### 2. CameraInput（相机输入）

代表一个已打开的相机设备。

```cpp
// 打开相机
int32_t ret = cameraInput->Open();

// 关闭相机
ret = cameraInput->Close();

// 释放资源
ret = cameraInput->Release();

// 设置相机参数
ret = cameraInput->SetCameraSettings(metadata);

// 设置错误回调
cameraInput->SetErrorCallback(callback);
```

### 3. CaptureSession（捕获会话）

支持三阶段提交的捕获会话。

**状态机：**
```
IDLE → CONFIGURING → CONFIGURED → STARTED
       ↑                           ↓
       └────────── Stop() ←────────┘
```

**三阶段提交：**

```cpp
// 阶段1：开始配置
int32_t ret = session->BeginConfig();

// 阶段2：添加输入和输出
ret = session->AddInput(cameraInput);
ret = session->AddOutput(previewOutput);

// 阶段3：提交配置
ret = session->CommitConfig();

// 启动会话
ret = session->Start();

// 停止会话
ret = session->Stop();

// 释放会话
ret = session->Release();
```

### 4. PreviewOutput（预览输出）

视频流输出。

```cpp
// 开始输出
int32_t ret = previewOutput->Start();

// 停止输出
ret = previewOutput->Stop();

// 设置帧率
ret = previewOutput->SetFrameRate(minFps, maxFps);

// 释放资源
ret = previewOutput->Release();
```

### 5. PhotoOutput（拍照输出）

照片捕获输出。

```cpp
// 拍照
int32_t ret = photoOutput->Capture();

// 取消拍照
ret = photoOutput->Cancel();

// 释放资源
ret = photoOutput->Release();
```

## 测试辅助函数（TestHelper）

### 初始化Mock相机

```cpp
using namespace OHOS::CameraStandard::TestHelper;

// 初始化模拟相机
InitializeMockCameras({"camera_0", "camera_1"});
```

### 重置Mock状态

```cpp
// 重置所有状态
ResetMockState();
```

### 快速设置完整相机管道

```cpp
// 一次性设置完整的相机管道
bool success = SetupCompleteCameraPipeline(
    "camera_0",       // 相机ID
    1920,             // 宽度
    1080,             // 高度
    CAMERA_FORMAT_YUV_420  // 格式
);
```

### 验证三阶段提交

```cpp
// 验证三阶段提交状态机
bool success = ValidateThreePhaseCommit("camera_0");
```

### 模拟视频帧输出

```cpp
// 模拟视频帧输出到Surface
SimulateVideoFrameOutput(
    "camera_0",       // 相机ID
    1920,             // 帧宽度
    1080,             // 帧高度
    CAMERA_FORMAT_YUV_420,  // 帧格式
    100,              // 帧数
    30                // 帧率
);
```

### 打印Mock状态

```cpp
// 打印所有相机状态
PrintMockState();

// 打印特定相机状态
PrintMockState("camera_0");
```

## 错误码定义

```cpp
enum CameraErrorCode {
    CAMERA_OK = 0,              // 成功
    CAMERA_INVALID_ARG = 1,     // 无效参数
    CAMERA_NOT_PERMITTED = 2,   // 权限不足
    SERVICE_FATL_ERROR = 3,     // 服务致命错误
    DEVICE_DISCONNECTED = 4,    // 设备断开连接
    DEVICE_IN_USE = 5,          // 设备使用中
    CONFLICT_CAMERA = 6,        // 相机冲突
    DEVICE_BUSY = 7,            // 设备忙碌
    CAMERA_CLOSED = 8           // 相机已关闭
};
```

## 使用示例

### 示例1：基本的相机操作流程

```cpp
#include "camera_mock.h"

using namespace OHOS::CameraStandard;

int main() {
    // 1. 获取相机管理器
    auto manager = CameraManager::GetInstance();

    // 2. 添加模拟相机
    manager->AddMockCamera("camera_main");

    // 3. 获取相机设备
    auto cameras = manager->GetSupportedCameras();
    auto camera = cameras[0];

    // 4. 创建相机输入
    std::shared_ptr<CameraInput> cameraInput;
    manager->CreateCameraInput(camera, &cameraInput);

    // 5. 打开相机
    cameraInput->Open();

    // 6. 创建捕获会话
    auto session = manager->CreateCaptureSession();

    // 7. 创建预览输出
    Profile profile(CAMERA_FORMAT_YUV_420, Size(1920, 1080));
    std::shared_ptr<PreviewOutput> previewOutput;
    manager->CreatePreviewOutput(profile, nullptr, &previewOutput);

    // 8. 三阶段提交配置
    session->BeginConfig();
    session->AddInput(cameraInput);
    session->AddOutput(previewOutput);
    session->CommitConfig();

    // 9. 启动会话
    session->Start();

    // 10. 使用相机...

    // 11. 停止并释放
    session->Stop();
    session->Release();
    cameraInput->Release();

    return 0;
}
```

### 示例2：使用TestHelper快速测试

```cpp
#include "camera_mock.h"

using namespace OHOS::CameraStandard;
using namespace OHOS::CameraStandard::TestHelper;

int main() {
    // 初始化
    InitializeMockCameras({"camera_main", "camera_front"});

    // 快速设置
    SetupCompleteCameraPipeline("camera_main", 1920, 1080, CAMERA_FORMAT_YUV_420);

    // 模拟视频输出
    SimulateVideoFrameOutput("camera_main", 1920, 1080, CAMERA_FORMAT_YUV_420, 100, 30);

    // 打印状态
    PrintMockState();

    // 重置
    ResetMockState();

    return 0;
}
```

## 三阶段提交详解

三阶段提交是Camera Framework的核心机制，确保配置的原子性和一致性。

### 阶段1：BeginConfig

将会话状态从`IDLE`转换为`CONFIGURING`，清空之前的配置。

```cpp
session->BeginConfig();  // 返回 CAMERA_OK 或错误码
```

### 阶段2：AddInput/AddOutput

在`CONFIGURING`状态下添加输入和输出。

```cpp
session->AddInput(cameraInput);    // 返回 CAMERA_OK 或错误码
session->AddOutput(previewOutput); // 返回 CAMERA_OK 或错误码
```

**状态验证：**
- 必须在`CONFIGURING`状态下调用
- 只能添加一个Input
- 可以添加多个Output
- 空指针会返回`CAMERA_INVALID_ARG`

### 阶段3：CommitConfig

提交配置，状态从`CONFIGURING`转换为`CONFIGURED`。

```cpp
session->CommitConfig();  // 返回 CAMERA_OK 或错误码
```

**验证：**
- 必须有至少一个Input
- 必须有至少一个Output
- 必须在`CONFIGURING`状态下调用

### 启动会话

```cpp
session->Start();  // 必须在CONFIGURED状态下调用
```

### 停止会话

```cpp
session->Stop();   // 必须在STARTED状态下调用
```

## 编译和测试

### CMake编译

```bash
cd mock/build
cmake ..
cmake --build .
```

### 运行测试

```bash
./camera_mock_test
```

### GN编译（OpenHarmony）

```bash
./build.sh --product-name {product_name} --build-target distributed_camera_mock
```

## 注意事项

1. **线程安全**：CameraManager内部使用mutex保护，但多线程使用时仍需注意同步。

2. **资源管理**：使用完毕后务必调用Release释放资源。

3. **状态检查**：调用方法前检查状态，避免状态机错误。

4. **错误处理**：所有方法都返回错误码，务必检查返回值。

5. **单例模式**：CameraManager是单例，全局共享一个实例。

## 扩展和定制

### 添加自定义错误码

在camera_mock.h中扩展CameraErrorCode枚举。

### 添加自定义回调

继承对应的Callback类并实现虚函数。

### 模拟特定场景

使用TestHelper函数或直接操作CameraManager的Mock配置方法。

## 参考资料

- OpenHarmony Camera Framework文档
- 分布式相机架构文档
- 单元测试最佳实践
