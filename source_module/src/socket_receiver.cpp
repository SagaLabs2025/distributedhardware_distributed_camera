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

#include "socket_receiver.h"
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

SocketReceiver::SocketReceiver()
    : listenSockfd_(INVALID_SOCKET), connSockfd_(INVALID_SOCKET),
      receiving_(false), initialized_(false), serverStarted_(false),
      callback_(nullptr) {
}

SocketReceiver::~SocketReceiver() {
    StopReceiving();
}

int32_t SocketReceiver::Initialize(ISourceCallback* callback) {
    DHLOGI("[SOCKET_RECEIVER] Initialize");

    callback_ = callback;

#ifdef _WIN32
    // Windows: 初始化 Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        DHLOGE("[SOCKET_RECEIVER] WSAStartup failed");
        return -1;
    }
#endif

    initialized_ = true;
    DHLOGI("[SOCKET_RECEIVER] Initialize success");
    return 0;
}

int32_t SocketReceiver::StartReceiving() {
    DHLOGI("[SOCKET_RECEIVER] StartReceiving");

    if (!initialized_) {
        DHLOGE("[SOCKET_RECEIVER] Not initialized");
        return -1;
    }

    if (receiving_) {
        DHLOGW("[SOCKET_RECEIVER] Already receiving");
        return 0;
    }

    // 启动服务器
    if (StartServer() != 0) {
        DHLOGE("[SOCKET_RECEIVER] Failed to start server");
        return -1;
    }

    receiving_ = true;
    receiveThread_ = std::thread(&SocketReceiver::ReceiveThreadProc, this);

    DHLOGI("[SOCKET_RECEIVER] StartReceiving success");
    return 0;
}

void SocketReceiver::StopReceiving() {
    DHLOGI("[SOCKET_RECEIVER] StopReceiving");

    receiving_ = false;
    StopServer();

    if (receiveThread_.joinable()) {
        receiveThread_.join();
    }

    DHLOGI("[SOCKET_RECEIVER] StopReceiving done");
}

int32_t SocketReceiver::StartServer() {
    DHLOGI("[SOCKET_RECEIVER] Starting server on port %{public}d", DEFAULT_PORT);

    // 创建 socket
    listenSockfd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSockfd_ == INVALID_SOCKET) {
        DHLOGE("[SOCKET_RECEIVER] socket creation failed");
        return -1;
    }

    // 设置 SO_REUSEADDR
    int opt = 1;
    setsockopt(listenSockfd_, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    // 绑定
    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(DEFAULT_PORT);

    if (bind(listenSockfd_, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        DHLOGE("[SOCKET_RECEIVER] bind failed");
        closesocket(listenSockfd_);
        return -1;
    }

    // 监听
    if (listen(listenSockfd_, 1) == SOCKET_ERROR) {
        DHLOGE("[SOCKET_RECEIVER] listen failed");
        closesocket(listenSockfd_);
        return -1;
    }

    serverStarted_ = true;
    DHLOGI("[SOCKET_RECEIVER] Server started, waiting for connection...");
    return 0;
}

void SocketReceiver::StopServer() {
    if (connSockfd_ != INVALID_SOCKET) {
        closesocket(connSockfd_);
        connSockfd_ = INVALID_SOCKET;
    }

    if (listenSockfd_ != INVALID_SOCKET) {
        closesocket(listenSockfd_);
        listenSockfd_ = INVALID_SOCKET;
    }

    serverStarted_ = false;

#ifdef _WIN32
    WSACleanup();
#endif
}

void SocketReceiver::ReceiveThreadProc() {
    DHLOGI("[SOCKET_RECEIVER] Receive thread started");

    // 等待客户端连接
    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    connSockfd_ = accept(listenSockfd_, (sockaddr*)&clientAddr, &clientLen);

    if (connSockfd_ == INVALID_SOCKET) {
        DHLOGE("[SOCKET_RECEIVER] accept failed");
        return;
    }

    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
    DHLOGI("[SOCKET_RECEIVER] Client connected: %{public}s", clientIP);

    // 接收数据循环
    while (receiving_) {
        if (ReceiveData() != 0) {
            break;
        }
    }

    DHLOGI("[SOCKET_RECEIVER] Receive thread ended");
}

int32_t SocketReceiver::ReceiveData() {
    // 首先接收数据长度
    uint32_t dataLen = 0;
    int received = recv(connSockfd_, (char*)&dataLen, sizeof(dataLen), 0);

    if (received <= 0) {
        if (received == 0) {
            DHLOGI("[SOCKET_RECEIVER] Client disconnected");
        } else {
            DHLOGE("[SOCKET_RECEIVER] recv failed");
        }
        return -1;
    }

    // 接收数据内容
    std::vector<uint8_t> data(dataLen);
    int totalReceived = 0;

    while (totalReceived < static_cast<int>(dataLen)) {
        int ret = recv(connSockfd_, (char*)(data.data() + totalReceived),
                      dataLen - totalReceived, 0);
        if (ret <= 0) {
            DHLOGE("[SOCKET_RECEIVER] recv data failed");
            return -1;
        }
        totalReceived += ret;
    }

    DHLOGI("[SOCKET_RECEIVER] Received %{public}u bytes", dataLen);

    // 通知回调 (这里简化处理，直接传递编码数据)
    // 实际应该解码后传递YUV数据
    if (callback_) {
        // 暂时直接传递编码数据（实际应用中需要解码）
        // callback_->OnDecodedFrameAvailable(data.data(), 1920, 1080);
    }

    return 0;
}
