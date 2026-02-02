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

#ifndef OHOS_DCAMERA_TCP_ADAPTER_H
#define OHOS_DCAMERA_TCP_ADAPTER_H

#include "dcamera_softbus_adapter.h"
#include "i_communication_adapter.h"
#include "communication_adapter_factory.h"
#include <memory>

namespace OHOS {
namespace DistributedHardware {

class DCameraTcpAdapter : public DCameraSoftbusAdapter {
public:
    DCameraTcpAdapter();
    ~DCameraTcpAdapter();

    // Override existing methods to use TCP adapter
    int32_t CreatSoftBusSinkSocketServer(std::string mySessionName, DCAMERA_CHANNEL_ROLE role,
        DCameraSessionMode sessionMode, std::string peerDevId, std::string peerSessionName) override;

    int32_t CreateSoftBusSourceSocketClient(std::string myDhId, std::string myDevId, std::string peerSessionName,
        std::string peerDevId, DCameraSessionMode sessionMode, DCAMERA_CHANNEL_ROLE role) override;

    int32_t DestroySoftbusSessionServer(std::string sessionName) override;
    int32_t CloseSoftbusSession(int32_t socket) override;
    int32_t SendSofbusBytes(int32_t socket, std::shared_ptr<DataBuffer> &buffer) override;
    int32_t SendSofbusStream(int32_t socket, std::shared_ptr<DataBuffer> &buffer) override;
    int32_t GetLocalNetworkId(std::string &myDevId) override;

private:
    std::unique_ptr<ICommunicationAdapter> tcpAdapter_;
};

} // namespace DistributedHardware
} // namespace OHOS

#endif // OHOS_DCAMERA_TCP_ADAPTER_H

#endif // DCAMERA_TEST_ENABLE
