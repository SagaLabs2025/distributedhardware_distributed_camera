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

#ifndef OHOS_MOCK_IMPLEMENTATION_H
#define OHOS_MOCK_IMPLEMENTATION_H

#include "mock_interface.h"
#include <mutex>
#include <vector>

namespace OHOS {
namespace DistributedHardware {

class MockDeviceManager : public IDeviceManager {
public:
    int32_t GetTrustedDeviceList(const std::string &pkgName, const std::string &extra,
        std::vector<DmDeviceInfo> &deviceList) override;
    int32_t InitDeviceManager(const std::string &pkgName, void* dmInitCallback) override;
    int32_t GetUdidByNetworkId(const std::string &pkgName, const std::string &networkId,
        std::string &udid) override;
    bool CheckSrcAccessControl(void* caller, void* callee) override;

    // Configuration methods
    void AddMockDevice(const DmDeviceInfo& device);
    void ClearMockDevices();
    void SetAccessControlResult(bool result);

private:
    std::vector<DmDeviceInfo> mockDevices_;
    bool accessControlResult_ = true;
    std::mutex deviceLock_;
};

class MockHdfDeviceManager : public IHdfDeviceManager {
public:
    int32_t LoadDCameraHDF(const std::string& dhId) override;
    int32_t UnloadDCameraHDF(const std::string& dhId) override;
    int32_t GetCameraIds(std::vector<std::string>& cameraIds) override;
    int32_t GetCameraInfo(const std::string& cameraId, CameraInfo& cameraInfo) override;
    int32_t SetCallback(const std::string& cameraId, void* callback) override;

    // Configuration methods
    void AddMockCamera(const std::string& cameraId, const CameraInfo& cameraInfo);
    void ClearMockCameras();
    void SetLoadResult(int32_t result);

private:
    struct MockCameraInfo {
        std::string cameraId;
        CameraInfo info;
    };

    std::vector<MockCameraInfo> mockCameras_;
    int32_t loadResult_ = 0;
    std::mutex hdfLock_;
};

class MockCameraFramework : public ICameraFramework {
public:
    void* GetCameraService() override;
    int32_t CreateCameraDevice(const std::string& cameraId, void*& device) override;
    std::vector<std::string> GetCameraIds() override;

    // Configuration methods
    void AddMockCamera(const std::string& cameraId);
    void ClearMockCameras();

private:
    std::vector<std::string> mockCameraIds_;
    std::mutex frameworkLock_;
};

class MockSystemService : public ISystemService {
public:
    void LogInfo(const std::string& tag, const std::string& message) override;
    void LogError(const std::string& tag, const std::string& message) override;
    void LogDebug(const std::string& tag, const std::string& message) override;
    void LogWarn(const std::string& tag, const std::string& message) override;

private:
    std::mutex logLock_;
};

} // namespace DistributedHardware
} // namespace OHOS

#endif // OHOS_MOCK_IMPLEMENTATION_H