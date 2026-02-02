# OpenHarmony Distributed Camera Development Skill

分布式相机模块开发技能，用于在vibe coding时提供完整的架构、依赖和调用流程上下文。

## 项目概述

OpenHarmony分布式相机是分布式硬件子系统的核心组件，实现跨设备相机协同使用能力。

### 系统规格
- **ROM占用**: ~5120 KB
- **RAM占用**: ~66560 KB
- **语言**: C++
- **系统**: OpenHarmony标准系统

### 架构模式
- **主控端（Source）**: 控制端，SA_ID=4803
- **被控端（Sink）**: 被控制端，SA_ID=4804

### 核心组件
```
distributed_camera/
├── interfaces/inner_kits/     # 对外接口层
│   ├── camera_source/         # Source SDK (distributed_camera_source_sdk)
│   └── camera_sink/           # Sink SDK (distributed_camera_sink_sdk)
├── services/                   # 服务实现层
│   ├── cameraservice/
│   │   ├── sourceservice/     # Source服务 (SA 4803)
│   │   ├── sinkservice/       # Sink服务 (SA 4804)
│   │   ├── cameraoperator/
│   │   │   ├── client/         # Camera Client (本地相机访问)
│   │   │   └── handler/        # Camera Handler (事件处理)
│   ├── channel/               # 软总线通信模块
│   └── data_process/          # 编解码、格式转换、缩放、旋转
├── common/                     # 公共工具
│   ├── include/constants/      # 系统常量和错误码
│   └── include/utils/          # 日志、跟踪、缓冲管理
├── mock/                      # Mock模拟层（测试用）
└── sa_profile/                # SA配置
    ├── 4803.json              # Source SA
    └── 4804.json              # Sink SA
```

## 三通道通信架构

Source和Sink通过三个独立通道通信，每条通道针对不同数据类型优化：

| 通道类型 | 会话名后缀 | 用途 | 最大分辨率 | 编码格式 |
|---------|-----------|------|-----------|---------|
| **Control通道** | `_control_sender` / `_control_receiver` | 相机命令、配置、元数据协商 | N/A | Protocol Buffers |
| **Snapshot通道** | `_dataSnapshot_sender` / `_dataSnapshot_receiver` | 高分辨率静态图片 | 4096×3072 | JPEG |
| **Continuous通道** | `_dataContinue_sender` / `_dataContinue_receiver` | 实时视频流 | 1920×1080 | H.265 |

### QoS参数
```cpp
// 带宽限制
constexpr int32_t DCAMERA_MAX_RECV_DATA_LEN = 41943040;  // 40 MB
constexpr int32_t DCAMERA_QOS_MIN_BW = 40960;             // 40 MB/s

// 延迟要求
constexpr int32_t DCAMERA_QOS_MAX_LATENCY = 8000;        // 8秒
constexpr int32_t DCAMERA_QOS_MIN_LATENCY = 2000;        // 2秒
```

## 完整外部依赖

### 1. 分布式硬件框架

#### 1.1 系统能力定义

```cpp
// 组件定义
// Component: distributed_hardware_fwk
// Subsystem: distributedhardware
// SystemCapability: SystemCapability.DistributedHardware.DistributedCamera
```

#### 1.2 核心头文件

```cpp
#include "distributed_hardware_log.h"    // 日志: DHLOGI/DHLOGE/DHLOGW/DHLOGD
#include "dm_device_manager.h"            // 设备管理器
#include "distributed_hardware_manager.h" // 分布式硬件管理器
```

#### 1.3 关键数据结构

```cpp
/**
 * 分布式硬件设备信息
 */
struct DmDeviceInfo {
    std::string deviceId;      // 设备ID
    std::string deviceName;    // 设备名称
    std::string networkId;     // 网络ID
};

/**
 * 分布式硬件基础信息
 */
struct DHBase {
    std::string deviceId_;     // 设备ID（即networkId）
    std::string dhId_;         // 分布式硬件ID
};
```

#### 1.4 与分布式相机的调用关系和调用流程

**调用关系图**：

```
分布式硬件框架
    └── 提供能力管理
        ├── 设备发现和组网
        ├── 权限管理
        └── 生命周期管理
            ↓
分布式相机 (Source/Sink SA)
    ├── 实现DHManagerCallBack接口
    │   ├── OnDeviceOnline() - 设备上线时触发
    │   ├── OnDeviceOffline() - 设备下线时触发
    │   └── OnDistributedHardwareEnable() - 使能时触发
    └── 调用DHManager接口
        ├── RegisterDistributedHardware() - 注册虚拟相机
        └── UnregisterDistributedHardware() - 注销虚拟相机
```

**完整调用流程**：

```
1. 系统启动
   ↓
2. SAMGR拉起SA服务
   ├─ DCameraSourceService (SA 4803)
   └─ DCameraSinkService (SA 4804)
   ↓
3. 分布式硬件框架初始化
   ↓
4. 设备组网上线
   ↓
5. 分布式硬件框架通知设备上线
   DHManagerCallBack::OnDeviceOnline()
   ↓
6. Source端获取相机能力并上报
   ↓
7. 应用查询可用相机
   ↓
8. 应用打开分布式相机
   ↓
9. 分布式硬件框架调用RegisterDistributedHardware()
   ↓
10. 创建虚拟相机HDF设备
   ↓
11. Camera Framework发现新相机
   ↓
12. 应用可正常使用相机
```

**测试用例如何交互**：

```cpp
// Mock测试中模拟分布式硬件框架调用

class MockDHManager : public DHManagerCallBack {
public:
    // 模拟设备上线事件
    void SimulateDeviceOnline(const std::string& networkId) {
        DmDeviceInfo deviceInfo;
        deviceInfo.deviceId = "dev001";
        deviceInfo.networkId = networkId;

        // 触发设备上线回调
        OnDeviceOnline(deviceInfo);
    }

    // 模拟设备下线事件
    void SimulateDeviceOffline(const std::string& networkId) {
        DmDeviceInfo deviceInfo;
        deviceInfo.networkId = networkId;

        // 触发设备下线回调
        OnDeviceOffline(deviceInfo);
    }
};

// 测试流程
auto mockDH = std::make_shared<MockDHManager>();

// 1. 模拟设备上线
mockDH->SimulateDeviceOnline("network_001");

// 2. 等待Source服务处理

// 3. 验证虚拟相机是否被创建
ASSERT_TRUE(IsVirtualCameraCreated("dh_001"));

// 4. 模拟设备下线
mockDH->SimulateDeviceOffline("network_001");

// 5. 验证虚拟相机是否被移除
ASSERT_FALSE(IsVirtualCameraCreated("dh_001"));
```

---

### 2. 软总线

软总线 (`@ohos.communication.dsoftbus`) 提供设备间的高速、安全通信能力，使用Socket风格的API。

#### 2.1 系统能力定义

```cpp
// 组件定义
// Component: dsoftbus
// Subsystem: communication
// SystemCapability: SystemCapability.Communication.DSoftBus
```

#### 2.2 核心头文件

```cpp
#include "socket.h"          // Socket API接口
#include "trans_type.h"      // 数据类型定义
```

#### 2.3 数据类型

```cpp
/**
 * 数据传输类型
 */
typedef enum {
    DATA_TYPE_MESSAGE = 1,        /**< 消息 */
    DATA_TYPE_BYTES,              /**< 字节流 */
    DATA_TYPE_FILE,               /**< 文件 */
    DATA_TYPE_RAW_STREAM,         /**< 原始数据流 */
    DATA_TYPE_VIDEO_STREAM,       /**< 视频数据流 */
    DATA_TYPE_AUDIO_STREAM,       /**< 音频数据流 */
    DATA_TYPE_SLICE_STREAM,       /**< 视频切片流 */
    DATA_TYPE_BUTT,
} TransDataType;
```

#### 2.4 Socket信息结构

```cpp
/**
 * Socket信息（本端）
 */
typedef struct {
    char *name;             /**< 本端socket名称 */
    char *peerName;         /**< 对端socket名称 */
    char *peerNetworkId;    /**< 对端网络ID */
    char *pkgName;          /**< 包名 */
    TransDataType dataType; /**< 数据类型 */
} SocketInfo;

/**
 * Peer Socket信息（对端）
 */
typedef struct {
    char *name;             /**< 对端socket名称 */
    char *networkId;        /**< 对端网络ID */
    char *pkgName;          /**< 对端包名 */
    TransDataType dataType; /**< 数据类型 */
} PeerSocketInfo;
```

#### 2.5 Socket回调接口

```cpp
/**
 * Socket回调结构
 */
typedef struct {
    void (*OnBind)(int32_t socket, PeerSocketInfo info);
    void (*OnShutdown)(int32_t socket, ShutdownReason reason);
    void (*OnBytes)(int32_t socket, const void *data, uint32_t dataLen);
    void (*OnMessage)(int32_t socket, const void *data, uint32_t dataLen);
    void (*OnStream)(int32_t socket, const StreamData *data,
                     const StreamData *ext, const StreamFrameInfo *param);
    void (*OnFile)(int32_t socket, FileEvent *event);
    void (*OnQos)(int32_t socket, QoSEvent eventId,
                  const QosTV *qos, uint32_t qosCount);
} ISocketListener;
```

#### 2.6 核心API接口

```cpp
// 创建Socket
int32_t Socket(SocketInfo info);

// 服务端监听
int32_t Listen(int32_t socket, const QosTV qos[], uint32_t qosCount,
               const ISocketListener *listener);

// 客户端连接
int32_t Bind(int32_t socket, const QosTV qos[], uint32_t qosCount,
             const ISocketListener *listener);

// 发送数据
int32_t SendBytes(int32_t socket, const void *data, uint32_t len);
int32_t SendMessage(int32_t socket, const void *data, uint32_t len);
int32_t SendStream(int32_t socket, const StreamData *data,
                   const StreamData *ext, const StreamFrameInfo *param);

// 关闭Socket
void Shutdown(int32_t socket);

// 评估QoS
int32_t EvaluateQos(const char *peerNetworkId, TransDataType dataType,
                     const QosTV *qos, uint32_t qosCount);
```

#### 2.7 与分布式相机的调用关系和调用流程

**调用关系图**：

```
分布式相机 Channel模块
    ├── Control通道
    │   └── Socket(DATA_TYPE_MESSAGE)
    │       ├── OnMessage() - 接收控制命令
    │       └── SendMessage() - 发送控制响应
    ├── Snapshot通道
    │   └── Socket(DATA_TYPE_BYTES)
    │       ├── OnBytes() - 接收JPEG图片
    │       └── SendBytes() - 发送JPEG图片
    └── Continuous通道
        └── Socket(DATA_TYPE_VIDEO_STREAM)
            ├── OnStream() - 接收H.265视频
            └── SendStream() - 发送H.265视频
```

**完整调用流程**：

```
Source端                                  Sink端
    │                                         │
    │ 1. 创建Control通道Socket                  │
    │ Socket(controlInfo)                      │
    │ Bind(controlSocket, ...)                 │
    │                                         │
    │ 2. 发送OpenSession消息 ────────────────→ │
    │                         OnMessage()     │
    │                          ↓              │
    │                         创建通道        │
    │                          ↓              │
    │ 3. ←─────────────── 返回OpenSession结果  │
    │    OnMessage()                            │
    │                                         │
    │ 4. 发送ConfigureStreams消息 ───────────→ │
    │                                         │
    │ 5. ←─────────────── 返回配置结果          │
    │                                         │
    │ 6. 发送StartCapture消息 ──────────────→ │
    │                    开始捕获              │
    │                                         │
    │ 7. Continuous通道接收视频帧 ──────────→ │
    │    SendStream()                         │
    │                   OnStream()            │
    │                    ↓                    │
    │                  显示视频               │
```

**测试用例如何交互**：

```cpp
// Mock软总线Socket

class MockSocket {
public:
    // 模拟Socket创建
    int32_t CreateSocket(const SocketInfo& info) {
        sockets_[socketId_] = info;
        return socketId_++;
    }

    // 模拟Bind连接
    int32_t Bind(int32_t socket, const QosTV qos[], uint32_t qosCount,
                 const ISocketListener *listener) {
        listeners_[socket] = listener;
        return 0;
    }

    // 模拟接收消息（触发回调）
    void SimulateReceiveMessage(int32_t socket, const void* data, uint32_t len) {
        if (listeners_[socket] && listeners_[socket]->OnMessage) {
            listeners_[socket]->OnMessage(socket, data, len);
        }
    }

    // 验证发送的数据
    bool VerifySentData(int32_t socket, const void* expectedData, uint32_t expectedLen) {
        // 实现数据验证逻辑
        return true;
    }

private:
    int32_t socketId_ = 1;
    std::map<int32_t, SocketInfo> sockets_;
    std::map<int32_t, const ISocketListener*> listeners_;
};

// 测试流程
auto mockSocket = std::make_shared<MockSocket>();

// 1. 创建Control通道
SocketInfo controlInfo = {
    .name = "ohos.distributedcamera.source.control",
    .peerName = "ohos.distributedcamera.sink.control",
    .dataType = DATA_TYPE_MESSAGE
};
int32_t controlSocket = mockSocket->CreateSocket(controlInfo);
mockSocket->Bind(controlSocket, qos, 2, &listener);

// 2. 模拟接收OpenSession消息
std::string openSessionMsg = "OpenSession";
mockSocket->SimulateReceiveMessage(controlSocket, openSessionMsg.c_str(), openSessionMsg.length());

// 3. 验证是否正确响应
ASSERT_TRUE(mockSocket->VerifySentData(controlSocket, expectedResponse, expectedLen));
```

---

### 3. 相机框架 (Camera Framework)

Camera Framework (`@ohos/camera_framework`) 实现五层架构：

```
Application (应用层)
    ↓
Bindings (NAPI/NDK) - JavaScript/C API
    ↓
Framework (框架层) - Native C++ API
    ↓
Service (camera_service) - SA #3009
    ↓
HDI → Hardware (硬件驱动层)
```

#### 3.1 系统能力定义

```cpp
// SystemAbility定义
constexpr int32_t CAMERA_SERVICE_ID = 3009;

// SystemCapability声明
// Component: camera_framework
// Subsystem: multimedia
// SystemCapability: SystemCapability.Multimedia.Camera.Core
```

#### 3.2 核心头文件

```cpp
// 核心管理器头文件
#include "interfaces/inner_api/native/camera/include/input/camera_manager.h"

// 捕获会话头文件
#include "interfaces/inner_api/native/camera/include/session/capture_session.h"

// 相机输入头文件
#include "interfaces/inner_api/native/camera/include/input/camera_input.h"

// 输出头文件
#include "interfaces/inner_api/native/camera/include/output/photo_output.h"
#include "interfaces/inner_api/native/camera/include/output/preview_output.h"
#include "interfaces/inner_api/native/camera/include/output/video_output.h"
```

#### 3.3 核心API类

**CameraManager** - 相机管理器入口点

```cpp
namespace OHOS::Camera {

class CameraManager {
public:
    // 获取单例实例
    static CameraManager* GetInstance();

    // 获取支持的相机列表
    std::vector<std::string> GetSupportedCameras();

    // 获取相机能力
    std::shared_ptr<CameraAbility> GetCameraAbility(const std::string& cameraId);

    // 创建相机输入
    std::shared_ptr<CameraInput> CreateCameraInput(const std::string& cameraId);

    // 设置回调
    void SetCallback(std::shared_ptr<ManagerCallback> callback);
};
}
```

**CameraInput** - 相机输入控制

```cpp
class CameraInput {
public:
    // 打开相机
    int32_t Open();

    // 关闭相机
    int32_t Close();

    // 释放资源
    int32_t Release();

    // 获取相机设备
    std::string GetCameraId();

    // 设置回调
    void SetErrorCallback(std::shared_ptr<CameraInputErrorCallback> callback);
};
```

**CaptureSession** - 捕获会话（使用三阶段提交模式）

```cpp
class CaptureSession {
public:
    // 配置阶段1: 开始配置
    int32_t BeginConfig();

    // 添加输入（相机输入）
    int32_t AddInput(std::shared_ptr<CameraInput> input);

    // 添加输出（预览/拍照/视频）
    int32_t AddOutput(std::shared_ptr<PreviewOutput> output);
    int32_t AddOutput(std::shared_ptr<PhotoOutput> output);
    int32_t AddOutput(std::shared_ptr<VideoOutput> output);

    // 移除输入/输出
    int32_t RemoveInput(std::shared_ptr<CameraInput> input);
    int32_t RemoveOutput(std::shared_ptr<CaptureOutput> output);

    // 配置阶段2: 提交配置
    int32_t CommitConfig();

    // 启动/停止捕获
    int32_t Start();
    int32_t Stop();

    // 释放会话
    int32_t Release();
};
```

**PreviewOutput** - 预览输出

```cpp
class PreviewOutput {
public:
    // 创建预览输出（绑定Surface）
    static std::shared_ptr<PreviewOutput> Create(sptr<Surface> surface);

    // 启动/停止预览
    int32_t Start();
    int32_t Stop();

    // 释放资源
    int32_t Release();

    // 设置回调
    void SetCallback(std::shared_ptr<PreviewOutputCallback> callback);
};
```

**PhotoOutput** - 拍照输出

```cpp
class PhotoOutput {
public:
    // 创建拍照输出
    static std::shared_ptr<PhotoOutput> Create(
        const sptr<Surface>& surface,
        const std::shared_ptr<PhotoCaptureSetting>& setting
    );

    // 拍照
    int32_t Capture();

    // 释放资源
    int32_t Release();

    // 设置回调
    void SetCallback(std::shared_ptr<PhotoOutputCallback> callback);
};
```

#### 3.4 配置流程（三阶段提交）

```cpp
// 阶段1: 开始配置
session->BeginConfig();

// 阶段2: 添加输入/输出
session->AddInput(cameraInput);
session->AddOutput(previewOutput);
session->AddOutput(photoOutput);

// 阶段3: 提交配置（原子性应用）
session->CommitConfig();

// 开始捕获
session->Start();
```

#### 3.5 与分布式相机的调用关系和调用流程

**调用关系图**：

```
Sink端分布式相机
    └── CameraClient组件
        └── 调用Camera Framework
            ├── CameraManager::GetInstance()
            ├── GetSupportedCameras()
            ├── CreateCameraInput()
            └── CaptureSession
                ├── BeginConfig()
                ├── AddInput()
                ├── AddOutput() - 绑定AVCodec的Surface
                ├── CommitConfig()
                └── Start()
                    ↓
            Camera Service (camera_service, SA #3009)
                ↓ IPC (Binder)
            HDI接口
                ↓
            本地相机硬件
```

**完整调用流程（Sink端）**：

```
1. 分布式硬件框架通知使能相机
   ↓
2. CameraClient初始化
   ↓
3. 获取CameraManager单例
   auto cameraManager = OHOS::Camera::CameraManager::GetInstance();
   ↓
4. 获取支持的相机列表
   auto cameraIds = cameraManager->GetSupportedCameras();
   ↓
5. 创建相机输入
   auto cameraInput = cameraManager->CreateCameraInput(cameraIds[0]);
   ↓
6. 创建捕获会话
   auto captureSession = std::make_shared<OHOS::Camera::CaptureSession>();
   ↓
7. 同时创建AVCodec编码器（下一节详述）
   auto encoder = VideoEncoderFactory::CreateByMime("video/hevc");
   auto encoderSurface = encoder->CreateInputSurface();
   ↓
8. 创建预览输出（绑定编码Surface）
   auto previewSurface = Surface::CreateSurface();
   previewSurface->SetNativeWindow(encoderSurface);
   auto previewOutput = OHOS::Camera::PreviewOutput::Create(previewSurface);
   ↓
9. 三阶段提交配置
   captureSession->BeginConfig();
   captureSession->AddInput(cameraInput);
   captureSession->AddOutput(previewOutput);
   captureSession->CommitConfig();
   ↓
10. 启动会话
    captureSession->Start();
    ↓
11. 相机开始输出YUV数据到PreviewOutput Surface
    ↓
12. Surface数据自动进入AVCodec编码器
    ↓
13. 编码后的H.265数据通过回调返回
```

**测试用例如何交互**：

```cpp
// Mock Camera Framework

class MockCameraFramework {
public:
    // 模拟CameraManager
    class MockCameraManager {
    public:
        static MockCameraManager* GetInstance() {
            static MockCameraManager instance;
            return &instance;
        }

        std::vector<std::string> GetSupportedCameras() {
            return {"camera_0", "camera_1"};
        }

        std::shared_ptr<MockCameraInput> CreateCameraInput(const std::string& cameraId) {
            return std::make_shared<MockCameraInput>(cameraId);
        }
    };

    // 模拟CameraInput
    class MockCameraInput {
    public:
        MockCameraInput(const std::string& cameraId) : cameraId_(cameraId) {}

        int32_t Open() { return 0; }
        int32_t Close() { return 0; }
        std::string GetCameraId() { return cameraId_; }

    private:
        std::string cameraId_;
    };

    // 模拟CaptureSession
    class MockCaptureSession {
    public:
        int32_t BeginConfig() {
            configs_.clear();
            return 0;
        }

        int32_t AddInput(std::shared_ptr<MockCameraInput> input) {
            cameraInput_ = input;
            return 0;
        }

        int32_t AddOutput(std::shared_ptr<MockPreviewOutput> output) {
            outputs_.push_back(output);
            return 0;
        }

        int32_t CommitConfig() { return 0; }

        int32_t Start() {
            started_ = true;
            // 模拟开始输出数据
            if (previewOutput_) {
                previewOutput_->SimulateFrameOutput();
            }
            return 0;
        }

        int32_t Stop() {
            started_ = false;
            return 0;
        }

    private:
        std::shared_ptr<MockCameraInput> cameraInput_;
        std::vector<std::shared_ptr<MockPreviewOutput>> outputs_;
        std::shared_ptr<MockPreviewOutput> previewOutput_;
        bool started_ = false;
    };
};

// 测试流程
auto cameraManager = MockCameraFramework::MockCameraManager::GetInstance();
auto cameraInput = cameraManager->CreateCameraInput("camera_0");
auto captureSession = std::make_shared<MockCameraFramework::MockCaptureSession>();

// 三阶段提交
captureSession->BeginConfig();
captureSession->AddInput(cameraInput);
captureSession->AddOutput(previewOutput);
captureSession->CommitConfig();

// 启动会话
captureSession->Start();

// 验证是否正确启动
ASSERT_TRUE(captureSession->IsStarted());
```

---

### 4. AVCodec音视频编解码

AVCodec (`@ohos.multimedia.media_avcodec`) 是OpenHarmony的音视频编解码组件，提供视频编码和解码能力。

**注意**：AVCodec模块提供了两套接口：
1. **对外提供的C接口**（公共SDK）：位于`multimedia/av_codec/interfaces/kits/c/native/`
2. **inner_api接口**（内部使用）：位于`multimedia/av_codec/interfaces/inner_api/native/`

**本模块使用的是inner_api接口**。

#### 4.1 系统能力定义

```cpp
// 组件定义
// Component: avcodec
// Subsystem: multimedia
// SystemCapability: SystemCapability.Multimedia.AVCodec.Codec
```

#### 4.2 核心头文件（inner_api）

```cpp
#include "avcodec_video_encoder.h"     // 视频编码器
#include "avcodec_video_decoder.h"     // 视频解码器
#include "avcodec_common.h"            // 通用定义和回调
#include "avcodec_info.h"              // 编解码器信息
#include "surface.h"                   // Surface支持
```

#### 4.3 支持的编码格式

- H.264/AVC (`"video/avc"`)
- H.265/HEVC (`"video/hevc"`)
- JPEG (`"image/jpeg"`)

#### 4.4 核心API类（inner_api C++接口）

**VideoEncoderFactory** - 编码器工厂

```cpp
namespace OHOS::MediaAVCodec {

class VideoEncoderFactory {
public:
    // 通过MIME类型创建编码器
    static std::shared_ptr<AVCodecVideoEncoder> CreateByMime(const std::string &mime);

    // 通过名称创建编码器
    static std::shared_ptr<AVCodecVideoEncoder> CreateByName(const std::string &name);
};

}
```

**AVCodecVideoEncoder** - 视频编码器接口

```cpp
namespace OHOS::MediaAVCodec {

class AVCodecVideoEncoder {
public:
    virtual ~AVCodecVideoEncoder() = default;

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

    // 重置编码器
    virtual int32_t Reset() = 0;

    // 释放编码器
    virtual int32_t Release() = 0;

    // 创建输入Surface（Surface模式）- 关键接口
    virtual sptr<Surface> CreateInputSurface() = 0;

    // 提交输入Buffer（Buffer模式）
    virtual int32_t QueueInputBuffer(uint32_t index, AVCodecBufferInfo info,
                                      AVCodecBufferFlag flag) = 0;

    // 获取输出格式
    virtual int32_t GetOutputFormat(Format &format) = 0;

    // 释放输出Buffer
    virtual int32_t ReleaseOutputBuffer(uint32_t index) = 0;

    // 设置参数
    virtual int32_t SetParameter(const Format &format) = 0;

    // 注册回调（AVCodecCallback风格）
    virtual int32_t SetCallback(const std::shared_ptr<AVCodecCallback> &callback) = 0;

    // 注册回调（MediaCodecCallback风格）
    virtual int32_t SetCallback(const std::shared_ptr<MediaCodecCallback> &callback) = 0;

    // 查询可用输入Buffer
    virtual int32_t QueryInputBuffer(uint32_t &index, int64_t timeoutUs) = 0;

    // 查询可用输出Buffer
    virtual int32_t QueryOutputBuffer(uint32_t &index, int64_t timeoutUs) = 0;

    // 获取输入Buffer
    virtual std::shared_ptr<AVBuffer> GetInputBuffer(uint32_t index) = 0;

    // 获取输出Buffer
    virtual std::shared_ptr<AVBuffer> GetOutputBuffer(uint32_t index) = 0;
};

}
```

#### 4.5 回调接口

**AVCodecCallback** - 传统回调风格

```cpp
namespace OHOS::MediaAVCodec {

class AVCodecCallback {
public:
    virtual ~AVCodecCallback() = default;

    // 错误回调
    virtual void OnError(AVCodecErrorType errorType, int32_t errorCode) = 0;

    // 输出格式变化回调
    virtual void OnOutputFormatChanged(const Format &format) = 0;

    // 输入Buffer可用回调
    virtual void OnInputBufferAvailable(uint32_t index,
                                        std::shared_ptr<AVSharedMemory> buffer) = 0;

    // 输出Buffer可用回调
    virtual void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info,
                                         AVCodecBufferFlag flag,
                                         std::shared_ptr<AVSharedMemory> buffer) = 0;
};

}
```

**MediaCodecCallback** - 新回调风格（使用AVBuffer）

```cpp
namespace OHOS::MediaAVCodec {

class MediaCodecCallback {
public:
    virtual ~MediaCodecCallback() = default;

    // 错误回调
    virtual void OnError(AVCodecErrorType errorType, int32_t errorCode) = 0;

    // 输出格式变化回调
    virtual void OnOutputFormatChanged(const Format &format) = 0;

    // 输入Buffer可用回调
    virtual void OnInputBufferAvailable(uint32_t index,
                                        std::shared_ptr<AVBuffer> buffer) = 0;

    // 输出Buffer可用回调
    virtual void OnOutputBufferAvailable(uint32_t index,
                                         std::shared_ptr<AVBuffer> buffer) = 0;
};

}
```

#### 4.6 数据结构

**AVCodecBufferInfo** - Buffer信息

```cpp
struct AVCodecBufferInfo {
    int64_t presentationTimeUs;  // 显示时间戳（微秒）
    int32_t size;                 // 数据大小（字节）
    int32_t offset;               // 数据偏移
};
```

**AVCodecBufferFlag** - Buffer标志

```cpp
enum AVCodecBufferFlag : uint32_t {
    AVCODEC_BUFFER_FLAG_NONE = 0,
    AVCODEC_BUFFER_FLAG_EOS = 1 << 0,              // 流结束标志
    AVCODEC_BUFFER_FLAG_SYNC_FRAME = 1 << 1,       // 同步帧（关键帧）
    AVCODEC_BUFFER_FLAG_PARTIAL_FRAME = 1 << 2,    // 部分帧
    AVCODEC_BUFFER_FLAG_CODEC_DATA = 1 << 3,       // 编码器特定数据
};
```

**Format** - 配置格式

```cpp
// Format用于配置编码器参数
Format format;

// 必须配置参数
format.PutIntValue("width", 1920);           // 视频宽度
format.PutIntValue("height", 1080);          // 视频高度
format.PutIntValue("pixel_format", NV12);    // 像素格式

// 可选配置参数
format.PutDoubleValue("frame_rate", 30.0);   // 帧率
format.PutIntValue("bitrate", 5000000);      // 比特率（bps）
```

#### 4.7 编码器生命周期

```
Released
    ↓ Create
Initialized
    ↓ Configure
Configured
    ↓ Prepare
Prepared
    ↓ Start
Executing (Running → Flushed → End-of-Stream)
    ↓ Stop → Reset
Prepared/Initialized
    ↓ Release
Released
```

#### 4.8 Surface模式（与Camera Framework配合）

**Surface模式特点**：
- 通过`CreateInputSurface()`获取输入Surface
- 可以与Camera Framework无缝对接
- 性能优于Buffer模式
- 零拷贝数据传输
- 无需处理OnInputBufferAvailable回调

**典型使用流程**：

```cpp
using namespace OHOS::MediaAVCodec;

// 1. 创建H.265编码器
auto encoder = VideoEncoderFactory::CreateByMime("video/hevc");

// 2. 注册回调（MediaCodecCallback风格）
class EncoderCallback : public MediaCodecCallback {
public:
    void OnError(AVCodecErrorType errorType, int32_t errorCode) override {
        // 处理错误
    }

    void OnOutputFormatChanged(const Format &format) override {
        // 处理格式变化
    }

    void OnInputBufferAvailable(uint32_t index,
                                std::shared_ptr<AVBuffer> buffer) override {
        // Surface模式下不需要处理
    }

    void OnOutputBufferAvailable(uint32_t index,
                                 std::shared_ptr<AVBuffer> buffer) override {
        // 获取编码后的数据
        auto attr = buffer->GetBufferAttr();
        uint8_t* data = buffer->GetAddr();
        int64_t pts = attr.presentationTimeUs;
        uint32_t size = attr.size;

        // 处理编码数据（通过通道发送）
        ProcessEncodedData(data, size, pts);

        // 释放Buffer
        encoder->ReleaseOutputBuffer(index);
    }
};

auto callback = std::make_shared<EncoderCallback>();
encoder->SetCallback(callback);

// 3. 配置编码器
Format format;
format.PutIntValue("width", 1920);              // 必须配置
format.PutIntValue("height", 1080);             // 必须配置
format.PutIntValue("pixel_format", NV12);       // 必须配置
format.PutDoubleValue("frame_rate", 30.0);
format.PutIntValue("bitrate", 5000000);         // 5Mbps
encoder->Configure(format);

// 4. 创建输入Surface（关键：在Prepare之前）
sptr<Surface> inputSurface = encoder->CreateInputSurface();

// 5. 准备并启动编码器
encoder->Prepare();
encoder->Start();

// 6. 将inputSurface传给Camera Framework
// （见Camera Framework章节的集成示例）

// 7. 通知流结束
encoder->NotifyEos();

// 8. 停止并释放编码器
encoder->Stop();
encoder->Release();
```

#### 4.9 与分布式相机的调用关系和调用流程

**调用关系图（Sink端）**：

```
Sink端分布式相机
    └── CameraClient组件
        ├── 调用Camera Framework
        │   └── 创建PreviewOutput（绑定AVCodec的Surface）
        └── 调用AVCodec VideoEncoder
            ├── VideoEncoderFactory::CreateByMime("video/hevc")
            ├── Configure(format)
            ├── CreateInputSurface() - 获取编码输入Surface
            ├── Prepare()
            ├── Start()
            └── SetCallback(callback)
                └── OnOutputBufferAvailable()
                    └── 通过Channel发送H.265数据给Source端
```

**完整调用流程（Sink端）**：

```
1. 分布式硬件框架通知使能相机
   ↓
2. Source端发送控制消息启动相机
   ↓
3. 创建H.265编码器
   auto encoder = VideoEncoderFactory::CreateByMime("video/hevc");
   ↓
4. 注册编码输出回调
   encoder->SetCallback(callback);
   ↓
5. 配置编码器
   encoder->Configure(format);  // width, height, pixel_format, frame_rate, bitrate
   ↓
6. 创建输入Surface（关键步骤）
   sptr<Surface> encoderSurface = encoder->CreateInputSurface();
   ↓
7. 同时初始化Camera Framework
   auto cameraManager = OHOS::Camera::CameraManager::GetInstance();
   auto cameraInput = cameraManager->CreateCameraInput(cameraId);
   auto captureSession = std::make_shared<OHOS::Camera::CaptureSession>();
   ↓
8. 创建PreviewOutput并绑定编码Surface
   auto previewSurface = Surface::CreateSurface();
   previewSurface->SetNativeWindow(encoderSurface);  // 绑定
   auto previewOutput = OHOS::Camera::PreviewOutput::Create(previewSurface);
   ↓
9. 三阶段提交配置
   captureSession->BeginConfig();
   captureSession->AddInput(cameraInput);
   captureSession->AddOutput(previewOutput);
   captureSession->CommitConfig();
   ↓
10. 准备并启动编码器
    encoder->Prepare();
    encoder->Start();
    ↓
11. 启动相机会话
    captureSession->Start();
    ↓
12. 相机开始输出YUV数据
    ↓
13. YUV数据通过Surface Buffer队列传递
    ↓
14. AVCodec编码器自动从Surface获取YUV数据
    ↓
15. H.265编码完成，触发OnOutputBufferAvailable回调
    ↓
16. 回调中通过Channel发送H.265数据给Source端
    channel->SendData(CHANNEL_TYPE_CONTINUOUS, data, size);
```

**测试用例如何交互**：

```cpp
// Mock AVCodec VideoEncoder

class MockVideoEncoder : public AVCodecVideoEncoder {
public:
    int32_t Configure(const Format &format) override {
        // 保存配置
        width_ = format.GetIntValue("width", 1920);
        height_ = format.GetIntValue("height", 1080);
        return 0;
    }

    int32_t Prepare() override {
        prepared_ = true;
        return 0;
    }

    int32_t Start() override {
        started_ = true;
        return 0;
    }

    int32_t Stop() override {
        started_ = false;
        return 0;
    }

    int32_t Release() override {
        prepared_ = false;
        return 0;
    }

    sptr<Surface> CreateInputSurface() override {
        // 返回模拟的Surface
        return mockSurface_;
    }

    int32_t SetCallback(const std::shared_ptr<AVCodecCallback> &callback) override {
        callback_ = callback;
        return 0;
    }

    int32_t SetCallback(const std::shared_ptr<MediaCodecCallback> &callback) override {
        mediaCallback_ = callback;
        return 0;
    }

    // 模拟输出编码数据
    void SimulateEncodedOutput(uint32_t index, const std::vector<uint8_t>& data,
                               int64_t pts) {
        if (mediaCallback_) {
            auto buffer = std::make_shared<AVBuffer>();
            buffer->SetData(data.data(), data.size());

            AVCodecBufferInfo info;
            info.presentationTimeUs = pts;
            info.size = data.size();
            buffer->SetBufferAttr(info);

            mediaCallback_->OnOutputBufferAvailable(index, buffer);
        }
    }

private:
    int width_ = 1920;
    int height_ = 1080;
    bool prepared_ = false;
    bool started_ = false;
    sptr<Surface> mockSurface_;
    std::shared_ptr<MediaCodecCallback> mediaCallback_;
};

class MockVideoEncoderFactory {
public:
    static std::shared_ptr<AVCodecVideoEncoder> CreateByMime(const std::string &mime) {
        return std::make_shared<MockVideoEncoder>();
    }
};

// 测试流程
// 1. 创建Mock编码器
auto encoder = MockVideoEncoderFactory::CreateByMime("video/hevc");

// 2. 配置
Format format;
format.PutIntValue("width", 1920);
format.PutIntValue("height", 1080);
encoder->Configure(format);

// 3. 创建Surface
auto surface = encoder->CreateInputSurface();
ASSERT_NE(surface, nullptr);

// 4. 启动编码器
encoder->Prepare();
encoder->Start();

// 5. 模拟输出编码数据
std::vector<uint8_t> testFrame = {0x00, 0x00, 0x01, ...};  // H.265帧
encoder->SimulateEncodedOutput(0, testFrame, 0);

// 6. 验证回调是否被调用
// （通过Mock回调验证）
```

---

### 5. HDF驱动框架和HDI接口

分布式相机的HDI接口定义在`C:\Work\OpenHarmony\distributed_camera_integrator\drivers_interface\distributed_camera`目录下。

HDI（Hardware Driver Interface）是连接上层服务和底层驱动的桥梁，Source端通过HDI接口与虚拟相机驱动通信。

#### 5.1 系统能力定义

```cpp
// HDI接口定义
// Package: ohos.hdi.distributed_camera.v1_0
// Interface: IDCameraProvider
```

#### 5.2 核心头文件位置

```
C:\Work\OpenHarmony\distributed_camera_integrator\drivers_interface\distributed_camera\
├── v1_0\
│   ├── DCameraTypes.idl              # 类型定义
│   ├── IDCameraProvider.idl          # Provider接口
│   └── IDCameraProviderCallback.idl  # Provider回调
└── v1_1\
    ├── DCameraTypes.idl
    ├── IDCameraProvider.idl
    ├── IDCameraProviderCallback.idl
    └── IDCameraHdfCallback.idl       # v1.1新增
```

#### 5.3 核心数据结构

**DCamRetCode** - 返回码

```cpp
enum DCamRetCode {
    SUCCESS = 0,                    // 成功
    CAMERA_BUSY = 1,                // 相机忙
    INVALID_ARGUMENT = 2,           // 无效参数
    METHOD_NOT_SUPPORTED = 3,       // 不支持的方法
    CAMERA_OFFLINE = 4,             // 相机离线
    EXCEED_MAX_NUMBER = 5,          // 超过最大数量
    DEVICE_NOT_INIT = 6,            // 设备未初始化
    FAILED = 7,                     // 失败
};
```

**DCEncodeType** - 编码类型

```cpp
enum DCEncodeType {
    ENCODE_TYPE_NULL = 0,           // 未指定
    ENCODE_TYPE_H264 = 1,           // H.264
    ENCODE_TYPE_H265 = 2,           // H.265
    ENCODE_TYPE_JPEG = 3,           // JPEG
    ENCODE_TYPE_MPEG4_ES = 4,       // MPEG4-ES
};
```

**DCStreamType** - 流类型

```cpp
enum DCStreamType {
    CONTINUOUS_FRAME = 0,           // 连续捕获流（预览/视频）
    SNAPSHOT_FRAME = 1,             // 单次捕获流（拍照）
};
```

**DHBase** - 分布式硬件基础信息

```cpp
struct DHBase {
    String deviceId_;                // 设备ID（networkId）
    String dhId_;                    // 分布式硬件ID
};
```

**DCameraSettings** - 相机设置

```cpp
struct DCameraSettings {
    enum DCSettingsType type_;       // 设置类型
    String value_;                   // 设置值（序列化的JSON）
};

enum DCSettingsType {
    UPDATE_METADATA = 0,             // 更新元数据
    ENABLE_METADATA = 1,             // 启用元数据
    DISABLE_METADATA = 2,            // 禁用元数据
    METADATA_RESULT = 3,             // 元数据结果
    SET_FLASH_LIGHT = 4,             // 设置闪光灯
    FPS_RANGE = 5,                   // 设置帧率范围
    UPDATE_FRAME_METADATA = 6,       // 更新帧元数据
};
```

**DCStreamInfo** - 流信息

```cpp
struct DCStreamInfo {
    int streamId_;                   // 流ID
    int width_;                      // 图像宽度
    int height_;                     // 图像高度
    int stride_;                     // 图像步长
    int format_;                     // 图像格式
    int dataspace_;                  // 色彩空间
    enum DCEncodeType encodeType_;   // 编码类型
    enum DCStreamType type_;         // 流类型
};
```

**DCCaptureInfo** - 捕获信息

```cpp
struct DCCaptureInfo {
    int[] streamIds_;                // 捕获的流ID列表
    int width_;                      // 图像宽度
    int height_;                     // 图像高度
    int stride_;                     // 图像步长
    int format_;                     // 图像格式
    int dataspace_;                  // 色彩空间
    boolean isCapture_;              // 是否触发Sink端捕获
    enum DCEncodeType encodeType_;   // 编码类型
    enum DCStreamType type_;         // 流类型
    struct DCameraSettings[] captureSettings_;  // 捕获设置
};
```

**DCameraBuffer** - 相机缓冲区

```cpp
struct DCameraBuffer {
    int index_;                      // 缓冲区索引
    unsigned int size_;              // 缓冲区大小
    NativeBuffer bufferHandle_;      // Native缓冲区句柄
};
```

**DCameraHDFEvent** - HDF事件

```cpp
struct DCameraHDFEvent {
    int type_;                       // 事件类型
    int result_;                     // 事件结果
    String content_;                 // 扩展内容（可选）
};
```

#### 5.4 HDI接口定义

**IDCameraProvider** - Provider接口

```idl
interface IDCameraProvider {
    // 使能分布式相机设备
    EnableDCameraDevice([in] struct DHBase dhBase,
                       [in] String abilityInfo,
                       [in] IDCameraProviderCallback callbackObj);

    // 去使能分布式相机设备
    DisableDCameraDevice([in] struct DHBase dhBase);

    // 获取帧缓冲区
    AcquireBuffer([in] struct DHBase dhBase,
                  [in] int streamId,
                  [out] struct DCameraBuffer buffer);

    // 提交填充后的帧缓冲区
    ShutterBuffer([in] struct DHBase dhBase,
                  [in] int streamId,
                  [in] struct DCameraBuffer buffer);

    // 上报元数据结果
    OnSettingsResult([in] struct DHBase dhBase,
                     [in] struct DCameraSettings result);

    // 通知事件
    Notify([in] struct DHBase dhBase,
           [in] struct DCameraHDFEvent event);
};
```

**IDCameraProviderCallback** - Provider回调接口

```idl
[callback] interface IDCameraProviderCallback {
    // 打开会话（创建传输通道）
    OpenSession([in] struct DHBase dhBase);

    // 关闭会话（销毁传输通道）
    CloseSession([in] struct DHBase dhBase);

    // 配置流
    ConfigureStreams([in] struct DHBase dhBase,
                     [in] struct DCStreamInfo[] streamInfos);

    // 释放流
    ReleaseStreams([in] struct DHBase dhBase,
                   [in] int[] streamIds);

    // 开始捕获
    StartCapture([in] struct DHBase dhBase,
                 [in] struct DCCaptureInfo[] captureInfos);

    // 停止捕获
    StopCapture([in] struct DHBase dhBase,
                [in] int[] streamIds);

    // 更新设置
    UpdateSettings([in] struct DHBase dhBase,
                   [in] struct DCameraSettings[] settings);
};
```

**IDCameraHdfCallback** (v1.1新增) - HDF事件回调

```idl
[callback] interface IDCameraHdfCallback {
    // 通知HDF事件
    NotifyEvent([in] int devId,
                [in] struct DCameraHDFEvent event);
};
```

#### 5.5 获取HDI服务

```cpp
#include "v1_0/idcamera_provider.h"
#include "iservice_registry.h"

using namespace OHOS::HDI::DistributedCamera::V1_0;

// 获取分布式相机HDI服务
auto provider = IDCameraProvider::Get();
if (provider == nullptr) {
    // 获取失败处理
}
```

#### 5.6 与分布式相机的调用关系和调用流程

**调用关系图（Source端）**：

```
Source端分布式相机
    └── DCameraSourceDev组件
        └── 调用HDI接口
            ├── EnableDCameraDevice() - 使能虚拟相机
            ├── AcquireBuffer() - 获取显示缓冲区
            ├── ShutterBuffer() - 提交填充后的缓冲区
            └── OnSettingsResult() - 上报元数据
                ↓
            IDCameraProvider实现（HDF驱动层）
                ├── 虚拟相机驱动
                ├── Surface Buffer管理
                └── 状态管理
                    ↓
            IDCameraProviderCallback回调
                ├── OpenSession() - 打开会话
                ├── ConfigureStreams() - 配置流
                ├── StartCapture() - 开始捕获
                └── StopCapture() - 停止捕获
                    ↓
            Channel模块（数据传输）
```

**完整调用流程（Source端）**：

```
1. 应用打开分布式相机
   ↓
2. 分布式硬件框架调用RegisterDistributedHardware()
   ↓
3. Source服务初始化
   ↓
4. 创建DCameraSourceDev
   ↓
5. 获取HDI服务
   auto provider = IDCameraProvider::Get();
   ↓
6. 使能虚拟相机设备
   provider->EnableDCameraDevice(dhBase, abilityInfo, callback);
   ↓
7. HDF驱动创建虚拟相机
   ↓
8. Camera Framework发现新相机
   ↓
9. 应用配置相机流
   ↓
10. 触发IDCameraProviderCallback::OpenSession()
    ↓
11. 建立与Sink端的Channel连接
    ↓
12. 触发IDCameraProviderCallback::ConfigureStreams()
    ↓
13. 协商三通道参数
    ↓
14. 触发IDCameraProviderCallback::StartCapture()
    ↓
15. 开始接收Sink端的编码数据
    ↓
16. 解码H.265数据为YUV
    ↓
17. 获取显示缓冲区
    provider->AcquireBuffer(dhBase, streamId, buffer);
    ↓
18. 填充YUV数据到Buffer
    ↓
19. 提交Buffer
    provider->ShutterBuffer(dhBase, streamId, buffer);
    ↓
20. 相机框架显示视频
```

**测试用例如何交互**：

```cpp
// Mock HDI Provider

class MockIDCameraProvider : public IDCameraProvider {
public:
    int32_t EnableDCameraDevice(const DHBase& dhBase,
                                const std::string& abilityInfo,
                                const sptr<IDCameraProviderCallback>& callbackObj) override {
        // 保存回调
        callback_ = callbackObj;
        dhBase_ = dhBase;
        enabled_ = true;
        return DCamRetCode::SUCCESS;
    }

    int32_t DisableDCameraDevice(const DHBase& dhBase) override {
        enabled_ = false;
        return DCamRetCode::SUCCESS;
    }

    int32_t AcquireBuffer(const DHBase& dhBase, int streamId,
                         DCameraBuffer& buffer) override {
        // 分配模拟的Buffer
        buffer.index_ = bufferIndex_++;
        buffer.size_ = 1920 * 1080 * 3 / 2;  // NV12
        buffer.bufferHandle_ = CreateMockBuffer(buffer.size_);
        return DCamRetCode::SUCCESS;
    }

    int32_t ShutterBuffer(const DHBase& dhBase, int streamId,
                         const DCameraBuffer& buffer) override {
        // 模拟Buffer提交
        submittedBuffers_.push_back(buffer);
        return DCamRetCode::SUCCESS;
    }

    // 触发OpenSession回调
    void SimulateOpenSession() {
        if (callback_) {
            callback_->OpenSession(dhBase_);
        }
    }

    // 触发ConfigureStreams回调
    void SimulateConfigureStreams(const std::vector<DCStreamInfo>& streamInfos) {
        if (callback_) {
            callback_->ConfigureStreams(dhBase_, streamInfos);
        }
    }

    // 触发StartCapture回调
    void SimulateStartCapture(const std::vector<DCCaptureInfo>& captureInfos) {
        if (callback_) {
            callback_->StartCapture(dhBase_, captureInfos);
        }
    }

private:
    sptr<IDCameraProviderCallback> callback_;
    DHBase dhBase_;
    bool enabled_ = false;
    int bufferIndex_ = 0;
    std::vector<DCameraBuffer> submittedBuffers_;
};

// 测试流程
auto mockProvider = std::make_shared<MockIDCameraProvider>();

// 1. 使能相机设备
DHBase dhBase;
dhBase.deviceId_ = "dev001";
dhBase.dhId_ = "dh001";
mockProvider->EnableDCameraDevice(dhBase, "{}", nullptr);

// 2. 模拟HDI驱动触发OpenSession
mockProvider->SimulateOpenSession();

// 3. 验证是否正确建立Channel
ASSERT_TRUE(IsChannelOpened("dh001"));

// 4. 模拟ConfigureStreams
std::vector<DCStreamInfo> streamInfos = {
    {0, 1920, 1080, 1920, NV12, 0, ENCODE_TYPE_H265, CONTINUOUS_FRAME}
};
mockProvider->SimulateConfigureStreams(streamInfos);

// 5. 模拟StartCapture
std::vector<DCCaptureInfo> captureInfos = {
    {{0}, 1920, 1080, 1920, NV12, 0, true, ENCODE_TYPE_H265, CONTINUOUS_FRAME, {}}
};
mockProvider->SimulateStartCapture(captureInfos);

// 6. 获取显示缓冲区
DCameraBuffer buffer;
mockProvider->AcquireBuffer(dhBase, 0, buffer);

// 7. 填充数据并提交
mockProvider->ShutterBuffer(dhBase, 0, buffer);

// 8. 验证
ASSERT_TRUE(mockProvider->IsBufferSubmitted(buffer.index_));
```

---

### 6. Surface和图形依赖

#### 6.1 系统能力定义

```cpp
// 组件定义
// Component: graphic_graphic
// Subsystem: graphic
// SystemCapability: SystemCapability.Graphic.Graphic2D
```

#### 6.2 核心头文件

```cpp
// 接口路径: C:\Work\OpenHarmony\graphic_graphic_surface\interfaces\inner_api
#include "surface_type.h"              // Surface类型定义
#include "surface.h"                   // Surface接口
#include "surface_buffer.h"            // Surface缓冲区
#include "iconsumer_surface.h"         // Consumer Surface接口
#include "native_window.h"             // NativeWindow接口
```

#### 6.3 核心API类

**Surface** - 图形缓冲区队列 (接口路径: `graphic_graphic_surface/interfaces/inner_api/surface/surface.h`)

```cpp
class Surface : public RefBase {
public:
    // 创建Surface
    static sptr<Surface> CreateSurfaceAsConsumer(std::string name = "noname");
    static sptr<Surface> CreateSurfaceAsProducer(sptr<IBufferProducer>& producer);

    // 请求Buffer（生产者）
    virtual GSError RequestBuffer(sptr<SurfaceBuffer>& buffer,
                                  int32_t &fence, BufferRequestConfig &config);
    virtual GSError RequestBuffer(sptr<SurfaceBuffer>& buffer,
                                  sptr<SyncFence>& fence, BufferRequestConfig &config);

    // 提交Buffer（生产者）
    virtual GSError FlushBuffer(sptr<SurfaceBuffer>& buffer,
                                int32_t fence, BufferFlushConfig &config);
    virtual GSError FlushBuffer(sptr<SurfaceBuffer>& buffer,
                                const sptr<SyncFence>& fence, BufferFlushConfig &config);

    // 获取Buffer（消费者）
    virtual GSError AcquireBuffer(sptr<SurfaceBuffer>& buffer, int32_t &fence,
                                  int64_t &timestamp, Rect &damage);
    virtual GSError AcquireBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
                                  int64_t &timestamp, Rect &damage);

    // 释放Buffer（消费者）
    virtual GSError ReleaseBuffer(sptr<SurfaceBuffer>& buffer, int32_t fence);
    virtual GSError ReleaseBuffer(sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence);

    // 附加/分离Buffer
    virtual GSError AttachBuffer(sptr<SurfaceBuffer>& buffer) = 0;
    virtual GSError DetachBuffer(sptr<SurfaceBuffer>& buffer) = 0;
    virtual GSError AttachBuffer(sptr<SurfaceBuffer>& buffer, int32_t timeOut) = 0;
    virtual GSError AttachBufferToQueue(sptr<SurfaceBuffer> buffer) = 0;
    virtual GSError DetachBufferFromQueue(sptr<SurfaceBuffer> buffer, bool isReserveSlot = false) = 0;

    // 队列管理
    virtual uint32_t GetQueueSize() = 0;
    virtual GSError SetQueueSize(uint32_t queueSize) = 0;

    // 元数据设置
    virtual GSError SetMetaData(uint32_t sequence, const std::vector<GraphicHDRMetaData> &metaData) = 0;
    virtual GSError SetMetaDataSet(uint32_t sequence, GraphicHDRMetadataKey key,
                                   const std::vector<uint8_t> &metaData) = 0;

    // 获取Producer
    virtual sptr<IBufferProducer> GetProducer() const = 0;

    // 注册消费者监听器
    virtual GSError RegisterConsumerListener(sptr<IBufferConsumerListener>& listener);
    virtual GSError RegisterConsumerListener(IBufferConsumerListenerClazz *listener);

    // 设置默认参数
    virtual GSError SetDefaultWidthAndHeight(int32_t width, int32_t height);
    virtual int32_t GetDefaultWidth() = 0;
    virtual int32_t GetDefaultHeight() = 0;
    virtual GSError SetDefaultUsage(uint64_t usage) = 0;
    virtual uint64_t GetDefaultUsage() = 0;
};
```

**IConsumerSurface** - 消费者Surface (接口路径: `graphic_graphic_surface/interfaces/inner_api/surface/iconsumer_surface.h`)

```cpp
class IConsumerSurface : public Surface {
public:
    static sptr<IConsumerSurface> Create(std::string name = "noname");

    virtual bool IsConsumer() const { return true; }
    virtual sptr<IBufferProducer> GetProducer() const;

    // AcquireBuffer扩展版本
    virtual GSError AcquireBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
                                  int64_t &timestamp, std::vector<Rect> &damages, bool isLppMode = false);

    using AcquireBufferReturnValue = struct {
        sptr<SurfaceBuffer> buffer;
        sptr<SyncFence> fence;
        int64_t timestamp;
        std::vector<Rect> damages;
    };
    virtual GSError AcquireBuffer(AcquireBufferReturnValue &returnValue, int64_t expectPresentTimestamp,
                                  bool isUsingAutoTimestamp);

    // 注册Surface代理
    virtual GSError RegisterSurfaceDelegator(sptr<IRemoteObject> client) = 0;
    virtual GSError UnregisterSurfaceDelegator();
};
```

**SurfaceBuffer** - 图形缓冲区 (接口路径: `graphic_graphic_surface/interfaces/inner_api/surface/surface_buffer.h`)

```cpp
class SurfaceBuffer : public RefBase {
public:
    // 获取BufferHandle
    virtual BufferHandle *GetBufferHandle() const = 0;

    // 获取虚拟地址
    virtual void* GetVirAddr() = 0;

    // 获取缓冲区大小
    virtual uint32_t GetSize() const = 0;

    // 设置/获取元数据
    virtual GSError SetMetadata(uint32_t key, const std::vector<uint8_t>& value, bool enableCache = true) = 0;
    virtual GSError GetMetadata(uint32_t key, std::vector<uint8_t>& value) = 0;

    // 分配/映射Buffer
    virtual GSError Alloc(const BufferRequestConfig &config,
                          const sptr<SurfaceBuffer>& previousBuffer = nullptr) = 0;
    virtual GSError Map() = 0;
    virtual GSError Unmap() = 0;
};
```

#### 6.4 与分布式相机的调用关系和调用流程

**调用关系图**：

```
Sink端：
    Camera Framework (PreviewOutput)
        └── 生产者Surface
            └── Queue Buffer队列
                └── 消费者Surface
                    └── AVCodec NativeWindow
                        └── VideoEncoder::CreateInputSurface()

Source端：
    AVCodec VideoDecoder
        └── 输出Surface
            └── Queue Buffer队列
                └── 消费者Surface
                    └── HDF虚拟相机
                        └── AcquireBuffer / ShutterBuffer
```

**测试用例如何交互**：

```cpp
// Mock Surface

class MockSurface : public Surface {
public:
    int32_t RequestBuffer(sptr<SurfaceBuffer>& buffer, int32_t& fence,
                          uint32_t width, uint32_t height) override {
        buffer = std::make_shared<MockSurfaceBuffer>();
        buffer->SetSize(width * height * 3 / 2);  // NV12
        fence = -1;
        return 0;
    }

    int32_t FlushBuffer(sptr<SurfaceBuffer> buffer, int32_t fence) override {
        flushedBuffers_.push_back(buffer);
        return 0;
    }

    int32_t SetNativeWindow(OHNativeWindow* window) override {
        nativeWindow_ = window;
        return 0;
    }

private:
    OHNativeWindow* nativeWindow_ = nullptr;
    std::vector<sptr<SurfaceBuffer>> flushedBuffers_;
};
```

---

## 完整工作流程

### 系统启动流程
```
1. 系统启动
   ↓
2. 设备组网上线
   ↓
3. 双端分布式硬件管理框架启动
   ├─ Source端分布式硬件框架启动
   └─ Sink端分布式硬件框架启动
   ↓
4. 拉起SA服务 (SAMGR注册SAID 4803/4804)
   ├─ Source端拉起DCameraSourceService
   └─ Sink端拉起DCameraSinkService
   ↓
5. Sink端获取相机能力并上报给Source端分布式硬件框架
   ↓
6. 分布式硬件框架调用RegisterDistributedHardware()
   ↓
7. DCameraSourceService创建虚拟相机HDF设备
   ↓
8. Camera Framework发现新相机
   ↓
9. 应用可查询和使用相机
```

### Source端完整调用链
```
应用层
    ↓
Camera Framework (OH_Camera*)
    ↓
Virtual Camera HDF
    ↓
Distributed Camera SDK
    ↓
DCameraSourceService (SA 4803)
    ├─ InitSource()
    ├─ RegisterDistributedHardware()
    │    ↓
    │   DCameraSourceDev
    │    ├─ OpenCamera(dhId)
    │    │    ├─ 创建虚拟相机HDF设备
    │    │    ├─ 配置Surface缓冲区
    │    │    └─ 创建ProducerSurface
    │    ├─ ConfigStreams()
    │    │    ├─ 协商三通道参数
    │    │    └─ 配置解码器
    │    └─ StartCapture()
    │         ├─ 启动Control通道
    │         ├─ 启动Snapshot通道
    │         └─ 启动Continuous通道
    │              ↓
    │         [接收Sink编码数据]
    │              ├─ H.265解码
    │              ├─ 格式转换(YUV→RGB)
    │              ├─ 转换到Surface
    │              └─ OnVideoFrame回调
    │
    └─ UnregisterDistributedHardware()
         └─ ReleaseSource()
```

### Sink端完整调用链
```
SA服务初始化 (SA 4804)
    ↓
DCameraSinkService
    ├─ InitSink()
    │    ↓
    │   // 接收Source端消息，触发CameraClient调用
    │   Channel模块接收Source端命令
    │    ↓
    │   CameraClient (本地相机访问层)
    │    ↓
    │   // 步骤1: 创建AVCodec编码器并获取Surface
    │   auto encoder = VideoEncoderFactory::CreateByMime("video/hevc");
    │   encoder->Configure(format);
    │   auto encoderSurface = encoder->CreateInputSurface();
    │    ↓
    │   // 步骤2: 创建Camera Framework会话
    │   Camera Framework (@ohos/camera_framework)
    │    ├─ CameraManager::GetInstance()
    │    ├─ GetSupportedCameras()
    │    ├─ CreateCameraInput(cameraId)
    │    │    ↓
    │    │   CaptureSession (三阶段提交)
    │    │    ├─ BeginConfig()
    │    │    ├─ AddInput(cameraInput)
    │    │    ├─ AddOutput(previewOutput)  // 绑定AVCodec的Surface
    │    │    ├─ CommitConfig()
    │    │    └─ Start()
    │    │         ↓
    │    │   Camera Service (camera_service, SA #3009)
    │    │    ↓
    │    │   HDI → 本地相机硬件
    │    │         ↓
    │    │    [相机原始YUV数据]
    │    │         ↓
    │    │    PreviewOutput Surface → AVCodec encoderSurface
    │    │         ↓
    │    │    [AVCodec Surface模式自动编码]
    │    │         ├─ YUV数据自动送入编码器
    │    │         ├─ H.265编码
    │    │         └─ OnOutputBufferAvailable回调
    │    │              ↓
    │    │    [编码后的H.265数据]
    │    │         ↓
    │    │         Channel模块
    │    │             ├─ Control通道(命令)
    │    │             ├─ Snapshot通道(JPEG图片)
    │    │             └─ Continuous通道(H.265视频)
    │    │                  ↓
    └──────────────────────┴─────────────────────→ Source端
```

## 错误码定义

```cpp
// 成功
constexpr int32_t DCAMERA_OK = 0;

// 通用错误
constexpr int32_t DCAMERA_ERR = -1;

// 参数错误
constexpr int32_t DCAMERA_INVALID_VALUE = -2;

// 状态错误
constexpr int32_t DCAMERA_STATE_ERROR = -3;

// 权限错误
constexpr int32_t DCAMERA_PERMISSION_ERR = -4;

// 超时错误
constexpr int32_t DCAMERA_TIMEOUT_ERR = -5;

// 资源不足
constexpr int32_t DCAMERA_NO_MEMORY = -6;
```
