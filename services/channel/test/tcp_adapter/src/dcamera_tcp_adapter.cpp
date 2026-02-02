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

#include "dcamera_tcp_adapter.h"
#include "communication_adapter_factory.h"
#include "local_tcp_adapter.h"
#include "distributed_hardware_log.h"

namespace OHOS {
namespace DistributedHardware {

DCameraTcpAdapter::DCameraTcpAdapter() {
    // Create TCP adapter based on configuration
    tcpAdapter_ = CommunicationAdapterFactory::CreateAdapterFromConfig();
    if (!tcpAdapter_) {
        DHLOGE("Failed to create TCP adapter");
        tcpAdapter_ = std::make_unique<LocalTcpAdapter>();
    }
}

DCameraTcpAdapter::~DCameraTcpAdapter() {
    // Cleanup handled by unique_ptr
}

int32_t DCameraTcpAdapter::CreatSoftBusSinkSocketServer(std::string mySessionName, DCAMERA_CHANNEL_ROLE role,
    DCameraSessionMode sessionMode, std::string peerDevId, std::string peerSessionName) {
    if (!tcpAdapter_) {
        return -1;
    }
    return tcpAdapter_->CreateSinkSocketServer(mySessionName, role, sessionMode, peerDevId, peerSessionName);
}

int32_t DCameraTcpAdapter::CreateSoftBusSourceSocketClient(std::string myDhId, std::string myDevId,
    std::string peerSessionName, std::string peerDevId, DCameraSessionMode sessionMode, DCAMERA_CHANNEL_ROLE role) {
    if (!tcpAdapter_) {
        return -1;
    }
    return tcpAdapter_->CreateSourceSocketClient(myDhId, myDevId, peerSessionName, peerDevId, sessionMode, role);
}

int32_t DCameraTcpAdapter::DestroySoftbusSessionServer(std::string sessionName) {
    if (!tcpAdapter_) {
        return -1;
    }
    return tcpAdapter_->DestroySessionServer(sessionName);
}

int32_t DCameraTcpAdapter::CloseSoftbusSession(int32_t socket) {
    if (!tcpAdapter_) {
        return -1;
    }
    return tcpAdapter_->CloseSession(socket);
}

int32_t DCameraTcpAdapter::SendSofbusBytes(int32_t socket, std::shared_ptr<DataBuffer> &buffer) {
    if (!tcpAdapter_) {
        return -1;
    }
    return tcpAdapter_->SendBytes(socket, buffer);
}

int32_t DCameraTcpAdapter::SendSofbusStream(int32_t socket, std::shared_ptr<DataBuffer> &buffer) {
    if (!tcpAdapter_) {
        return -1;
    }
    return tcpAdapter_->SendStream(socket, buffer);
}

int32_t DCameraTcpAdapter::GetLocalNetworkId(std::string &myDevId) {
    if (!tcpAdapter_) {
        myDevId = "LOCAL_TEST_DEVICE";
        return 0;
    }
    return tcpAdapter_->GetLocalNetworkId(myDevId);
}

} // namespace DistributedHardware
} // namespace OHOS

#endif // DCAMERA_TEST_ENABLE
