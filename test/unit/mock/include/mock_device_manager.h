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

#ifndef OHOS_MOCK_DEVICE_MANAGER_H
#define OHOS_MOCK_DEVICE_MANAGER_H

#include <vector>
#include <string>
#include <memory>
#include "device_manager.h"
#include "mock_manager.h"

namespace OHOS {
namespace DistributedHardware {

class MockDeviceManager {
public:
    static std::shared_ptr<MockDeviceManager> GetInstance();

    // Mock implementation of DeviceManager methods
    int32_t GetTrustedDeviceList(const std::string &pkgName, const std::string &extra,
        std::vector<DmDeviceInfo> &deviceList);

    int32_t InitDeviceManager(const std::string &pkgName, std::shared_ptr<DmInitCallback> dmInitCallback);

    int32_t GetUdidByNetworkId(const std::string &pkgName, const std::string &networkId,
        std::string &udid);

    bool CheckSrcAccessControl(const DmAccessCaller &caller, const DmAccessCallee &callee);

    // Configuration methods for testing
    void AddMockDevice(const DmDeviceInfo& device);
    void ClearMockDevices();
    void SetAccessControlResult(bool result);

private:
    MockDeviceManager() = default;
    ~MockDeviceManager() = default;

    std::vector<DmDeviceInfo> mockDevices_;
    bool accessControlResult_ = true;
    std::mutex deviceLock_;
};

// Global instance for easy access
extern std::shared_ptr<MockDeviceManager> g_mockDeviceManager;

} // namespace DistributedHardware
} // namespace OHOS

#endif // OHOS_MOCK_DEVICE_MANAGER_H