# SoftBus Socket API Mock 实现说明

## 概述

SoftBus Mock提供了与OpenHarmony SoftBus Socket API兼容的Mock实现，使用本地TCP Socket作为底层传输机制。该实现支持分布式相机三通道通信模式（Control/Snapshot/Continuous），便于在本地开发和测试环境中模拟设备间通信。

## 功能特性

### 1. Socket API兼容性
完整实现了SoftBus Socket API的主要接口：
- `Socket()` - 创建Socket
- `Listen()` - 服务器端监听
- `Bind()` - 客户端绑定
- `SendBytes()` - 发送字节数据
- `SendMessage()` - 发送消息数据
- `SendStream()` - 发送流数据
- `Shutdown()` - 关闭Socket
- `EvaluateQos()` - QoS评估

### 2. 三通道支持
支持分布式相机的三种通道类型：
- **Control通道** - 用于控制指令传输
- **Snapshot通道** - 用于拍照图片传输
- **Continuous通道** - 用于视频流传输

### 3. 数据传输特性
- 自定义数据包协议（魔数、校验和）
- 支持Bytes/Message/Stream三种数据类型
- 帧信息封装（I帧/P帧、时间戳、序列号等）
- 统计信息收集

## 文件结构

```
mock/
├── include/
│   └── softbus_mock.h          # SoftBus Mock头文件
├── src/
│   └── softbus_mock.cpp        # SoftBus Mock实现
└── test/
    └── softbus_mock_test.cpp   # 单元测试
```

## 使用方法

### 1. 初始化Mock

```cpp
#include "softbus_mock.h"

using namespace OHOS::DistributedHardware;

// 使用默认配置初始化
SoftbusMock::GetInstance().Initialize();

// 或使用自定义配置
SoftbusMockConfig config;
config.localIp = "127.0.0.1";
config.basePort = 50000;
config.maxSockets = 64;
config.receiveBufferSize = 2 * 1024 * 1024;
config.sendBufferSize = 2 * 1024 * 1024;
config.enableDataCheck = true;
config.socketTimeout = 30000;

SoftbusMock::GetInstance().Initialize(config);
```

### 2. 创建服务器端（Sink）

```cpp
// 定义回调函数
void OnBind(int32_t socket, PeerSocketInfo info) {
    DHLOGI("Client connected: socket=%d", socket);
}

void OnBytes(int32_t socket, const void* data, uint32_t dataLen) {
    DHLOGI("Received %u bytes", dataLen);
}

void OnStream(int32_t socket, const StreamData* data,
              const StreamData* ext, const StreamFrameInfo* param) {
    DHLOGI("Received stream: frameType=%d", param->frameType);
}

// 创建Socket
SocketInfo info = {};
info.name = "DCameraSinkControl";
info.peerName = "DCameraSourceControl";
info.peerNetworkId = "PEER_DEVICE_001";
info.pkgName = "ohos.dhardware.dcamera";
info.dataType = DATA_TYPE_BYTES;

int32_t socket = Socket(info);

// 设置监听器
ISocketListener listener = {};
listener.OnBind = OnBind;
listener.OnBytes = OnBytes;
listener.OnStream = OnStream;

// 设置QoS
QosTV qos[] = {
    {QOS_TYPE_MIN_BW, 1000000},
    {QOS_TYPE_MAX_LATENCY, 100}
};

// 开始监听
Listen(socket, qos, 2, &listener);
```

### 3. 创建客户端（Source）

```cpp
// 创建Socket（类似服务器端）
SocketInfo info = {};
info.name = "DCameraSourceControl";
info.peerName = "DCameraSinkControl";
info.peerNetworkId = "PEER_DEVICE_001";
info.pkgName = "ohos.dhardware.dcamera";
info.dataType = DATA_TYPE_BYTES;

int32_t socket = Socket(info);

// 设置监听器和QoS（同服务器端）
ISocketListener listener = {};
QosTV qos[] = {...};

// 连接到服务器
Bind(socket, qos, 2, &listener);
```

### 4. 发送数据

#### 发送字节数据
```cpp
const char* data = "Hello, SoftBus!";
SendBytes(socket, data, strlen(data) + 1);
```

#### 发送消息
```cpp
const char* msg = "Test message";
SendMessage(socket, msg, strlen(msg) + 1);
```

#### 发送流数据
```cpp
// 准备流数据
char* dataBuf = new char[dataSize];
// ... 填充数据 ...

StreamData streamData = {};
streamData.buf = dataBuf;
streamData.bufLen = dataSize;

StreamFrameInfo frameInfo = {};
frameInfo.frameType = 1;  // I帧
frameInfo.timeStamp =GetCurrentTime();
frameInfo.seqNum = 0;
frameInfo.seqSubNum = 0;
frameInfo.level = 0;
frameInfo.bitMap = 0;

SendStream(socket, &streamData, nullptr, &frameInfo);

delete[] dataBuf;
```

### 5. 关闭Socket
```cpp
Shutdown(socket);

// 清理Mock（程序结束时）
SoftbusMock::GetInstance().Deinitialize();
```

## 数据包格式

### 基础数据包头
```cpp
struct DataPacketHeader {
    uint32_t magic;          // 魔数 0x53425453 ("SBTS")
    uint32_t dataLength;     // 数据长度
    uint32_t dataType;       // 数据类型 (0=Message, 1=Bytes, 2=Stream)
    uint32_t sequence;       // 序列号
    uint64_t timestamp;      // 时间戳
    uint32_t checksum;       // 校验和
};
```

### 流数据包头
```cpp
struct StreamPacketHeader {
    DataPacketHeader base;   // 基础包头
    int32_t frameType;       // 帧类型 (I帧/P帧)
    int64_t timeStamp;       // 时间戳
    int32_t seqNum;          // 序列号
    int32_t seqSubNum;       // 子序列号
    int32_t level;           // SVC级别
    int32_t bitMap;          // 位图
};
```

## 通道类型识别

Mock根据会话名自动识别通道类型：

```cpp
// 控制通道
"DCameraSinkControl"      -> CHANNEL_TYPE_CONTROL
"DCameraSourceControl"    -> CHANNEL_TYPE_CONTROL

// 抓拍通道
"DCameraSinkSnapshot"     -> CHANNEL_TYPE_SNAPSHOT
"DCameraSourceSnapshot"   -> CHANNEL_TYPE_SNAPSHOT

// 连续通道
"DCameraSinkContinuous"   -> CHANNEL_TYPE_CONTINUOUS
"DCameraSourceContinuous" -> CHANNEL_TYPE_CONTINUOUS
```

## 统计信息

Mock提供详细的通信统计信息：

```cpp
const auto& stats = SoftbusMock::GetInstance().GetStatistics();

DHLOGI("Total Bytes Sent: %llu", stats.totalBytesSent);
DHLOGI("Total Bytes Received: %llu", stats.totalBytesReceived);
DHLOGI("Total Packets Sent: %llu", stats.totalPacketsSent);
DHLOGI("Total Packets Received: %llu", stats.totalPacketsReceived);
DHLOGI("Active Connections: %u", stats.activeConnections);
DHLOGI("Total Sockets Created: %u", stats.totalSocketsCreated);
DHLOGI("Send Errors: %llu", stats.sendErrors);
DHLOGI("Receive Errors: %llu", stats.receiveErrors);

// 重置统计信息
SoftbusMock::GetInstance().ResetStatistics();
```

## C接口

为了兼容性，提供了C风格接口：

```c
// 初始化和清理
int32_t SoftbusMock_Initialize(const SoftbusMockConfig* config);
void SoftbusMock_Deinitialize();
void SoftbusMock_ResetStatistics();

// Socket API
int32_t Socket(SocketInfo info);
int32_t Listen(int32_t socket, const QosTV qos[], uint32_t qosCount,
               const ISocketListener* listener);
int32_t Bind(int32_t socket, const QosTV qos[], uint32_t qosCount,
             const ISocketListener* listener);
int32_t SendBytes(int32_t socket, const void* data, uint32_t len);
int32_t SendMessage(int32_t socket, const void* data, uint32_t len);
int32_t SendStream(int32_t socket, const StreamData* data,
                   const StreamData* ext, const StreamFrameInfo* param);
void Shutdown(int32_t socket);
int32_t EvaluateQos(const char* peerNetworkId, TransDataType dataType,
                    const QosTV* qos, uint32_t qosCount);
```

## 编译和测试

### 编译Mock库
```bash
cd mock/build
cmake ..
cmake --build .
```

### 运行测试
```bash
# 运行SoftBus Mock单元测试
./softbus_mock_test
```

## 注意事项

1. **端口管理**：Mock使用从basePort开始的端口范围，确保端口可用。

2. **线程安全**：Mock内部使用互斥锁保护共享资源，但回调函数应在不同线程中谨慎处理。

3. **资源清理**：使用完毕后务必调用Shutdown和Deinitialize清理资源。

4. **性能考虑**：对于大规模数据传输，建议调整receiveBufferSize和sendBufferSize。

5. **Winsock**：Windows平台需要Winsock支持，Mock会自动初始化WSA。

6. **错误处理**：所有接口返回-1表示失败，应检查返回值。

## 与实际SoftBus的差异

1. **传输层**：Mock使用TCP，实际SoftBus可能使用多种传输方式。

2. **设备发现**：Mock不实现设备发现功能，需预先配置IP和端口。

3. **加密**：Mock不实现数据加密，仅用于测试。

4. **性能**：Mock性能可能与实际SoftBus有差异。

## 示例：三通道设置

```cpp
// 控制通道
SetupChannel("DCameraSourceControl", "DCameraSinkControl",
             DATA_TYPE_BYTES, 50000);

// 抓拍通道
SetupChannel("DCameraSourceSnapshot", "DCameraSinkSnapshot",
             DATA_TYPE_BYTES, 50001);

// 连续通道
SetupChannel("DCameraSourceContinuous", "DCameraSinkContinuous",
             DATA_TYPE_VIDEO_STREAM, 50002);
```

## 支持的平台

- Windows (MSVC)
- Linux (GCC/Clang)
- macOS (Clang)
