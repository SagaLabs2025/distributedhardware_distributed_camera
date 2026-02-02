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

#ifndef OHOS_MOCK_HDF_DEVICE_MANAGER_H
#define OHOS_MOCK_HDF_DEVICE_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include "dcamera_hdf_operate.h"

namespace OHOS {
namespace DistributedHardware {

class MockHdfDeviceManager {
public:
    static std::shared_ptr<MockHdfDeviceManager> GetInstance();

    // Mock implementation of HDF device manager methods
    int32_t LoadDCameraHDF(const std::string& dhId);
    int32_t UnloadDCameraHDF(const std::string& dhId);
    int32_t GetCameraIds(std::vector<std::string>& cameraIds);
    int32_t GetCameraInfo(const std::string& cameraId, CameraInfo& cameraInfo);
    int32_t SetCallback(const std::string& cameraId, const std::shared_ptr<ICameraCallback>& callback);

    // Configuration methods for testing
    void AddMockCamera(const std::string& cameraId, const CameraInfo& cameraInfo);
    void ClearMockCameras();
    void SetLoadResult(int32_t result);
    void SetCallback(std::shared_ptr<ICameraCallback> callback);

private:
    MockHdfDeviceManager() = default;
    ~MockHdfDeviceManager() = default;

    struct MockCameraInfo {
        std::string cameraId;
        CameraInfo info;
    };

    std::vector<MockCameraInfo> mockCameras_;
    int32_t loadResult_ = 0; // Default success
    std::shared_ptr<ICameraCallback> mockCallback_;
    std::mutex hdfLock_;
};

// Global instance for easy access
extern std::shared_ptr<MockHdfDeviceManager> g_mockHdfDeviceManager;

} // namespace DistributedHardware
} // namespace OHOS

#endif // OHOS_MOCK_HDF_DEVICE_MANAGER_H