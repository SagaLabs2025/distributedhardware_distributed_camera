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

#include "mock_module_manager.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace OHOS::DistributedHardware;

int main() {
    std::cout << "=== Distributed Camera Mock Framework Test ===" << std::endl;

    // Initialize mock environment
    auto& mockManager = MockModuleManager::GetInstance();
    mockManager.InitializeMockEnvironment();

    // Test device manager
    auto deviceManager = mockManager.GetDeviceManager();
    std::vector<DmDeviceInfo> devices;
    int32_t result = deviceManager->GetTrustedDeviceList("test", "", devices);
    std::cout << "Device manager test: " << (result == 0 ? "PASSED" : "FAILED")
              << ", found " << devices.size() << " devices" << std::endl;

    // Test HDF device manager
    auto hdfManager = mockManager.GetHdfDeviceManager();
    std::vector<std::string> cameraIds;
    result = hdfManager->GetCameraIds(cameraIds);
    std::cout << "HDF manager test: " << (result == 0 ? "PASSED" : "FAILED")
              << ", found " << cameraIds.size() << " cameras" << std::endl;

    // Test camera info
    if (!cameraIds.empty()) {
        CameraInfo cameraInfo;
        result = hdfManager->GetCameraInfo(cameraIds[0], cameraInfo);
        std::cout << "Camera info test: " << (result == 0 ? "PASSED" : "FAILED")
                  << ", camera: " << cameraInfo.cameraId
                  << ", resolution: " << cameraInfo.width << "x" << cameraInfo.height
                  << ", fps: " << cameraInfo.fps << std::endl;
    }

    // Test system service logging
    auto systemService = mockManager.GetSystemService();
    systemService->LogInfo("TEST", "Mock framework test completed successfully!");

    // Keep running for a moment to see logs
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Cleanup
    mockManager.CleanupMockEnvironment();

    std::cout << "=== Test completed ===" << std::endl;
    return 0;
}