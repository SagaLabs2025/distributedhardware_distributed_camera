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

#include "mock_hdf_device_manager.h"
#include "distributed_hardware_log.h"
#include <thread>
#include <chrono>

namespace OHOS {
namespace DistributedHardware {

std::shared_ptr<MockHdfDeviceManager> g_mockHdfDeviceManager = nullptr;

std::shared_ptr<MockHdfDeviceManager> MockHdfDeviceManager::GetInstance() {
    if (g_mockHdfDeviceManager == nullptr) {
        g_mockHdfDeviceManager = std::make_shared<MockHdfDeviceManager>();
    }
    return g_mockHdfDeviceManager;
}

int32_t MockHdfDeviceManager::LoadDCameraHDF(const std::string& dhId) {
    std::lock_guard<std::mutex> lock(hdfLock_);
    DHLOGI("MockHdfDeviceManager: Loading HDF for device %{public}s", dhId.c_str());

    // Simulate loading time
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    return loadResult_;
}

int32_t MockHdfDeviceManager::UnloadDCameraHDF(const std::string& dhId) {
    std::lock_guard<std::mutex> lock(hdfLock_);
    DHLOGI("MockHdfDeviceManager: Unloading HDF for device %{public}s", dhId.c_str());
    return 0;
}

int32_t MockHdfDeviceManager::GetCameraIds(std::vector<std::string>& cameraIds) {
    std::lock_guard<std::mutex> lock(hdfLock_);
    cameraIds.clear();
    for (const auto& camera : mockCameras_) {
        cameraIds.push_back(camera.cameraId);
    }
    DHLOGI("MockHdfDeviceManager: Returning %{public}zu camera IDs", mockCameras_.size());
    return 0;
}

int32_t MockHdfDeviceManager::GetCameraInfo(const std::string& cameraId, CameraInfo& cameraInfo) {
    std::lock_guard<std::mutex> lock(hdfLock_);

    for (const auto& camera : mockCameras_) {
        if (camera.cameraId == cameraId) {
            cameraInfo = camera.info;
            DHLOGI("MockHdfDeviceManager: Found camera info for %{public}s", cameraId.c_str());
            return 0;
        }
    }

    DHLOGE("MockHdfDeviceManager: Camera ID %{public}s not found", cameraId.c_str());
    return -1;
}

int32_t MockHdfDeviceManager::SetCallback(const std::string& cameraId,
    const std::shared_ptr<ICameraCallback>& callback) {
    std::lock_guard<std::mutex> lock(hdfLock_);
    mockCallback_ = callback;
    DHLOGI("MockHdfDeviceManager: Set callback for camera %{public}s", cameraId.c_str());
    return 0;
}

void MockHdfDeviceManager::AddMockCamera(const std::string& cameraId, const CameraInfo& cameraInfo) {
    std::lock_guard<std::mutex> lock(hdfLock_);
    MockCameraInfo mockInfo;
    mockInfo.cameraId = cameraId;
    mockInfo.info = cameraInfo;
    mockCameras_.push_back(mockInfo);
    DHLOGI("MockHdfDeviceManager: Added mock camera %{public}s", cameraId.c_str());
}

void MockHdfDeviceManager::ClearMockCameras() {
    std::lock_guard<std::mutex> lock(hdfLock_);
    mockCameras_.clear();
    DHLOGI("MockHdfDeviceManager: Cleared all mock cameras");
}

void MockHdfDeviceManager::SetLoadResult(int32_t result) {
    loadResult_ = result;
    DHLOGI("MockHdfDeviceManager: Set load result to %{public}d", result);
}

void MockHdfDeviceManager::SetCallback(std::shared_ptr<ICameraCallback> callback) {
    std::lock_guard<std::mutex> lock(hdfLock_);
    mockCallback_ = callback;
}

} // namespace DistributedHardware
} // namespace OHOS