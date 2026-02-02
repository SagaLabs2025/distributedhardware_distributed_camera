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

#ifndef OHOS_MOCK_CAMERA_FRAMEWORK_H
#define OHOS_MOCK_CAMERA_FRAMEWORK_H

#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include "icamera_service.h"
#include "icamera_device.h"

namespace OHOS {
namespace DistributedHardware {

class MockCameraFramework {
public:
    static std::shared_ptr<MockCameraFramework> GetInstance();

    // Mock implementation of Camera Framework methods
    sptr<ICameraService> GetCameraService();
    int32_t CreateCameraDevice(const std::string& cameraId, sptr<ICameraDevice>& device);
    std::vector<std::string> GetCameraIds();

    // Configuration methods for testing
    void AddMockCamera(const std::string& cameraId);
    void ClearMockCameras();
    void SetCreateDeviceResult(int32_t result);

private:
    MockCameraFramework() = default;
    ~MockCameraFramework() = default;

    std::vector<std::string> mockCameraIds_;
    int32_t createDeviceResult_ = 0;
    std::mutex frameworkLock_;
};

// Global instance for easy access
extern std::shared_ptr<MockCameraFramework> g_mockCameraFramework;

} // namespace DistributedHardware
} // namespace OHOS

#endif // OHOS_MOCK_CAMERA_FRAMEWORK_H