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

#ifdef DCAMERA_TEST_ENABLE

#include "local_tcp_adapter.h"
#include "i_communication_adapter.h"
#include "data_buffer.h"
#include "icamera_channel.h"
#include "dhlog.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <sstream>
#include <set>

namespace OHOS {
namespace DistributedHardware {

LocalTcpAdapter::LocalTcpAdapter()
    : isInitialized_(false), sourceListener_(nullptr), sinkListener_(nullptr), basePort_(50000) {
    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData_) != 0) {
        DHLOGE("Failed to initialize Winsock");
        return;
    }
    isInitialized_ = true;
}

LocalTcpAdapter::~LocalTcpAdapter() {
    if (isInitialized_) {
        // Clean up all sockets
        for (auto& server : serverSockets_) {
            closesocket(server.second);
        }
        WSACleanup();
    }
}

int32_t LocalTcpAdapter::CreateSinkSocketServer(std::string mySessionName, DCAMERA_CHANNEL_ROLE role,
    DCameraSessionMode sessionMode, std::string peerDevId, std::string peerSessionName) {
    if (!isInitialized_) {
        DHLOGE("Winsock not initialized");
        return -1;
    }

    std::lock_guard<std::mutex> lock(adapterLock_);

    // Get available port
    uint16_t port = GetAvailablePort();
    if (port == 0) {
        DHLOGE("No available ports");
        return -1;
    }

    // Create server socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        DHLOGE("Failed to create server socket");
        return -1;
    }

    // Bind to localhost and port
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        DHLOGE("Failed to bind server socket");
        closesocket(serverSocket);
        return -1;
    }

    // Listen for connections
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        DHLOGE("Failed to listen on server socket");
        closesocket(serverSocket);
        return -1;
    }

    // Store server socket
    std::string sessionKey = GenerateSessionKey(mySessionName, peerDevId);
    serverSockets_[sessionKey] = serverSocket;

    // Start server thread
    serverThreads_[serverSocket] = std::thread(&LocalTcpAdapter::ServerThread, this, mySessionName, port);

    DHLOGI("Created TCP server for session %{public}s on port %{public}d",
           mySessionName.c_str(), port);

    return static_cast<int32_t>(serverSocket);
}

int32_t LocalTcpAdapter::CreateSourceSocketClient(std::string myDhId, std::string myDevId,
    std::string peerSessionName, std::string peerDevId, DCameraSessionMode sessionMode,
    DCAMERA_CHANNEL_ROLE role) {
    if (!isInitialized_) {
        DHLOGE("Winsock not initialized");
        return -1;
    }

    std::lock_guard<std::mutex> lock(adapterLock_);

    // For local testing, we assume the server is running on localhost
    // In a real scenario, this would need to be configured or discovered
    std::string host = "127.0.0.1";
    uint16_t port = 50000; // Default starting port, should be configurable

    int32_t clientSocket = ConnectToServer(host, port);
    if (clientSocket == -1) {
        DHLOGE("Failed to connect to server");
        return -1;
    }

    // Store socket role
    socketRoles_[clientSocket] = role;

    // Start receive thread
    receiveThreads_[clientSocket] = std::thread(&LocalTcpAdapter::ReceiveThread, this, clientSocket, role);

    DHLOGI("Connected to TCP server for session %{public}s", peerSessionName.c_str());

    return clientSocket;
}

void LocalTcpAdapter::ServerThread(std::string sessionName, uint16_t port) {
    std::string sessionKey = sessionName; // Simplified for local testing

    auto it = serverSockets_.find(sessionKey);
    if (it == serverSockets_.end()) {
        return;
    }

    SOCKET serverSocket = it->second;

    while (true) {
        sockaddr_in clientAddr;
        int addrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &addrLen);

        if (clientSocket == INVALID_SOCKET) {
            break;
        }

        DHLOGI("Accepted client connection for session %{public}s", sessionName.c_str());

        // Determine role based on session name or other criteria
        DCAMERA_CHANNEL_ROLE role = DCAMERA_CHANNLE_ROLE_SINK; // Default for server

        // Start receive thread for client
        std::lock_guard<std::mutex> lock(adapterLock_);
        socketRoles_[static_cast<int32_t>(clientSocket)] = role;
        receiveThreads_[static_cast<int32_t>(clientSocket)] =
            std::thread(&LocalTcpAdapter::ReceiveThread, this, static_cast<int32_t>(clientSocket), role);
    }
}

int32_t LocalTcpAdapter::ConnectToServer(std::string host, uint16_t port) {
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        DHLOGE("Failed to create client socket");
        return -1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    // Convert host to IP address
    inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        DHLOGE("Failed to connect to server at %{public}s:%{public}d", host.c_str(), port);
        closesocket(clientSocket);
        return -1;
    }

    return static_cast<int32_t>(clientSocket);
}

void LocalTcpAdapter::ReceiveThread(int32_t socket, DCAMERA_CHANNEL_ROLE role) {
    const int BUFFER_SIZE = 65536;
    char* buffer = new char[BUFFER_SIZE];

    while (true) {
        int bytesReceived = recv(socket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0) {
            // Connection closed or error
            break;
        }

        // Dispatch to appropriate callbacks based on role
        if (role == DCAMERA_CHANNLE_ROLE_SOURCE) {
            DispatchSourceCallbacks(socket, buffer, bytesReceived);
        } else {
            DispatchSinkCallbacks(socket, buffer, bytesReceived);
        }
    }

    delete[] buffer;

    // Clean up socket
    closesocket(socket);

    DHLOGI("Receive thread ended for socket %{public}d", socket);
}

uint16_t LocalTcpAdapter::GetAvailablePort() {
    // Simple port allocation strategy
    // In production, this should check if port is actually available
    for (uint16_t port = basePort_; port < basePort_ + 100; port++) {
        if (usedPorts_.find(port) == usedPorts_.end()) {
            usedPorts_.insert(port);
            return port;
        }
    }
    return 0; // No available ports
}

std::string LocalTcpAdapter::GenerateSessionKey(const std::string& sessionName, const std::string& peerDevId) {
    return sessionName + "_" + peerDevId;
}

int32_t LocalTcpAdapter::DestroySessionServer(std::string sessionName) {
    std::lock_guard<std::mutex> lock(adapterLock_);

    auto it = serverSockets_.find(sessionName);
    if (it != serverSockets_.end()) {
        closesocket(it->second);
        serverSockets_.erase(it);

        // Remove from used ports (simplified)
        // In reality, we'd need to track which port was used for this session
    }

    return 0;
}

int32_t LocalTcpAdapter::CloseSession(int32_t socket) {
    closesocket(socket);

    // Clean up threads
    auto threadIt = receiveThreads_.find(socket);
    if (threadIt != receiveThreads_.end()) {
        // Note: This doesn't actually stop the thread gracefully
        // In production, you'd need proper thread termination
        threadIt->second.detach();
        receiveThreads_.erase(threadIt);
    }

    socketRoles_.erase(socket);

    return 0;
}

int32_t LocalTcpAdapter::SendBytes(int32_t socket, std::shared_ptr<DataBuffer> &buffer) {
    if (!buffer || buffer->Size() == 0) {
        return -1;
    }

    int bytesSent = send(socket, reinterpret_cast<const char*>(buffer->Data()),
                        static_cast<int>(buffer->Size()), 0);

    if (bytesSent == SOCKET_ERROR) {
        DHLOGE("Failed to send bytes: error %{public}d", WSAGetLastError());
        return -1;
    }

    return bytesSent;
}

int32_t LocalTcpAdapter::SendStream(int32_t socket, std::shared_ptr<DataBuffer> &buffer) {
    // Stream sending is similar to bytes for TCP
    return SendBytes(socket, buffer);
}

int32_t LocalTcpAdapter::GetLocalNetworkId(std::string &myDevId) {
    // For local testing, return a dummy network ID
    myDevId = "LOCAL_TEST_DEVICE_001";
    return 0;
}

void LocalTcpAdapter::RegisterSourceListener(ICameraChannelListener* listener) {
    sourceListener_ = listener;
}

void LocalTcpAdapter::RegisterSinkListener(ICameraChannelListener* listener) {
    sinkListener_ = listener;
}

void LocalTcpAdapter::DispatchSourceCallbacks(int32_t socket, const char* data, uint32_t dataLen, bool isStream) {
    if (sourceListener_) {
        if (isStream) {
            // Handle stream callback
            // This would need to parse the stream data according to the protocol
            sourceListener_->OnStream(socket, data, dataLen);
        } else {
            // Handle message/bytes callback
            sourceListener_->OnBytes(socket, data, dataLen);
        }
    }
}

void LocalTcpAdapter::DispatchSinkCallbacks(int32_t socket, const char* data, uint32_t dataLen, bool isStream) {
    if (sinkListener_) {
        if (isStream) {
            sinkListener_->OnStream(socket, data, dataLen);
        } else {
            sinkListener_->OnBytes(socket, data, dataLen);
        }
    }
}

} // namespace DistributedHardware
} // namespace OHOS

#endif // DCAMERA_TEST_ENABLE
