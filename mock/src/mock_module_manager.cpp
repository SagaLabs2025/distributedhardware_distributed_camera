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

namespace OHOS {
namespace DistributedHardware {

MockModuleManager& MockModuleManager::GetInstance() {
    static MockModuleManager instance;
    return instance;
}

std::shared_ptr<MockDeviceManager> MockModuleManager::GetDeviceManager() {
    if (!deviceManager_) {
        deviceManager_ = std::make_shared<MockDeviceManager>();
    }
    return deviceManager_;
}

std::shared_ptr<MockHdfDeviceManager> MockModuleManager::GetHdfDeviceManager() {
    if (!hdfDeviceManager_) {
        hdfDeviceManager_ = std::make_shared<MockHdfDeviceManager>();
    }
    return hdfDeviceManager_;
}

std::shared_ptr<MockCameraFramework> MockModuleManager::GetCameraFramework() {
    if (!cameraFramework_) {
        cameraFramework_ = std::make_shared<MockCameraFramework>();
    }
    return cameraFramework_;
}

std::shared_ptr<MockSystemService> MockModuleManager::GetSystemService() {
    if (!systemService_) {
        systemService_ = std::make_shared<MockSystemService>();
    }
    return systemService_;
}

void MockModuleManager::InitializeMockEnvironment() {
    std::cout << "Initializing mock environment..." << std::endl;

    // Initialize all mock services
    GetDeviceManager();
    GetHdfDeviceManager();
    GetCameraFramework();
    GetSystemService();

    // Configure default devices and cameras
    ConfigureDefaultDevices();
    ConfigureDefaultCameras();

    std::cout << "Mock environment initialized successfully!" << std::endl;
}

void MockModuleManager::CleanupMockEnvironment() {
    std::cout << "Cleaning up mock environment..." << std::endl;

    if (deviceManager_) {
        deviceManager_->ClearMockDevices();
        deviceManager_.reset();
    }

    if (hdfDeviceManager_) {
        hdfDeviceManager_->ClearMockCameras();
        hdfDeviceManager_.reset();
    }

    if (cameraFramework_) {
        cameraFramework_->ClearMockCameras();
        cameraFramework_.reset();
    }

    systemService_.reset();

    std::cout << "Mock environment cleaned up successfully!" << std::endl;
}

void MockModuleManager::ConfigureDefaultDevices() {
    auto deviceManager = GetDeviceManager();

    // Add source device
    DmDeviceInfo sourceDevice;
    sourceDevice.networkId = "LOCAL_SOURCE_DEVICE";
    sourceDevice.udid = "LOCAL_SOURCE_UDID";
    sourceDevice.name = "Local Source Camera";
    sourceDevice.deviceType = 1; // CAMERA
    sourceDevice.deviceTypeId = 10001;
    deviceManager->AddMockDevice(sourceDevice);

    // Add sink device
    DmDeviceInfo sinkDevice;
    sinkDevice.networkId = "LOCAL_SINK_DEVICE";
    sinkDevice.udid = "LOCAL_SINK_UDID";
    sinkDevice.name = "Local Sink Display";
    sinkDevice.deviceType = 2; // DISPLAY
    sinkDevice.deviceTypeId = 20001;
    deviceManager->AddMockDevice(sinkDevice);

    std::cout << "Configured default devices: source and sink" << std::endl;
}

void MockModuleManager::ConfigureDefaultCameras() {
    auto hdfManager = GetHdfDeviceManager();
    auto cameraFramework = GetCameraFramework();

    // Add mock camera
    CameraInfo cameraInfo;
    cameraInfo.cameraId = "MOCK_CAMERA_001";
    cameraInfo.width = 1280;
    cameraInfo.height = 720;
    cameraInfo.fps = 30;
    cameraInfo.format = "H264";

    hdfManager->AddMockCamera("MOCK_CAMERA_001", cameraInfo);
    cameraFramework->AddMockCamera("MOCK_CAMERA_001");

    std::cout << "Configured default camera: MOCK_CAMERA_001 (1280x720@30fps)" << std::endl;
}

} // namespace DistributedHardware
} // namespace OHOS