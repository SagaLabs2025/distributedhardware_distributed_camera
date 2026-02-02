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

#include "socket_communication_adapter.h"
#include <cstring>
#include <thread>
#include <chrono>
#include <iostream>

#ifdef _WIN32
    #define CLOSE_SOCKET closesocket
#else
    #define CLOSE_SOCKET close
#endif

namespace OHOS {
namespace DistributedHardware {

SocketCommunicationAdapter::SocketCommunicationAdapter() : isInitialized_(false), shouldStop_(false) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        isInitialized_ = false;
        return;
    }
#endif
    isInitialized_ = true;
    localNetworkId_ = GetLocalIpAddress();
}

SocketCommunicationAdapter::~SocketCommunicationAdapter() {
    shouldStop_ = true;

    // Close all sessions
    std::lock_guard<std::mutex> lock(sessionsMutex_);
    for (auto& sessionPair : sessions_) {
        CLOSE_SOCKET(sessionPair.first);
        if (sessionPair.second->receiveThreads_.joinable()) {
            sessionPair.second->receiveThreads_.join();
        }
    }
    sessions_.clear();
    sessionNameToSocket_.clear();

#ifdef _WIN32
    WSACleanup();
#endif
}

int32_t SocketCommunicationAdapter::CreateServer(const std::string& sessionName,
                                              SessionMode mode,
                                              const std::string& peerDevId,
                                              const std::string& peerSessionName) {
    if (!isInitialized_) {
        return -1;
    }

    int32_t port = GetPortForSessionMode(mode);
    int32_t serverSocket = CreateTcpServer(defaultHost_, port);
    if (serverSocket < 0) {
        return -1;
    }

    auto session = std::make_shared<SocketSession>();
    session->socketId = serverSocket;
    session->sessionName = sessionName;
    session->mode = mode;
    session->peerDevId = peerDevId;
    session->peerSessionName = peerSessionName;
    session->isServer = true;
    session->isActive = true;

    {
        std::lock_guard<std::mutex> lock(sessionsMutex_);
        sessions_[serverSocket] = session;
        sessionNameToSocket_[sessionName] = serverSocket;
    }

    // Start accepting connections
    acceptThread_ = std::thread(&SocketCommunicationAdapter::AcceptConnections, this, serverSocket);

    return serverSocket;
}

int32_t SocketCommunicationAdapter::CreateClient(const std::string& myDhId,
                                              const std::string& myDevId,
                                              const std::string& peerSessionName,
                                              const std::string& peerDevId,
                                              SessionMode mode) {
    if (!isInitialized_) {
        return -1;
    }

    int32_t port = GetPortForSessionMode(mode);
    int32_t clientSocket = CreateTcpClient(defaultHost_, port);
    if (clientSocket < 0) {
        return -1;
    }

    auto session = std::make_shared<SocketSession>();
    session->socketId = clientSocket;
    session->sessionName = myDhId;
    session->mode = mode;
    session->peerDevId = peerDevId;
    session->peerSessionName = peerSessionName;
    session->isServer = false;
    session->isActive = true;

    {
        std::lock_guard<std::mutex> lock(sessionsMutex_);
        sessions_[clientSocket] = session;
        sessionNameToSocket_[myDhId] = clientSocket;
    }

    // Start receiving data
    receiveThreads_[clientSocket] = std::thread(&SocketCommunicationAdapter::ReceiveDataLoop, this, clientSocket);

    // Notify bind callback
    PeerInfo peerInfo;
    peerInfo.deviceId = peerDevId;
    peerInfo.sessionName = peerSessionName;
    peerInfo.socketId = clientSocket;
    OnBind(clientSocket, peerInfo);

    return clientSocket;
}

int32_t SocketCommunicationAdapter::DestroyServer(const std::string& sessionName) {
    std::lock_guard<std::mutex> lock(sessionsMutex_);
    auto it = sessionNameToSocket_.find(sessionName);
    if (it == sessionNameToSocket_.end()) {
        return -1;
    }

    int32_t socketId = it->second;
    auto sessionIt = sessions_.find(socketId);
    if (sessionIt != sessions_.end()) {
        sessionIt->second->isActive = false;
        CLOSE_SOCKET(socketId);
        sessions_.erase(sessionIt);
    }

    sessionNameToSocket_.erase(it);
    return 0;
}

int32_t SocketCommunicationAdapter::CloseSession(int32_t socketId) {
    std::lock_guard<std::mutex> lock(sessionsMutex_);
    auto it = sessions_.find(socketId);
    if (it == sessions_.end()) {
        return -1;
    }

    it->second->isActive = false;
    CLOSE_SOCKET(socketId);
    sessions_.erase(it);

    // Remove from sessionNameToSocket_ mapping
    for (auto nameIt = sessionNameToSocket_.begin(); nameIt != sessionNameToSocket_.end();) {
        if (nameIt->second == socketId) {
            nameIt = sessionNameToSocket_.erase(nameIt);
        } else {
            ++nameIt;
        }
    }

    return 0;
}

int32_t SocketCommunicationAdapter::SendBytes(int32_t socketId,
                                           std::shared_ptr<IDataBuffer> buffer) {
    if (!buffer || !buffer->IsValid()) {
        return -1;
    }

    return SendData(socketId, buffer->ConstData(), buffer->Size());
}

int32_t SocketCommunicationAdapter::SendStream(int32_t socketId,
                                            std::shared_ptr<IDataBuffer> buffer) {
    if (!buffer || !buffer->IsValid()) {
        return -1;
    }

    return SendData(socketId, buffer->ConstData(), buffer->Size());
}

int32_t SocketCommunicationAdapter::GetLocalNetworkId(std::string& myDevId) {
    myDevId = localNetworkId_;
    return 0;
}

void SocketCommunicationAdapter::OnBind(int32_t socketId, const PeerInfo& peerInfo) {
    if (onBindCallback_) {
        onBindCallback_(socketId, peerInfo);
    }
}

void SocketCommunicationAdapter::OnShutDown(int32_t socketId, int32_t reason) {
    if (onShutDownCallback_) {
        onShutDownCallback_(socketId, reason);
    }
}

void SocketCommunicationAdapter::OnBytesReceived(int32_t socketId,
                                              const uint8_t* data,
                                              uint32_t dataLen) {
    if (onBytesReceivedCallback_) {
        onBytesReceivedCallback_(socketId, data, dataLen);
    }
}

void SocketCommunicationAdapter::OnMessageReceived(int32_t socketId,
                                                const uint8_t* data,
                                                uint32_t dataLen) {
    if (onMessageReceivedCallback_) {
        onMessageReceivedCallback_(socketId, data, dataLen);
    }
}

void SocketCommunicationAdapter::OnStreamReceived(int32_t socketId,
                                               const uint8_t* data,
                                               uint32_t dataLen,
                                               const uint8_t* extData,
                                               uint32_t extLen) {
    if (onStreamReceivedCallback_) {
        onStreamReceivedCallback_(socketId, data, dataLen, extData, extLen);
    }
}

int32_t SocketCommunicationAdapter::InitializeWinsock() {
#ifdef _WIN32
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData);
#else
    return 0;
#endif
}

int32_t SocketCommunicationAdapter::CreateTcpServer(const std::string& host, int32_t port) {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        return -1;
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(host.c_str());
    serverAddr.sin_port = htons(port);

    // Allow address reuse
#ifdef _WIN32
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        CLOSE_SOCKET(serverSocket);
        return -1;
    }

    if (listen(serverSocket, 5) < 0) {
        CLOSE_SOCKET(serverSocket);
        return -1;
    }

    return serverSocket;
}

int32_t SocketCommunicationAdapter::CreateTcpClient(const std::string& host, int32_t port) {
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        return -1;
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(host.c_str());
    serverAddr.sin_port = htons(port);

    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        CLOSE_SOCKET(clientSocket);
        return -1;
    }

    return clientSocket;
}

void SocketCommunicationAdapter::AcceptConnections(int32_t serverSocket) {
    while (!shouldStop_) {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        int32_t clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);

        if (clientSocket < 0) {
            if (!shouldStop_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            continue;
        }

        // Find the server session to get session info
        std::shared_ptr<SocketSession> serverSession;
        {
            std::lock_guard<std::mutex> lock(sessionsMutex_);
            auto it = sessions_.find(serverSocket);
            if (it != sessions_.end()) {
                serverSession = it->second;
            }
        }

        if (serverSession) {
            auto clientSession = std::make_shared<SocketSession>();
            clientSession->socketId = clientSocket;
            clientSession->sessionName = serverSession->sessionName + "_client";
            clientSession->mode = serverSession->mode;
            clientSession->peerDevId = serverSession->peerDevId;
            clientSession->peerSessionName = serverSession->peerSessionName;
            clientSession->isServer = false;
            clientSession->isActive = true;

            {
                std::lock_guard<std::mutex> lock(sessionsMutex_);
                sessions_[clientSocket] = clientSession;
                sessionNameToSocket_[clientSession->sessionName] = clientSocket;
            }

            // Start receiving data for client
            receiveThreads_[clientSocket] = std::thread(&SocketCommunicationAdapter::ReceiveDataLoop, this, clientSocket);

            // Notify bind callback
            PeerInfo peerInfo;
            peerInfo.deviceId = serverSession->peerDevId;
            peerInfo.sessionName = serverSession->peerSessionName;
            peerInfo.socketId = clientSocket;
            OnBind(clientSocket, peerInfo);
        }
    }
}

void SocketCommunicationAdapter::HandleClientConnection(int32_t clientSocket) {
    // This method can be extended for more complex client handling
    // For now, we just start the receive loop
    ReceiveDataLoop(clientSocket);
}

void SocketCommunicationAdapter::ReceiveDataLoop(int32_t socketId) {
    const size_t BUFFER_SIZE = 65536; // 64KB buffer
    std::vector<uint8_t> buffer(BUFFER_SIZE);

    while (!shouldStop_) {
        std::shared_ptr<SocketSession> session;
        {
            std::lock_guard<std::mutex> lock(sessionsMutex_);
            auto it = sessions_.find(socketId);
            if (it == sessions_.end() || !it->second->isActive) {
                break;
            }
            session = it->second;
        }

        if (!session) {
            break;
        }

        int32_t bytesRead = ReceiveData(socketId, buffer.data(), BUFFER_SIZE);
        if (bytesRead <= 0) {
            // Connection closed or error
            OnShutDown(socketId, bytesRead);
            break;
        }

        ParseReceivedData(socketId, buffer.data(), bytesRead);
    }
}

int32_t SocketCommunicationAdapter::SendData(int32_t socketId, const uint8_t* data, size_t size) {
    if (!data || size == 0) {
        return -1;
    }

    std::lock_guard<std::mutex> lock(sessionsMutex_);
    auto it = sessions_.find(socketId);
    if (it == sessions_.end() || !it->second->isActive) {
        return -1;
    }

#ifdef _WIN32
    int sent = send(socketId, reinterpret_cast<const char*>(data), static_cast<int>(size), 0);
#else
    ssize_t sent = send(socketId, data, size, 0);
#endif

    return (sent == static_cast<int32_t>(size)) ? 0 : -1;
}

int32_t SocketCommunicationAdapter::ReceiveData(int32_t socketId, uint8_t* buffer, size_t maxSize) {
    if (!buffer || maxSize == 0) {
        return -1;
    }

#ifdef _WIN32
    int received = recv(socketId, reinterpret_cast<char*>(buffer), static_cast<int>(maxSize), 0);
#else
    ssize_t received = recv(socketId, buffer, maxSize, 0);
#endif

    return (received > 0) ? received : -1;
}

void SocketCommunicationAdapter::ParseReceivedData(int32_t socketId, const uint8_t* data, uint32_t dataLen) {
    // Simple protocol: first byte indicates message type
    // 0x01: Bytes message
    // 0x02: Stream message
    // 0x03: Regular message

    if (dataLen == 0) {
        return;
    }

    uint8_t messageType = data[0];
    const uint8_t* payload = data + 1;
    uint32_t payloadLen = dataLen - 1;

    switch (messageType) {
        case 0x01: // Bytes
            OnBytesReceived(socketId, payload, payloadLen);
            break;
        case 0x02: // Stream
            // For stream, we assume no extended data for simplicity
            OnStreamReceived(socketId, payload, payloadLen, nullptr, 0);
            break;
        case 0x03: // Message
            OnMessageReceived(socketId, payload, payloadLen);
            break;
        default:
            // Unknown message type, treat as bytes
            OnBytesReceived(socketId, data, dataLen);
            break;
    }
}

std::string SocketCommunicationAdapter::GenerateSessionName(SessionMode mode) {
    std::string baseName = "dcamera_session_";
    switch (mode) {
        case SessionMode::CONTROL_SESSION:
            return baseName + "control";
        case SessionMode::DATA_CONTINUE_SESSION:
            return baseName + "data_continue";
        case SessionMode::DATA_SNAPSHOT_SESSION:
            return baseName + "data_snapshot";
        default:
            return baseName + "unknown";
    }
}

int32_t SocketCommunicationAdapter::GetPortForSessionMode(SessionMode mode) {
    auto it = sessionPorts_.find(mode);
    if (it != sessionPorts_.end()) {
        return it->second;
    }
    return 8080; // Default port
}

std::string SocketCommunicationAdapter::GetLocalIpAddress() {
    // Simple implementation - return localhost for testing
    // In real implementation, you would get the actual IP address
    return "127.0.0.1";
}

} // namespace DistributedHardware
} // namespace OHOS