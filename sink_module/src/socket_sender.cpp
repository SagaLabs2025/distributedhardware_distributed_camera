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

#include "socket_sender.h"
#include "distributed_hardware_log.h"

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

SocketSender::SocketSender()
    : sockfd_(INVALID_SOCKET), connected_(false), initialized_(false) {
}

SocketSender::~SocketSender() {
    Close();
}

int32_t SocketSender::Initialize() {
    DHLOGI("[SOCKET_SENDER] Initialize");

#ifdef _WIN32
    // Windows: 初始化 Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        DHLOGE("[SOCKET_SENDER] WSAStartup failed");
        return -1;
    }
#endif

    initialized_= true;
    DHLOGI("[SOCKET_SENDER] Initialize success");
    return 0;
}

int32_t SocketSender::ConnectToSource(const std::string& host, int port) {
    DHLOGI("[SOCKET_SENDER] Connecting to %{public}s:%{public}d", host.c_str(), port);

    if (!initialized_) {
        DHLOGE("[SOCKET_SENDER] Not initialized");
        return -1;
    }

    // 创建 socket
    sockfd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd_ == INVALID_SOCKET) {
        DHLOGE("[SOCKET_SENDER] socket creation failed");
        return -1;
    }

    // 连接到服务器
    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr) <= 0) {
        DHLOGE("[SOCKET_SENDER] Invalid address");
        closesocket(sockfd_);
        return -1;
    }

    if (connect(sockfd_, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        DHLOGE("[SOCKET_SENDER] Connection failed");
        closesocket(sockfd_);
        return -1;
    }

    connected_= true;
    DHLOGI("[SOCKET_SENDER] Connected successfully");
    return 0;
}

int32_t SocketSender::SendData(const std::vector<uint8_t>& data) {
    if (!connected_) {
        DHLOGE("[SOCKET_SENDER] Not connected");
        return -1;
    }

    return SendDataInternal(data.data(), static_cast<int>(data.size()));
}

int32_t SocketSender::SendControlMessage(const std::string& message) {
    if (!connected_) {
        DHLOGE("[SOCKET_SENDER] Not connected");
        return -1;
    }

    // 发送消息长度
    uint32_t msgLen = static_cast<uint32_t>(message.size());
    int32_t ret = SendDataInternal(&msgLen, sizeof(msgLen));
    if (ret < 0) {
        return ret;
    }

    // 发送消息内容
    return SendDataInternal(message.data(), static_cast<int>(message.size()));
}

int32_t SocketSender::SendDataInternal(const void* data, int size) {
    int totalSent = 0;
    const char* ptr = static_cast<const char*>(data);

    while (totalSent < size) {
        int sent = send(sockfd_, ptr + totalSent, size - totalSent, 0);
        if (sent == SOCKET_ERROR) {
            DHLOGE("[SOCKET_SENDER] Send failed");
            return -1;
        }
        totalSent += sent;
    }

    return 0;
}

void SocketSender::Close() {
    if (sockfd_ != INVALID_SOCKET) {
        closesocket(sockfd_);
        sockfd_ = INVALID_SOCKET;
    }
    connected_= false;
    initialized_= false;

#ifdef _WIN32
    WSACleanup();
#endif

    DHLOGI("[SOCKET_SENDER] Closed");
}
