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

#ifndef OHOS_I_COMMUNICATION_ADAPTER_H
#define OHOS_I_COMMUNICATION_ADAPTER_H

#include <string>
#include <memory>
#include "data_buffer.h"

namespace OHOS {
namespace DistributedHardware {

class ICameraChannelListener;

typedef enum {
    DCAMERA_CHANNLE_ROLE_SOURCE = 0,
    DCAMERA_CHANNLE_ROLE_SINK = 1,
} DCAMERA_CHANNEL_ROLE;

typedef enum {
    DCAMERA_SESSION_MODE_BYTES = 0,
    DCAMERA_SESSION_MODE_STREAM = 1,
} DCameraSessionMode;

class ICommunicationAdapter {
public:
    virtual ~ICommunicationAdapter() = default;

    // Server creation for Sink role
    virtual int32_t CreateSinkSocketServer(std::string mySessionName, DCAMERA_CHANNEL_ROLE role,
        DCameraSessionMode sessionMode, std::string peerDevId, std::string peerSessionName) = 0;

    // Client creation for Source role
    virtual int32_t CreateSourceSocketClient(std::string myDhId, std::string myDevId, std::string peerSessionName,
        std::string peerDevId, DCameraSessionMode sessionMode, DCAMERA_CHANNEL_ROLE role) = 0;

    virtual int32_t DestroySessionServer(std::string sessionName) = 0;
    virtual int32_t CloseSession(int32_t socket) = 0;
    virtual int32_t SendBytes(int32_t socket, std::shared_ptr<DataBuffer> &buffer) = 0;
    virtual int32_t SendStream(int32_t socket, std::shared_ptr<DataBuffer> &buffer) = 0;
    virtual int32_t GetLocalNetworkId(std::string &myDevId) = 0;

    // Callback registration
    virtual void RegisterSourceListener(ICameraChannelListener* listener) = 0;
    virtual void RegisterSinkListener(ICameraChannelListener* listener) = 0;
};

} // namespace DistributedHardware
} // namespace OHOS

#endif // OHOS_I_COMMUNICATION_ADAPTER_H

#endif // DCAMERA_TEST_ENABLE
