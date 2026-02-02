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

#include "dcamera_tcp_adapter.h"
#include "data_buffer.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace OHOS::DistributedHardware;

class TestListener : public ICameraChannelListener {
public:
    void OnBytes(int32_t socket, const void* data, uint32_t dataLen) override {
        std::cout << "Received bytes: " << dataLen << " bytes on socket " << socket << std::endl;
    }

    void OnStream(int32_t socket, const void* data, uint32_t dataLen) override {
        std::cout << "Received stream: " << dataLen << " bytes on socket " << socket << std::endl;
    }
};

int main() {
    std::cout << "Testing DCamera TCP Adapter..." << std::endl;

    // Create adapter instance
    DCameraTcpAdapter adapter;
    TestListener listener;

    // Register listener
    // Note: In real usage, this would be done through proper channel setup

    // Test network ID
    std::string networkId;
    int32_t result = adapter.GetLocalNetworkId(networkId);
    std::cout << "GetLocalNetworkId result: " << result << ", Network ID: " << networkId << std::endl;

    std::cout << "TCP adapter test completed." << std::endl;
    return 0;
}