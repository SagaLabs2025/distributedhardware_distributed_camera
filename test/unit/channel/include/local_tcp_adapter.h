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

#ifndef OHOS_LOCAL_TCP_ADAPTER_H
#define OHOS_LOCAL_TCP_ADAPTER_H

#include "i_communication_adapter.h"
#include <winsock2.h>
#include <windows.h>
#include <thread>
#include <mutex>
#include <map>
#include <string>
#include <memory>

namespace OHOS {
namespace DistributedHardware {

class ICameraChannelListener;
class DCameraSoftbusSession;

class LocalTcpAdapter : public ICommunicationAdapter {
public:
    LocalTcpAdapter();
    ~LocalTcpAdapter();

    // Server creation for Sink role
    int32_t CreateSinkSocketServer(std::string mySessionName, DCAMERA_CHANNEL_ROLE role,
        DCameraSessionMode sessionMode, std::string peerDevId, std::string peerSessionName) override;

    // Client creation for Source role
    int32_t CreateSourceSocketClient(std::string myDhId, std::string myDevId, std::string peerSessionName,
        std::string peerDevId, DCameraSessionMode sessionMode, DCAMERA_CHANNEL_ROLE role) override;

    int32_t DestroySessionServer(std::string sessionName) override;
    int32_t CloseSession(int32_t socket) override;
    int32_t SendBytes(int32_t socket, std::shared_ptr<DataBuffer> &buffer) override;
    int32_t SendStream(int32_t socket, std::shared_ptr<DataBuffer> &buffer) override;
    int32_t GetLocalNetworkId(std::string &myDevId) override;

    // Callback registration
    void RegisterSourceListener(ICameraChannelListener* listener) override;
    void RegisterSinkListener(ICameraChannelListener* listener) override;

private:
    // TCP server thread function
    void ServerThread(std::string sessionName, uint16_t port);

    // TCP client connection function
    int32_t ConnectToServer(std::string host, uint16_t port);

    // Data receiving thread
    void ReceiveThread(int32_t socket, DCAMERA_CHANNEL_ROLE role);

    // Helper functions
    uint16_t GetAvailablePort();
    std::string GenerateSessionKey(const std::string& sessionName, const std::string& peerDevId);

    // Callback dispatchers
    void DispatchSourceCallbacks(int32_t socket, const char* data, uint32_t dataLen, bool isStream = false);
    void DispatchSinkCallbacks(int32_t socket, const char* data, uint32_t dataLen, bool isStream = false);

private:
    WSADATA wsaData_;
    bool isInitialized_;
    std::mutex adapterLock_;

    // Session management
    std::map<std::string, int32_t> serverSockets_; // sessionName -> socket
    std::map<int32_t, std::thread> serverThreads_;
    std::map<int32_t, std::thread> receiveThreads_;
    std::map<int32_t, DCAMERA_CHANNEL_ROLE> socketRoles_;

    // Listeners
    ICameraChannelListener* sourceListener_;
    ICameraChannelListener* sinkListener_;

    // Port management
    uint16_t basePort_;
    std::set<uint16_t> usedPorts_;
};

} // namespace DistributedHardware
} // namespace OHOS

#endif // OHOS_LOCAL_TCP_ADAPTER_H

#endif // DCAMERA_TEST_ENABLE
