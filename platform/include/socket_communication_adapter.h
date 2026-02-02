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

#ifndef OHOS_SOCKET_COMMUNICATION_ADAPTER_H
#define OHOS_SOCKET_COMMUNICATION_ADAPTER_H

#include "platform_interface.h"
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <map>
#include <atomic>
#include <condition_variable>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
#endif

namespace OHOS {
namespace DistributedHardware {

class SocketCommunicationAdapter : public ICommunicationAdapter {
public:
    SocketCommunicationAdapter();
    ~SocketCommunicationAdapter() override;

    // Server operations
    int32_t CreateServer(const std::string& sessionName,
                        SessionMode mode,
                        const std::string& peerDevId,
                        const std::string& peerSessionName) override;

    // Client operations
    int32_t CreateClient(const std::string& myDhId,
                        const std::string& myDevId,
                        const std::string& peerSessionName,
                        const std::string& peerDevId,
                        SessionMode mode) override;

    // Session management
    int32_t DestroyServer(const std::string& sessionName) override;
    int32_t CloseSession(int32_t socketId) override;

    // Data transmission
    int32_t SendBytes(int32_t socketId,
                     std::shared_ptr<IDataBuffer> buffer) override;
    int32_t SendStream(int32_t socketId,
                      std::shared_ptr<IDataBuffer> buffer) override;

    // Network information
    int32_t GetLocalNetworkId(std::string& myDevId) override;

    // Event callbacks
    void OnBind(int32_t socketId, const PeerInfo& peerInfo) override;
    void OnShutDown(int32_t socketId, int32_t reason) override;
    void OnBytesReceived(int32_t socketId,
                       const uint8_t* data,
                       uint32_t dataLen) override;
    void OnMessageReceived(int32_t socketId,
                         const uint8_t* data,
                         uint32_t dataLen) override;
    void OnStreamReceived(int32_t socketId,
                        const uint8_t* data,
                        uint32_t dataLen,
                        const uint8_t* extData,
                        uint32_t extLen) override;

private:
    struct SocketSession {
        int32_t socketId;
        std::string sessionName;
        SessionMode mode;
        std::string peerDevId;
        std::string peerSessionName;
        bool isServer;
        std::atomic<bool> isActive;
    };

    int32_t InitializeWinsock();
    int32_t CreateTcpServer(const std::string& host, int32_t port);
    int32_t CreateTcpClient(const std::string& host, int32_t port);
    void AcceptConnections(int32_t serverSocket);
    void HandleClientConnection(int32_t clientSocket);
    void ReceiveDataLoop(int32_t socketId);
    int32_t SendData(int32_t socketId, const uint8_t* data, size_t size);
    int32_t ReceiveData(int32_t socketId, uint8_t* buffer, size_t maxSize);
    void ParseReceivedData(int32_t socketId, const uint8_t* data, uint32_t dataLen);

    std::string GenerateSessionName(SessionMode mode);
    int32_t GetPortForSessionMode(SessionMode mode);
    std::string GetLocalIpAddress();

    std::map<int32_t, std::shared_ptr<SocketSession>> sessions_;
    std::map<std::string, int32_t> sessionNameToSocket_;
    std::mutex sessionsMutex_;

    std::atomic<bool> isInitialized_;
    std::atomic<bool> shouldStop_;

    std::thread acceptThread_;
    std::map<int32_t, std::thread> receiveThreads_;

    std::string localNetworkId_;
    std::string defaultHost_ = "127.0.0.1";
    std::map<SessionMode, int32_t> sessionPorts_ = {
        {SessionMode::CONTROL_SESSION, 8080},
        {SessionMode::DATA_CONTINUE_SESSION, 8081},
        {SessionMode::DATA_SNAPSHOT_SESSION, 8082}
    };

    // Callbacks
    std::function<void(int32_t, const PeerInfo&)> onBindCallback_;
    std::function<void(int32_t, int32_t)> onShutDownCallback_;
    std::function<void(int32_t, const uint8_t*, uint32_t)> onBytesReceivedCallback_;
    std::function<void(int32_t, const uint8_t*, uint32_t)> onMessageReceivedCallback_;
    std::function<void(int32_t, const uint8_t*, uint32_t, const uint8_t*, uint32_t)> onStreamReceivedCallback_;
};

} // namespace DistributedHardware
} // namespace OHOS

#endif // OHOS_SOCKET_COMMUNICATION_ADAPTER_H