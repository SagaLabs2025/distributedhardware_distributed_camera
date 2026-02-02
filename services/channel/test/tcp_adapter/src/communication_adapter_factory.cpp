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

#include "communication_adapter_factory.h"
#include "local_tcp_adapter.h"
#include <cstdlib>
#include <string>

namespace OHOS {
namespace DistributedHardware {

std::unique_ptr<ICommunicationAdapter> CommunicationAdapterFactory::CreateAdapter(CommunicationMode mode) {
    switch (mode) {
        case COMM_MODE_TCP:
            return std::make_unique<LocalTcpAdapter>();
        default:
            // Default to TCP for Windows local testing
            return std::make_unique<LocalTcpAdapter>();
    }
}

std::unique_ptr<ICommunicationAdapter> CommunicationAdapterFactory::CreateAdapterFromConfig() {
    CommunicationMode mode = GetModeFromEnvironment();
    return CreateAdapter(mode);
}

CommunicationMode CommunicationAdapterFactory::GetModeFromEnvironment() {
    const char* modeStr = std::getenv("DCAMERA_COMM_MODE");
    if (modeStr != nullptr) {
        std::string modeString(modeStr);
        if (modeString == "tcp") {
            return COMM_MODE_TCP;
        }
    }
    // Default to TCP
    return COMM_MODE_TCP;
}

} // namespace DistributedHardware
} // namespace OHOS

#endif // DCAMERA_TEST_ENABLE
