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

#include "mock_device_manager.h"
#include "distributed_hardware_log.h"
#include <cstring>

namespace OHOS {
namespace DistributedHardware {

std::shared_ptr<MockDeviceManager> g_mockDeviceManager = nullptr;

std::shared_ptr<MockDeviceManager> MockDeviceManager::GetInstance() {
    if (g_mockDeviceManager == nullptr) {
        g_mockDeviceManager = std::make_shared<MockDeviceManager>();
    }
    return g_mockDeviceManager;
}

int32_t MockDeviceManager::GetTrustedDeviceList(const std::string &pkgName, const std::string &extra,
    std::vector<DmDeviceInfo> &deviceList) {
    std::lock_guard<std::mutex> lock(deviceLock_);

    // For local testing, return our mock devices
    deviceList.clear();
    for (const auto& device : mockDevices_) {
        deviceList.push_back(device);
    }

    DHLOGI("MockDeviceManager: Returning %{public}zu mock devices", mockDevices_.size());
    return 0;
}

int32_t MockDeviceManager::InitDeviceManager(const std::string &pkgName,
    std::shared_ptr<DmInitCallback> dmInitCallback) {
    DHLOGI("MockDeviceManager: InitDeviceManager called for package %{public}s", pkgName.c_str());
    // Mock initialization always succeeds
    return 0;
}

int32_t MockDeviceManager::GetUdidByNetworkId(const std::string &pkgName, const std::string &networkId,
    std::string &udid) {
    // For local testing, map network ID to a mock UDID
    udid = "LOCAL_TEST_UDID_" + networkId;
    DHLOGI("MockDeviceManager: Mapped network ID %{public}s to UDID %{public}s",
           networkId.c_str(), udid.c_str());
    return 0;
}

bool MockDeviceManager::CheckSrcAccessControl(const DmAccessCaller &caller, const DmAccessCallee &callee) {
    DHLOGI("MockDeviceManager: Access control check - returning %{public}s",
           accessControlResult_ ? "true" : "false");
    return accessControlResult_;
}

void MockDeviceManager::AddMockDevice(const DmDeviceInfo& device) {
    std::lock_guard<std::mutex> lock(deviceLock_);
    mockDevices_.push_back(device);
    DHLOGI("MockDeviceManager: Added mock device with networkId %{public}s",
           device.networkId.c_str());
}

void MockDeviceManager::ClearMockDevices() {
    std::lock_guard<std::mutex> lock(deviceLock_);
    mockDevices_.clear();
    DHLOGI("MockDeviceManager: Cleared all mock devices");
}

void MockDeviceManager::SetAccessControlResult(bool result) {
    accessControlResult_ = result;
    DHLOGI("MockDeviceManager: Set access control result to %{public}s",
           result ? "true" : "false");
}

} // namespace DistributedHardware
} // namespace OHOS