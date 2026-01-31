/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "socket_adapter.h"
#include "distributed_hardware_log.h"

#include <sstream>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

const char* SocketAdapter::DEFAULT_HOST = "127.0.0.1";

SocketAdapter::SocketAdapter()
    : sockfd_(INVALID_SOCKET), connected_(false), receiving_(false), initialized_(false) {
}

SocketAdapter::~SocketAdapter() {
    StopReceiving();
    closesocket(sockfd_);

#ifdef _WIN32
    WSACleanup();
#endif
}

int32_t SocketAdapter::Initialize() {
    DHLOGI("[SOCKET_ADAPTER] Initialize");

#ifdef _WIN32
    // Windows: 初始化 Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        DHLOGE("[SOCKET_ADAPTER] WSAStartup failed");
        return -1;
    }
#endif

    initialized_.store(true);
    DHLOGI("[SOCKET_ADAPTER] Initialize success");
    return 0;
}

int32_t SocketAdapter::ConnectToSink() {
    DHLOGI("[SOCKET_ADAPTER] Connecting to Sink at %{public}s:%{public}d",
            DEFAULT_HOST, DEFAULT_PORT);

    if (!initialized_.load()) {
        DHLOGE("[SOCKET_ADAPTER] Not initialized");
        return -1;
    }

    // 创建 socket
    sockfd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd_ == INVALID_SOCKET) {
        DHLOGE("[SOCKET_ADAPTER] socket creation failed");
        return -1;
    }

    // 连接到 Sink 端
    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(DEFAULT_PORT);

    if (inet_pton(AF_INET, DEFAULT_HOST, &serverAddr.sin_addr) <= 0) {
        DHLOGE("[SOCKET_ADAPTER] Invalid address");
        closesocket(sockfd_);
        return -1;
    }

    if (connect(sockfd_, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        DHLOGW("[SOCKET_ADAPTER] Connection failed (Sink may not be ready yet)");
        // 不关闭socket，稍后可以重试
        return -1;
    }

    connected_.store(true);
    DHLOGI("[SOCKET_ADAPTER] Connected to Sink successfully");
    return 0;
}

int32_t SocketAdapter::SendConfigureMessage(const std::vector<DCStreamInfo>& streamInfos) {
    DHLOGI("[SOCKET_ADAPTER] Sending ConfigureStreams message, count: %{public}zu",
            streamInfos.size());

    std::string message = SerializeStreamInfos(streamInfos);
    return SendControlMessage("CONFIGURE_STREAMS:" + message);
}

int32_t SocketAdapter::SendStartCaptureMessage(const std::vector<DCCaptureInfo>& captureInfos) {
    DHLOGI("[SOCKET_ADAPTER] Sending StartCapture message, count: %{public}zu",
            captureInfos.size());

    std::string message = SerializeCaptureInfos(captureInfos);
    return SendControlMessage("START_CAPTURE:" + message);
}

int32_t SocketAdapter::SendStopCaptureMessage() {
    DHLOGI("[SOCKET_ADAPTER] Sending StopCapture message");
    return SendControlMessage("STOP_CAPTURE:");
}

int32_t SocketAdapter::StartReceiving() {
    DHLOGI("[SOCKET_ADAPTER] Starting receive thread");

    if (receiving_.load()) {
        DHLOGW("[SOCKET_ADAPTER] Already receiving");
        return 0;
    }

    receiving_.store(true);
    receiveThread_ = std::thread(&SocketAdapter::ReceiveThreadProc, this);

    DHLOGI("[SOCKET_ADAPTER] Receive thread started");
    return 0;
}

void SocketAdapter::StopReceiving() {
    DHLOGI("[SOCKET_ADAPTER] Stopping receive thread");

    receiving_.store(false);

    if (receiveThread_.joinable()) {
        receiveThread_.join();
    }

    DHLOGI("[SOCKET_ADAPTER] Receive thread stopped");
}

int32_t SocketAdapter::SendControlMessage(const std::string& message) {
    if (!connected_.load()) {
        DHLOGE("[SOCKET_ADAPTER] Not connected");
        return -1;
    }

    // 发送消息长度
    uint32_t msgLen = static_cast<uint32_t>(message.size());

    std::lock_guard<std::mutex> lock(mutex_);

    int sent = send(sockfd_, reinterpret_cast<const char*>(&msgLen), sizeof(msgLen), 0);
    if (sent == SOCKET_ERROR) {
        DHLOGE("[SOCKET_ADAPTER] Failed to send message length");
        return -1;
    }

    // 发送消息内容
    sent = send(sockfd_, message.data(), static_cast<int>(message.size()), 0);
    if (sent == SOCKET_ERROR) {
        DHLOGE("[SOCKET_ADAPTER] Failed to send message content");
        return -1;
    }

    DHLOGI("[SOCKET_ADAPTER] Control message sent: %{public}s", message.c_str());
    return 0;
}

void SocketAdapter::ReceiveThreadProc() {
    DHLOGI("[SOCKET_ADAPTER] Receive thread proc started");

    while (receiving_.load()) {
        if (!connected_.load()) {
            // 尝试重连
            ConnectToSink();
            if (!connected_.load()) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }
        }

        // 接收数据
        int32_t ret = ReceiveData();
        if (ret < 0) {
            // 连接断开
            connected_.store(false);
            closesocket(sockfd_);
            sockfd_ = INVALID_SOCKET;
        }
    }

    DHLOGI("[SOCKET_ADAPTER] Receive thread proc ended");
}

int32_t SocketAdapter::ReceiveData() {
    // 首先接收数据长度
    uint32_t dataLen = 0;
    int received = recv(sockfd_, reinterpret_cast<char*>(&dataLen), sizeof(dataLen), 0);
    if (received <= 0) {
        return -1;
    }

    // 分配缓冲区并接收数据
    std::vector<uint8_t> buffer(dataLen);
    int totalReceived = 0;
    while (totalReceived < static_cast<int>(dataLen)) {
        received = recv(sockfd_, reinterpret_cast<char*>(buffer.data()) + totalReceived,
                       dataLen - totalReceived, 0);
        if (received <= 0) {
            return -1;
        }
        totalReceived += received;
    }

    // 触发数据回调
    if (dataCallback_) {
        dataCallback_(buffer.data(), buffer.size());
    }

    return 0;
}

std::string SocketAdapter::SerializeStreamInfos(const std::vector<DCStreamInfo>& streamInfos) {
    std::ostringstream oss;
    oss << "{";
    for (size_t i = 0; i < streamInfos.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "{\"id\":" << streamInfos[i].streamId_
            << ",\"w\":" << streamInfos[i].width_
            << ",\"h\":" << streamInfos[i].height_
            << ",\"fmt\":" << streamInfos[i].format_
            << ",\"enc\":" << streamInfos[i].encodeType_
            << ",\"type\":" << streamInfos[i].type_ << "}";
    }
    oss << "}";
    return oss.str();
}

std::string SocketAdapter::SerializeCaptureInfos(const std::vector<DCCaptureInfo>& captureInfos) {
    std::ostringstream oss;
    oss << "{";
    for (size_t i = 0; i < captureInfos.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "{";
        // streamIds
        oss << "\"ids\":[";
        for (size_t j = 0; j < captureInfos[i].streamIds_.size(); ++j) {
            if (j > 0) oss << ",";
            oss << captureInfos[i].streamIds_[j];
        }
        oss << "],";
        oss << "\"w\":" << captureInfos[i].width_
            << ",\"h\":" << captureInfos[i].height_
            << ",\"fmt\":" << captureInfos[i].format_
            << ",\"enc\":" << captureInfos[i].encodeType_
            << ",\"type\":" << captureInfos[i].type_
            << ",\"capture\":" << (captureInfos[i].isCapture_ ? 1 : 0) << "}";
    }
    oss << "}";
    return oss.str();
}
