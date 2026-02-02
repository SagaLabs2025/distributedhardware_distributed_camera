# SoftBus Mock 快速入门指南

## 快速开始

### 1. 编译Mock库和测试程序

```bash
# 进入mock目录
cd mock

# 创建构建目录
mkdir -p build
cd build

# 配置CMake
cmake ..

# 编译
cmake --build . --config Debug
```

### 2. 运行单元测试

```bash
# 运行基础单元测试
./softbus_mock_test

# 预期输出：
# [INFO] ========== SoftBus Mock Test Start ==========
# [INFO] Test: Initialize
# [INFO] Initialize: PASSED
# [INFO] Test: Socket Create
# [INFO] Socket Create: PASSED (sink=100, source=101)
# [INFO] Test: Server Listen
# [INFO] Server Listen: PASSED
# ...
# [INFO] All tests PASSED
```

### 3. 运行集成测试

```bash
# 运行分布式相机集成测试
./softbus_mock_integration

# 预期输出：
# [INFO] ========== DCamera SoftBus Integration Test ==========
# [INFO] Initializing DCamera SoftBus Integration
# [INFO] DCamera SoftBus Integration initialized successfully
# [INFO] Creating Sink servers
# [INFO] Sink server created for DCameraSinkControl (socket=100)
# [INFO] Sink server created for DCameraSinkSnapshot (socket=101)
# [INFO] Sink server created for DCameraSinkContinuous (socket=102)
# ...
# [INFO] ========== Integration Test Complete ==========
```

## 三通道测试

### 控制通道测试

控制通道用于传输相机控制指令：

```cpp
// 发送控制指令
const char* controlCmd = "{\"cmd\":\"start_preview\",\"params\":{}}";
SendBytes(sourceControlSocket, controlCmd, strlen(controlCmd) + 1);
```

### 抓拍通道测试

抓拍通道用于传输拍照图片数据：

```cpp
// 发送拍照数据
const char* snapshotData = "MOCK_SNAPSHOT_IMAGE_DATA";
SendBytes(sourceSnapshotSocket, snapshotData, strlen(snapshotData) + 1);
```

### 连续通道测试

连续通道用于传输视频流：

```cpp
// 发送视频帧
StreamData streamData = {};
streamData.buf = frameBuffer;
streamData.bufLen = frameSize;

StreamFrameInfo frameInfo = {};
frameInfo.frameType = 1;  // I帧
frameInfo.timeStamp = GetCurrentTime();
frameInfo.seqNum = 0;

SendStream(sourceContinuousSocket, &streamData, nullptr, &frameInfo);
```

## 查看统计信息

```cpp
const auto& stats = SoftbusMock::GetInstance().GetStatistics();

printf("Total Bytes Sent: %llu\n", stats.totalBytesSent);
printf("Total Packets: %llu\n", stats.totalPacketsSent);
```

## 常见问题

### Q: 如何修改端口范围？
A: 在初始化时配置：
```cpp
SoftbusMockConfig config;
config.basePort = 60000;  // 修改起始端口
config.maxSockets = 128;  // 修改最大Socket数
SoftbusMock::GetInstance().Initialize(config);
```

### Q: 如何启用/禁用数据校验？
A: 修改配置：
```cpp
config.enableDataCheck = false;  // 禁用校验和
```

### Q: 如何调整缓冲区大小？
A: 修改配置：
```cpp
config.receiveBufferSize = 4 * 1024 * 1024;  // 4MB
config.sendBufferSize = 4 * 1024 * 1024;     // 4MB
```

### Q: Socket创建失败怎么办？
A: 检查以下几点：
1. 是否已调用Initialize()
2. 端口是否已被占用
3. 是否超过maxSockets限制
4. SocketInfo参数是否有效

## 下一步

- 阅读 `SOFTBUS_MOCK_README.md` 了解详细API
- 查看 `softbus_mock_integration.cpp` 学习集成示例
- 参考 `SOFTBUS_MOCK_IMPLEMENTATION.md` 了解实现细节

## 技术支持

如遇到问题，请检查：
1. 编译日志中的错误信息
2. 运行时的日志输出
3. 网络端口是否被占用
4. 防火墙设置是否阻止连接
