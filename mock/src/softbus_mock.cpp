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

#include "softbus_mock.h"
#include "socket.h"
#include "trans_type.h"
#include "distributed_hardware_log.h"

#include <algorithm>
#include <chrono>
#include <sstream>

namespace OHOS {
namespace DistributedHardware {

// 魔数定义
static const uint32_t PACKET_MAGIC = 0x53425453;  // "SBTS"

// SoftbusMock实现
SoftbusMock::SoftbusMock()
    : nextSocketId_(100)
    , isInitialized_(false)
#ifdef _WIN32
    , winsockInitialized_(false)
#endif
{
    std::memset(&statistics_, 0, sizeof(Statistics));
}

SoftbusMock::~SoftbusMock() {
    Deinitialize();
}

SoftbusMock& SoftbusMock::GetInstance() {
    static SoftbusMock instance;
    return instance;
}

int32_t SoftbusMock::Initialize(const SoftbusMockConfig& config) {
    std::lock_guard<std::mutex> lock(socketMutex_);

    if (isInitialized_) {
        DHLOGW("SoftbusMock already initialized");
        return 0;
    }

    config_ = config;
    nextPort_ = config_.basePort;

#ifdef _WIN32
    // 初始化Winsock
    if (!winsockInitialized_) {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            DHLOGE("WSAStartup failed with error: %d", result);
            return -1;
        }
        winsockInitialized_ = true;
    }
#endif

    isInitialized_ = true;
    DHLOGI("SoftbusMock initialized successfully");
    return 0;
}

void SoftbusMock::Deinitialize() {
    std::lock_guard<std::mutex> lock(socketMutex_);

    if (!isInitialized_) {
        return;
    }

    // 停止所有接收线程
    for (auto& pair : receiveThreads_) {
        threadRunningFlags_[pair.first] = false;
        if (pair.second.joinable()) {
            pair.second.join();
        }
    }
    receiveThreads_.clear();
    threadRunningFlags_.clear();

    // 停止所有接受线程
    for (auto& pair : acceptThreads_) {
        if (pair.second.joinable()) {
            pair.second.join();
        }
    }
    acceptThreads_.clear();

    // 关闭所有Socket
    for (auto& pair : sockets_) {
        if (pair.second->tcpSocket != INVALID_SOFTBUS_SOCKET) {
            CloseTcpSocket(pair.second->tcpSocket);
        }
    }
    sockets_.clear();
    sessionKeyToSocket_.clear();

    // 关闭所有服务器Socket
    for (auto& pair : serverSockets_) {
        CloseTcpSocket(pair.second);
    }
    serverSockets_.clear();

    // 清空端口使用记录
    usedPorts_.clear();

#ifdef _WIN32
    if (winsockInitialized_) {
        WSACleanup();
        winsockInitialized_ = false;
    }
#endif

    isInitialized_ = false;
    DHLOGI("SoftbusMock deinitialized");
}

int32_t SoftbusMock::Socket(const SocketInfo& info) {
    std::lock_guard<std::mutex> lock(socketMutex_);

    if (!isInitialized_) {
        DHLOGE("SoftbusMock not initialized");
        return -1;
    }

    // 验证输入参数
    if (!info.name || !info.peerName || !info.peerNetworkId || !info.pkgName) {
        DHLOGE("Invalid SocketInfo parameters");
        return -1;
    }

    // 检查Socket数量限制
    if (sockets_.size() >= config_.maxSockets) {
        DHLOGE("Maximum socket limit reached: %u", config_.maxSockets);
        return -1;
    }

    // 生成新的Socket ID
    int32_t socketId = GenerateSocketId();

    // 创建Socket信息
    auto socketInfo = std::make_shared<SoftbusSocketInfo>();
    socketInfo->socketId = socketId;
    socketInfo->name = info.name;
    socketInfo->peerName = info.peerName;
    socketInfo->peerNetworkId = info.peerNetworkId;
    socketInfo->pkgName = info.pkgName;
    socketInfo->dataType = info.dataType;
    socketInfo->state = SOCKET_STATE_IDLE;
    socketInfo->tcpSocket = INVALID_SOFTBUS_SOCKET;
    socketInfo->listener = nullptr;
    socketInfo->localPort = 0;
    socketInfo->peerPort = 0;
    socketInfo->localIp = config_.localIp;
    socketInfo->peerIp = config_.localIp;  // 默认为本地

    // 根据会话名确定通道类型
    socketInfo->channelType = GetChannelType(info.name);

    sockets_[socketId] = socketInfo;

    // 生成会话键
    std::string sessionKey = GetSessionKey(info.name, info.peerNetworkId);
    sessionKeyToSocket_[sessionKey] = socketId;

    // 更新统计信息
    {
        std::lock_guard<std::mutex> statsLock(statsMutex_);
        statistics_.totalSocketsCreated++;
    }

    DHLOGI("Created socket %d for session %s, channel type: %d",
           socketId, info.name, socketInfo->channelType);

    return socketId;
}

int32_t SoftbusMock::Listen(int32_t socket, const QosTV qos[], uint32_t qosCount,
                             const ISocketListener* listener) {
    std::lock_guard<std::mutex> lock(socketMutex_);

    if (!IsSocketIdValid(socket)) {
        DHLOGE("Invalid socket ID: %d", socket);
        return -1;
    }

    auto socketInfo = sockets_[socket];
    if (socketInfo->state != SOCKET_STATE_IDLE) {
        DHLOGE("Socket %d not in IDLE state", socket);
        return -1;
    }

    // 获取可用端口
    uint16_t port;
    {
        std::lock_guard<std::mutex> portLock(portMutex_);
        for (uint32_t i = 0; i < 100; i++) {
            if (usedPorts_.find(nextPort_) == usedPorts_.end()) {
                port = nextPort_;
                usedPorts_.insert(port);
                nextPort_++;
                break;
            }
            nextPort_++;
        }
        if (port == 0) {
            DHLOGE("No available ports");
            return -1;
        }
    }

    // 创建TCP服务器
    softbus_socket_t serverSocket = CreateTcpServer(port);
    if (serverSocket == INVALID_SOFTBUS_SOCKET) {
        DHLOGE("Failed to create TCP server on port %d", port);
        return -1;
    }

    // 更新Socket信息
    socketInfo->state = SOCKET_STATE_LISTENING;
    socketInfo->tcpSocket = serverSocket;
    socketInfo->localPort = port;
    socketInfo->listener = listener;

    // 存储服务器Socket
    std::string serverKey = socketInfo->name;
    serverSockets_[serverKey] = serverSocket;

    // 启动接受连接的线程
    StartAcceptThread(socket);

    DHLOGI("Socket %d listening on port %d", socket, port);
    return 0;
}

int32_t SoftbusMock::Bind(int32_t socket, const QosTV qos[], uint32_t qosCount,
                           const ISocketListener* listener) {
    std::lock_guard<std::mutex> lock(socketMutex_);

    if (!IsSocketIdValid(socket)) {
        DHLOGE("Invalid socket ID: %d", socket);
        return -1;
    }

    auto socketInfo = sockets_[socket];
    if (socketInfo->state != SOCKET_STATE_IDLE) {
        DHLOGE("Socket %d not in IDLE state", socket);
        return -1;
    }

    // 连接到服务器
    // 在实际实现中，需要从某个地方获取服务器的IP和端口
    // 这里简化处理，假设服务器在同一台机器上
    std::string serverIp = config_.localIp;
    uint16_t serverPort = config_.basePort;  // 需要根据实际情况调整

    softbus_socket_t clientSocket = ConnectToTcpServer(serverIp, serverPort);
    if (clientSocket == INVALID_SOFTBUS_SOCKET) {
        DHLOGE("Failed to connect to TCP server %s:%d", serverIp.c_str(), serverPort);
        return -1;
    }

    // 更新Socket信息
    socketInfo->state = SOCKET_STATE_BOUND;
    socketInfo->tcpSocket = clientSocket;
    socketInfo->peerIp = serverIp;
    socketInfo->peerPort = serverPort;
    socketInfo->listener = listener;

    // 启动接收线程
    StartReceiveThread(socket);

    // 触发OnBind回调
    PeerSocketInfo peerInfo;
    peerInfo.name = const_cast<char*>(socketInfo->peerName.c_str());
    peerInfo.networkId = const_cast<char*>(socketInfo->peerNetworkId.c_str());
    peerInfo.pkgName = const_cast<char*>(socketInfo->pkgName.c_str());
    peerInfo.dataType = socketInfo->dataType;

    TriggerOnBind(socket, peerInfo);

    DHLOGI("Socket %d bound to %s:%d", socket, serverIp.c_str(), serverPort);
    return socket;
}

int32_t SoftbusMock::SendBytes(int32_t socket, const void* data, uint32_t len) {
    if (!data || len == 0) {
        DHLOGE("Invalid data parameters");
        return -1;
    }

    std::lock_guard<std::mutex> lock(socketMutex_);

    if (!IsSocketIdValid(socket)) {
        DHLOGE("Invalid socket ID: %d", socket);
        return -1;
    }

    auto socketInfo = sockets_[socket];
    if (socketInfo->state != SOCKET_STATE_BOUND && socketInfo->state != SOCKET_STATE_CONNECTED) {
        DHLOGE("Socket %d not bound", socket);
        return -1;
    }

    return SendDataPacket(socket, data, len, 1);  // dataType = 1 for Bytes
}

int32_t SoftbusMock::SendMessage(int32_t socket, const void* data, uint32_t len) {
    if (!data || len == 0) {
        DHLOGE("Invalid data parameters");
        return -1;
    }

    std::lock_guard<std::mutex> lock(socketMutex_);

    if (!IsSocketIdValid(socket)) {
        DHLOGE("Invalid socket ID: %d", socket);
        return -1;
    }

    auto socketInfo = sockets_[socket];
    if (socketInfo->state != SOCKET_STATE_BOUND && socketInfo->state != SOCKET_STATE_CONNECTED) {
        DHLOGE("Socket %d not bound", socket);
        return -1;
    }

    return SendDataPacket(socket, data, len, 0);  // dataType = 0 for Message
}

int32_t SoftbusMock::SendStream(int32_t socket, const StreamData* data, const StreamData* ext,
                                 const StreamFrameInfo* param) {
    if (!data || !param) {
        DHLOGE("Invalid stream parameters");
        return -1;
    }

    std::lock_guard<std::mutex> lock(socketMutex_);

    if (!IsSocketIdValid(socket)) {
        DHLOGE("Invalid socket ID: %d", socket);
        return -1;
    }

    auto socketInfo = sockets_[socket];
    if (socketInfo->state != SOCKET_STATE_BOUND && socketInfo->state != SOCKET_STATE_CONNECTED) {
        DHLOGE("Socket %d not bound", socket);
        return -1;
    }

    // 构建流数据包
    StreamPacketHeader streamHeader;
    streamHeader.base.magic = PACKET_MAGIC;
    streamHeader.base.dataLength = data ? data->bufLen : 0;
    streamHeader.base.dataType = 2;  // Stream type
    streamHeader.base.sequence = 0;
    streamHeader.base.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    streamHeader.frameType = param ? param->frameType : 0;
    streamHeader.timeStamp = param ? param->timeStamp : 0;
    streamHeader.seqNum = param ? param->seqNum : 0;
    streamHeader.seqSubNum = param ? param->seqSubNum : 0;
    streamHeader.level = param ? param->level : 0;
    streamHeader.bitMap = param ? param->bitMap : 0;
    streamHeader.base.checksum = 0;

    // 计算校验和
    if (config_.enableDataCheck && data && data->buf) {
        streamHeader.base.checksum = CalculateChecksum(data->buf, data->bufLen);
    }

    // 发送流包头
    int bytesSent = send(socketInfo->tcpSocket,
                         reinterpret_cast<const char*>(&streamHeader),
                         sizeof(StreamPacketHeader), 0);

    if (bytesSent != sizeof(StreamPacketHeader)) {
        DHLOGE("Failed to send stream header");
        {
            std::lock_guard<std::mutex> statsLock(statsMutex_);
            statistics_.sendErrors++;
        }
        return -1;
    }

    // 发送流数据
    if (data && data->buf && data->bufLen > 0) {
        bytesSent = send(socketInfo->tcpSocket, data->buf, data->bufLen, 0);
        if (bytesSent != data->bufLen) {
            DHLOGE("Failed to send stream data");
            {
                std::lock_guard<std::mutex> statsLock(statsMutex_);
                statistics_.sendErrors++;
            }
            return -1;
        }
    }

    // 更新统计信息
    {
        std::lock_guard<std::mutex> statsLock(statsMutex_);
        statistics_.totalBytesSent += sizeof(StreamPacketHeader) + (data ? data->bufLen : 0);
        statistics_.totalPacketsSent++;
    }

    return 0;
}

void SoftbusMock::Shutdown(int32_t socket) {
    std::lock_guard<std::mutex> lock(socketMutex_);

    if (!IsSocketIdValid(socket)) {
        DHLOGE("Invalid socket ID: %d", socket);
        return;
    }

    auto socketInfo = sockets_[socket];

    // 停止接收线程
    StopThread(socket);

    // 关闭TCP Socket
    if (socketInfo->tcpSocket != INVALID_SOFTBUS_SOCKET) {
        CloseTcpSocket(socketInfo->tcpSocket);
        socketInfo->tcpSocket = INVALID_SOFTBUS_SOCKET;
    }

    // 释放端口
    if (socketInfo->localPort != 0) {
        std::lock_guard<std::mutex> portLock(portMutex_);
        usedPorts_.erase(socketInfo->localPort);
    }

    // 移除服务器Socket
    if (!socketInfo->name.empty()) {
        serverSockets_.erase(socketInfo->name);
    }

    // 移除会话键映射
    std::string sessionKey = GetSessionKey(socketInfo->name, socketInfo->peerNetworkId);
    sessionKeyToSocket_.erase(sessionKey);

    // 更新状态
    socketInfo->state = SOCKET_STATE_CLOSED;

    DHLOGI("Socket %d shutdown", socket);
}

int32_t SoftbusMock::EvaluateQos(const char* peerNetworkId, TransDataType dataType,
                                  const QosTV* qos, uint32_t qosCount) {
    // 简单实现，直接返回成功
    DHLOGI("EvaluateQos for peer %s, dataType %d", peerNetworkId, dataType);
    return 0;
}

std::shared_ptr<SoftbusSocketInfo> SoftbusMock::GetSocketInfo(int32_t socket) {
    std::lock_guard<std::mutex> lock(socketMutex_);

    if (!IsSocketIdValid(socket)) {
        return nullptr;
    }

    return sockets_[socket];
}

bool SoftbusMock::IsSocketValid(int32_t socket) {
    std::lock_guard<std::mutex> lock(socketMutex_);
    return IsSocketIdValid(socket);
}

SoftbusChannelType SoftbusMock::GetChannelType(const std::string& sessionName) {
    // 根据会话名后缀确定通道类型
    if (sessionName.find("Control") != std::string::npos ||
        sessionName.find("control") != std::string::npos) {
        return CHANNEL_TYPE_CONTROL;
    } else if (sessionName.find("Snapshot") != std::string::npos ||
               sessionName.find("snapshot") != std::string::npos) {
        return CHANNEL_TYPE_SNAPSHOT;
    } else if (sessionName.find("Continuous") != std::string::npos ||
               sessionName.find("continuous") != std::string::npos) {
        return CHANNEL_TYPE_CONTINUOUS;
    }

    // 默认返回控制通道类型
    return CHANNEL_TYPE_CONTROL;
}

std::string SoftbusMock::GetChannelSuffix(SoftbusChannelType type) {
    switch (type) {
        case CHANNEL_TYPE_CONTROL:
            return "Control";
        case CHANNEL_TYPE_SNAPSHOT:
            return "Snapshot";
        case CHANNEL_TYPE_CONTINUOUS:
            return "Continuous";
        default:
            return "Control";
    }
}

const SoftbusMock::Statistics& SoftbusMock::GetStatistics() const {
    return statistics_;
}

void SoftbusMock::ResetStatistics() {
    std::lock_guard<std::mutex> lock(statsMutex_);
    std::memset(&statistics_, 0, sizeof(Statistics));
}

// 私有方法实现

int32_t SoftbusMock::GenerateSocketId() {
    return nextSocketId_++;
}

bool SoftbusMock::IsSocketIdValid(int32_t socketId) {
    return sockets_.find(socketId) != sockets_.end();
}

void SoftbusMock::CloseTcpSocket(softbus_socket_t sock) {
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}

std::string SoftbusMock::GetSessionKey(const std::string& name, const std::string& peerNetworkId) {
    return name + "_" + peerNetworkId;
}

int32_t SoftbusMock::CreateTcpServer(uint16_t port) {
    softbus_socket_t serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOFTBUS_SOCKET) {
        DHLOGE("Failed to create server socket");
        return INVALID_SOFTBUS_SOCKET;
    }

    // 设置SO_REUSEADDR
    int opt = 1;
#ifdef _WIN32
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
#else
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    // 绑定到端口
    sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == -1) {
        DHLOGE("Failed to bind server socket to port %d", port);
        CloseTcpSocket(serverSocket);
        return INVALID_SOFTBUS_SOCKET;
    }

    // 监听
    if (listen(serverSocket, SOMAXCONN) == -1) {
        DHLOGE("Failed to listen on server socket");
        CloseTcpSocket(serverSocket);
        return INVALID_SOFTBUS_SOCKET;
    }

    return serverSocket;
}

int32_t SoftbusMock::ConnectToTcpServer(const std::string& ip, uint16_t port) {
    softbus_socket_t clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOFTBUS_SOCKET) {
        DHLOGE("Failed to create client socket");
        return INVALID_SOFTBUS_SOCKET;
    }

    sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);

    if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == -1) {
        DHLOGE("Failed to connect to server %s:%d", ip.c_str(), port);
        CloseTcpSocket(clientSocket);
        return INVALID_SOFTBUS_SOCKET;
    }

    return static_cast<int32_t>(clientSocket);
}

void SoftbusMock::AcceptConnections(int32_t socket) {
    auto socketInfo = GetSocketInfo(socket);
    if (!socketInfo) {
        return;
    }

    DHLOGI("Accept thread started for socket %d", socket);

    while (threadRunningFlags_[socket]) {
        sockaddr_in clientAddr;
#ifdef _WIN32
        int addrLen = sizeof(clientAddr);
#else
        socklen_t addrLen = sizeof(clientAddr);
#endif

        softbus_socket_t clientSocket = accept(socketInfo->tcpSocket,
                                                reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);

        if (clientSocket == INVALID_SOFTBUS_SOCKET) {
            if (threadRunningFlags_[socket]) {
                DHLOGE("Accept failed for socket %d", socket);
            }
            break;
        }

        // 获取客户端地址
        char clientIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, INET_ADDRSTRLEN);
        uint16_t clientPort = ntohs(clientAddr.sin_port);

        DHLOGI("Accepted connection from %s:%d", clientIp, clientPort);

        // 在实际实现中，这里需要为新的客户端连接创建一个新的Socket ID
        // 简化处理，直接将客户端Socket添加到现有的Socket信息中
        // 实际应用中需要更复杂的处理
    }

    DHLOGI("Accept thread ended for socket %d", socket);
}

void SoftbusMock::ReceiveData(int32_t socket) {
    auto socketInfo = GetSocketInfo(socket);
    if (!socketInfo) {
        return;
    }

    DHLOGI("Receive thread started for socket %d", socket);

    std::vector<uint8_t> buffer(config_.receiveBufferSize);

    while (threadRunningFlags_[socket]) {
        std::vector<uint8_t> data;
        int32_t result = ReceiveDataPacket(socket, data);

        if (result <= 0) {
            if (threadRunningFlags_[socket]) {
                DHLOGE("Receive failed for socket %d", socket);
                TriggerOnShutdown(socket, SHUTDOWN_REASON_PEER);
            }
            break;
        }

        // 分发接收到的数据
        DispatchData(socket, data);
    }

    DHLOGI("Receive thread ended for socket %d", socket);
}

int32_t SoftbusMock::SendDataPacket(int32_t socket, const void* data, uint32_t len, uint32_t dataType) {
    auto socketInfo = sockets_[socket];

    // 构建数据包头
    DataPacketHeader header;
    header.magic = PACKET_MAGIC;
    header.dataLength = len;
    header.dataType = dataType;
    header.sequence = 0;
    header.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    header.checksum = 0;

    // 计算校验和
    if (config_.enableDataCheck) {
        header.checksum = CalculateChecksum(data, len);
    }

    // 发送包头
    int bytesSent = send(socketInfo->tcpSocket, reinterpret_cast<const char*>(&header),
                         sizeof(DataPacketHeader), 0);
    if (bytesSent != sizeof(DataPacketHeader)) {
        DHLOGE("Failed to send packet header");
        {
            std::lock_guard<std::mutex> statsLock(statsMutex_);
            statistics_.sendErrors++;
        }
        return -1;
    }

    // 发送数据
    bytesSent = send(socketInfo->tcpSocket, static_cast<const char*>(data), len, 0);
    if (bytesSent != len) {
        DHLOGE("Failed to send packet data");
        {
            std::lock_guard<std::mutex> statsLock(statsMutex_);
            statistics_.sendErrors++;
        }
        return -1;
    }

    // 更新统计信息
    {
        std::lock_guard<std::mutex> statsLock(statsMutex_);
        statistics_.totalBytesSent += sizeof(DataPacketHeader) + len;
        statistics_.totalPacketsSent++;
    }

    return len;
}

int32_t SoftbusMock::ReceiveDataPacket(int32_t socket, std::vector<uint8_t>& buffer) {
    auto socketInfo = sockets_[socket];

    // 接收包头
    DataPacketHeader header;
    int bytesReceived = recv(socketInfo->tcpSocket, reinterpret_cast<char*>(&header),
                             sizeof(DataPacketHeader), MSG_WAITALL);

    if (bytesReceived != sizeof(DataPacketHeader)) {
        DHLOGE("Failed to receive packet header");
        return -1;
    }

    // 验证魔数
    if (header.magic != PACKET_MAGIC) {
        DHLOGE("Invalid packet magic: 0x%08X", header.magic);
        return -1;
    }

    // 调整缓冲区大小
    buffer.resize(sizeof(DataPacketHeader) + header.dataLength);

    // 复制包头
    std::memcpy(buffer.data(), &header, sizeof(DataPacketHeader));

    // 接收数据
    if (header.dataLength > 0) {
        bytesReceived = recv(socketInfo->tcpSocket,
                             reinterpret_cast<char*>(buffer.data() + sizeof(DataPacketHeader)),
                             header.dataLength, MSG_WAITALL);

        if (bytesReceived != static_cast<int>(header.dataLength)) {
            DHLOGE("Failed to receive packet data");
            return -1;
        }
    }

    // 验证校验和
    if (config_.enableDataCheck && header.checksum != 0) {
        uint32_t calculatedChecksum = CalculateChecksum(buffer.data() + sizeof(DataPacketHeader),
                                                         header.dataLength);
        if (calculatedChecksum != header.checksum) {
            DHLOGE("Checksum mismatch");
            return -1;
        }
    }

    // 更新统计信息
    {
        std::lock_guard<std::mutex> statsLock(statsMutex_);
        statistics_.totalBytesReceived += sizeof(DataPacketHeader) + header.dataLength;
        statistics_.totalPacketsReceived++;
    }

    return static_cast<int32_t>(sizeof(DataPacketHeader) + header.dataLength);
}

void SoftbusMock::DispatchData(int32_t socket, const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(DataPacketHeader)) {
        DHLOGE("Invalid data packet size");
        return;
    }

    const DataPacketHeader* header = reinterpret_cast<const DataPacketHeader*>(data.data());
    const uint8_t* payload = data.data() + sizeof(DataPacketHeader);

    switch (header->dataType) {
        case 0:  // Message
            TriggerOnMessage(socket, payload, header->dataLength);
            break;

        case 1:  // Bytes
            TriggerOnBytes(socket, payload, header->dataLength);
            break;

        case 2:  // Stream
            {
                const StreamPacketHeader* streamHeader = reinterpret_cast<const StreamPacketHeader*>(header);
                StreamData streamData;
                streamData.buf = const_cast<char*>(reinterpret_cast<const char*>(payload));
                streamData.bufLen = streamHeader->base.dataLength;

                StreamFrameInfo frameInfo;
                frameInfo.frameType = streamHeader->frameType;
                frameInfo.timeStamp = streamHeader->timeStamp;
                frameInfo.seqNum = streamHeader->seqNum;
                frameInfo.seqSubNum = streamHeader->seqSubNum;
                frameInfo.level = streamHeader->level;
                frameInfo.bitMap = streamHeader->bitMap;
                frameInfo.tvCount = 0;
                frameInfo.tvList = nullptr;

                TriggerOnStream(socket, &streamData, nullptr, &frameInfo);
            }
            break;

        default:
            DHLOGE("Unknown data type: %u", header->dataType);
            break;
    }
}

void SoftbusMock::TriggerOnBind(int32_t socket, const PeerSocketInfo& info) {
    auto socketInfo = sockets_[socket];
    if (socketInfo->listener && socketInfo->listener->OnBind) {
        socketInfo->listener->OnBind(socket, info);
    }
}

void SoftbusMock::TriggerOnShutdown(int32_t socket, ShutdownReason reason) {
    auto socketInfo = sockets_[socket];
    if (socketInfo->listener && socketInfo->listener->OnShutdown) {
        socketInfo->listener->OnShutdown(socket, reason);
    }
}

void SoftbusMock::TriggerOnBytes(int32_t socket, const void* data, uint32_t dataLen) {
    auto socketInfo = sockets_[socket];
    if (socketInfo->listener && socketInfo->listener->OnBytes) {
        socketInfo->listener->OnBytes(socket, data, dataLen);
    }
}

void SoftbusMock::TriggerOnMessage(int32_t socket, const void* data, uint32_t dataLen) {
    auto socketInfo = sockets_[socket];
    if (socketInfo->listener && socketInfo->listener->OnMessage) {
        socketInfo->listener->OnMessage(socket, data, dataLen);
    }
}

void SoftbusMock::TriggerOnStream(int32_t socket, const StreamData* data, const StreamData* ext,
                                   const StreamFrameInfo* param) {
    auto socketInfo = sockets_[socket];
    if (socketInfo->listener && socketInfo->listener->OnStream) {
        socketInfo->listener->OnStream(socket, data, ext, param);
    }
}

void SoftbusMock::TriggerOnQos(int32_t socket, int32_t eventId, const QosTV* qos, uint32_t qosCount) {
    auto socketInfo = sockets_[socket];
    if (socketInfo->listener && socketInfo->listener->OnQos) {
        socketInfo->listener->OnQos(socket, static_cast<QoSEvent>(eventId), qos, qosCount);
    }
}

uint32_t SoftbusMock::CalculateChecksum(const void* data, uint32_t len) {
    uint32_t checksum = 0;
    const uint8_t* bytes = static_cast<const uint8_t*>(data);

    for (uint32_t i = 0; i < len; i++) {
        checksum += bytes[i];
    }

    return checksum;
}

void SoftbusMock::StartReceiveThread(int32_t socket) {
    threadRunningFlags_[socket] = true;
    receiveThreads_[socket] = std::thread(&SoftbusMock::ReceiveData, this, socket);
}

void SoftbusMock::StartAcceptThread(int32_t socket) {
    acceptThreads_[socket] = std::thread(&SoftbusMock::AcceptConnections, this, socket);
}

void SoftbusMock::StopThread(int32_t socket) {
    threadRunningFlags_[socket] = false;

    auto it = receiveThreads_.find(socket);
    if (it != receiveThreads_.end()) {
        if (it->second.joinable()) {
            it->second.join();
        }
        receiveThreads_.erase(it);
    }
}

} // namespace DistributedHardware
} // namespace OHOS

// C接口实现
extern "C" {

int32_t Socket(SocketInfo info) {
    return OHOS::DistributedHardware::SoftbusMock::GetInstance().Socket(info);
}

int32_t Listen(int32_t socket, const QosTV qos[], uint32_t qosCount,
               const ISocketListener* listener) {
    return OHOS::DistributedHardware::SoftbusMock::GetInstance().Listen(socket, qos, qosCount, listener);
}

int32_t Bind(int32_t socket, const QosTV qos[], uint32_t qosCount,
             const ISocketListener* listener) {
    return OHOS::DistributedHardware::SoftbusMock::GetInstance().Bind(socket, qos, qosCount, listener);
}

int32_t SendBytes(int32_t socket, const void* data, uint32_t len) {
    return OHOS::DistributedHardware::SoftbusMock::GetInstance().SendBytes(socket, data, len);
}

int32_t SendMessage(int32_t socket, const void* data, uint32_t len) {
    return OHOS::DistributedHardware::SoftbusMock::GetInstance().SendMessage(socket, data, len);
}

int32_t SendStream(int32_t socket, const StreamData* data, const StreamData* ext,
                   const StreamFrameInfo* param) {
    return OHOS::DistributedHardware::SoftbusMock::GetInstance().SendStream(socket, data, ext, param);
}

void Shutdown(int32_t socket) {
    OHOS::DistributedHardware::SoftbusMock::GetInstance().Shutdown(socket);
}

int32_t EvaluateQos(const char* peerNetworkId, TransDataType dataType,
                    const QosTV* qos, uint32_t qosCount) {
    return OHOS::DistributedHardware::SoftbusMock::GetInstance().EvaluateQos(peerNetworkId, dataType, qos, qosCount);
}

// Mock控制接口
int32_t SoftbusMock_Initialize(const OHOS::DistributedHardware::SoftbusMockConfig* config) {
    if (config) {
        return OHOS::DistributedHardware::SoftbusMock::GetInstance().Initialize(*config);
    } else {
        return OHOS::DistributedHardware::SoftbusMock::GetInstance().Initialize();
    }
}

void SoftbusMock_Deinitialize() {
    OHOS::DistributedHardware::SoftbusMock::GetInstance().Deinitialize();
}

void SoftbusMock_ResetStatistics() {
    OHOS::DistributedHardware::SoftbusMock::GetInstance().ResetStatistics();
}

} // extern "C"
