/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OHOS_SOFTBUS_MOCK_H
#define OHOS_SOFTBUS_MOCK_H

#include <cstdint>
#include <cstring>
#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET softbus_socket_t;
#define INVALID_SOFTBUS_SOCKET INVALID_SOCKET
#else
typedef int softbus_socket_t;
#define INVALID_SOFTBUS_SOCKET (-1)
#endif

// Forward declarations
struct SocketInfo;
struct PeerSocketInfo;
struct QosTV;
struct ISocketListener;
struct StreamData;
struct StreamFrameInfo;
enum ShutdownReason;
enum TransDataType;

namespace OHOS {
namespace DistributedHardware {

// 三通道类型定义
enum SoftbusChannelType {
    CHANNEL_TYPE_CONTROL = 0,    // 控制通道 - 用于控制指令
    CHANNEL_TYPE_SNAPSHOT = 1,   // 抓拍通道 - 用于拍照图片传输
    CHANNEL_TYPE_CONTINUOUS = 2  // 连续通道 - 用于视频流传输
};

// Socket状态
enum SocketState {
    SOCKET_STATE_IDLE = 0,       // 未连接
    SOCKET_STATE_BINDING,        // 绑定中
    SOCKET_STATE_BOUND,          // 已绑定
    SOCKET_STATE_LISTENING,      // 监听中
    SOCKET_STATE_CONNECTED,      // 已连接
    SOCKET_STATE_CLOSED          // 已关闭
};

// Socket信息结构
struct SoftbusSocketInfo {
    int32_t socketId;                    // Socket ID
    std::string name;                    // Socket名称
    std::string peerName;                // 对端Socket名称
    std::string peerNetworkId;           // 对端网络ID
    std::string pkgName;                 // 包名
    TransDataType dataType;              // 数据类型
    SocketState state;                   // Socket状态
    SoftbusChannelType channelType;      // 通道类型
    softbus_socket_t tcpSocket;          // 底层TCP Socket
    const ISocketListener* listener;     // 监听器
    uint16_t localPort;                  // 本地端口
    uint16_t peerPort;                   // 对端端口
    std::string localIp;                 // 本地IP
    std::string peerIp;                  // 对端IP
};

// 数据包头
struct DataPacketHeader {
    uint32_t magic;          // 魔数 0x53425453 ("SBTS")
    uint32_t dataLength;     // 数据长度
    uint32_t dataType;       // 数据类型 (0=Bytes, 1=Message, 2=Stream)
    uint32_t sequence;       // 序列号
    uint64_t timestamp;      // 时间戳
    uint32_t checksum;       // 校验和
};

// Stream数据包头
struct StreamPacketHeader {
    DataPacketHeader base;           // 基础包头
    int32_t frameType;               // 帧类型 (I帧/P帧)
    int64_t timeStamp;               // 时间戳
    int32_t seqNum;                  // 序列号
    int32_t seqSubNum;               // 子序列号
    int32_t level;                   // SVC级别
    int32_t bitMap;                  // 位图
};

// SoftBus配置
struct SoftbusMockConfig {
    std::string localIp = "127.0.0.1";       // 本地IP
    uint16_t basePort = 50000;               // 起始端口
    uint32_t maxSockets = 64;                // 最大Socket数量
    uint32_t receiveBufferSize = 2 * 1024 * 1024;  // 接收缓冲区大小 (2MB)
    uint32_t sendBufferSize = 2 * 1024 * 1024;     // 发送缓冲区大小 (2MB)
    bool enableDataCheck = true;             // 启用数据校验
    uint32_t socketTimeout = 30000;          // Socket超时 (ms)
};

/**
 * @brief SoftBus Socket API Mock类
 *
 * 该类提供了与SoftBus Socket API兼容的Mock实现，使用本地TCP Socket作为底层传输。
 * 支持三通道通信模式（Control/Snapshot/Continuous）。
 */
class SoftbusMock {
public:
    // 获取单例实例
    static SoftbusMock& GetInstance();

    // 初始化Mock
    int32_t Initialize(const SoftbusMockConfig& config = SoftbusMockConfig());
    void Deinitialize();

    // Socket API实现
    int32_t Socket(const SocketInfo& info);
    int32_t Listen(int32_t socket, const QosTV qos[], uint32_t qosCount, const ISocketListener* listener);
    int32_t Bind(int32_t socket, const QosTV qos[], uint32_t qosCount, const ISocketListener* listener);
    int32_t SendBytes(int32_t socket, const void* data, uint32_t len);
    int32_t SendMessage(int32_t socket, const void* data, uint32_t len);
    int32_t SendStream(int32_t socket, const StreamData* data, const StreamData* ext, const StreamFrameInfo* param);
    void Shutdown(int32_t socket);
    int32_t EvaluateQos(const char* peerNetworkId, TransDataType dataType, const QosTV* qos, uint32_t qosCount);

    // 获取Socket信息
    std::shared_ptr<SoftbusSocketInfo> GetSocketInfo(int32_t socket);
    bool IsSocketValid(int32_t socket);

    // 获取通道类型
    static SoftbusChannelType GetChannelType(const std::string& sessionName);
    static std::string GetChannelSuffix(SoftbusChannelType type);

    // 统计信息
    struct Statistics {
        uint64_t totalBytesSent = 0;
        uint64_t totalBytesReceived = 0;
        uint64_t totalPacketsSent = 0;
        uint64_t totalPacketsReceived = 0;
        uint32_t activeConnections = 0;
        uint32_t totalSocketsCreated = 0;
        uint64_t sendErrors = 0;
        uint64_t receiveErrors = 0;
    };
    const Statistics& GetStatistics() const;
    void ResetStatistics();

private:
    SoftbusMock();
    ~SoftbusMock();

    // 禁止拷贝和赋值
    SoftbusMock(const SoftbusMock&) = delete;
    SoftbusMock& operator=(const SoftbusMock&) = delete;

    // 内部辅助方法
    int32_t GenerateSocketId();
    bool IsSocketIdValid(int32_t socketId);
    void CloseTcpSocket(softbus_socket_t sock);
    std::string GetSessionKey(const std::string& name, const std::string& peerNetworkId);

    // TCP相关操作
    int32_t CreateTcpServer(uint16_t port);
    int32_t ConnectToTcpServer(const std::string& ip, uint16_t port);
    void AcceptConnections(int32_t socket);
    void ReceiveData(int32_t socket);

    // 数据收发处理
    int32_t SendDataPacket(int32_t socket, const void* data, uint32_t len, uint32_t dataType);
    int32_t ReceiveDataPacket(int32_t socket, std::vector<uint8_t>& buffer);
    void DispatchData(int32_t socket, const std::vector<uint8_t>& data);

    // 回调触发
    void TriggerOnBind(int32_t socket, const PeerSocketInfo& info);
    void TriggerOnShutdown(int32_t socket, ShutdownReason reason);
    void TriggerOnBytes(int32_t socket, const void* data, uint32_t dataLen);
    void TriggerOnMessage(int32_t socket, const void* data, uint32_t dataLen);
    void TriggerOnStream(int32_t socket, const StreamData* data, const StreamData* ext, const StreamFrameInfo* param);
    void TriggerOnQos(int32_t socket, int32_t eventId, const QosTV* qos, uint32_t qosCount);

    // 校验和计算
    uint32_t CalculateChecksum(const void* data, uint32_t len);

    // 线程管理
    void StartReceiveThread(int32_t socket);
    void StartAcceptThread(int32_t socket);
    void StopThread(int32_t socket);

private:
    // 配置
    SoftbusMockConfig config_;
    bool isInitialized_;

    // Socket管理
    std::mutex socketMutex_;
    std::map<int32_t, std::shared_ptr<SoftbusSocketInfo>> sockets_;
    int32_t nextSocketId_;
    std::map<std::string, int32_t> sessionKeyToSocket_;

    // 服务器Socket管理
    std::mutex serverMutex_;
    std::map<std::string, softbus_socket_t> serverSockets_;
    std::map<int32_t, std::thread> acceptThreads_;

    // 接收线程管理
    std::map<int32_t, std::thread> receiveThreads_;
    std::map<int32_t, bool> threadRunningFlags_;

    // 统计信息
    Statistics statistics_;
    std::mutex statsMutex_;

    // 端口管理
    std::mutex portMutex_;
    std::set<uint16_t> usedPorts_;
    uint16_t nextPort_;

    // Winsock初始化
#ifdef _WIN32
    bool winsockInitialized_;
#endif
};

} // namespace DistributedHardware
} // namespace OHOS

// C接口 - 与SoftBus API兼容
#ifdef __cplusplus
extern "C" {
#endif

// 导出C风格的Socket API
int32_t Socket(SocketInfo info);
int32_t Listen(int32_t socket, const QosTV qos[], uint32_t qosCount, const ISocketListener* listener);
int32_t Bind(int32_t socket, const QosTV qos[], uint32_t qosCount, const ISocketListener* listener);
int32_t SendBytes(int32_t socket, const void* data, uint32_t len);
int32_t SendMessage(int32_t socket, const void* data, uint32_t len);
int32_t SendStream(int32_t socket, const StreamData* data, const StreamData* ext, const StreamFrameInfo* param);
void Shutdown(int32_t socket);
int32_t EvaluateQos(const char* peerNetworkId, TransDataType dataType, const QosTV* qos, uint32_t qosCount);

// Mock控制接口
int32_t SoftbusMock_Initialize(const OHOS::DistributedHardware::SoftbusMockConfig* config);
void SoftbusMock_Deinitialize();
void SoftbusMock_ResetStatistics();

#ifdef __cplusplus
}
#endif

#endif // OHOS_SOFTBUS_MOCK_H
