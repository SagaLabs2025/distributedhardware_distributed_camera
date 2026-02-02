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

#ifndef OHOS_MOCK_PLATFORM_INTERFACES_H
#define OHOS_MOCK_PLATFORM_INTERFACES_H

#include "platform_interface.h"
#include <string>
#include <vector>
#include <memory>
#include <mutex>

namespace OHOS {
namespace DistributedHardware {

// Mock implementation of platform interfaces for testing

class MockDeviceManagerImpl : public IDeviceManager {
public:
    MockDeviceManagerImpl();
    ~MockDeviceManagerImpl() override = default;

    int32_t GetTrustedDeviceList(const std::string& pkgName,
                                const std::string& extra,
                                std::vector<DeviceInfo>& deviceList) override;

    int32_t InitDeviceManager(const std::string& pkgName,
                             void* dmInitCallback) override;

    int32_t GetUdidByNetworkId(const std::string& pkgName,
                              const std::string& networkId,
                              std::string& udid) override;

    bool CheckSrcAccessControl(void* caller, void* callee) override;

    // Configuration methods for testing
    void AddMockDevice(const DeviceInfo& device);
    void ClearMockDevices();
    void SetAccessControlResult(bool result);
    void SetInitResult(int32_t result);

private:
    std::vector<DeviceInfo> mockDevices_;
    bool accessControlResult_ = true;
    int32_t initResult_ = 0; // Default success
    std::mutex deviceLock_;
};

class MockHdfDeviceManagerImpl : public IHdfDeviceManager {
public:
    MockHdfDeviceManagerImpl();
    ~MockHdfDeviceManagerImpl() override = default;

    int32_t LoadDCameraHDF(const std::string& dhId) override;
    int32_t UnloadDCameraHDF(const std::string& dhId) override;
    int32_t GetCameraIds(std::vector<std::string>& cameraIds) override;
    int32_t GetCameraCapabilities(const std::string& cameraId,
                                 CameraCapability& capabilities) override;
    int32_t OpenSession(const std::string& dhId) override;
    int32_t CloseSession(const std::string& dhId) override;
    int32_t ConfigureStreams(const std::string& dhId,
                           const std::vector<StreamConfig>& streamConfigs) override;
    int32_t ReleaseStreams(const std::string& dhId,
                          const std::vector<int32_t>& streamIds) override;
    int32_t StartCapture(const std::string& dhId,
                        const std::vector<CaptureConfig>& captureConfigs) override;
    int32_t StopCapture(const std::string& dhId,
                       const std::vector<int32_t>& streamIds) override;
    int32_t UpdateSettings(const std::string& dhId,
                          const std::vector<uint8_t>& settings) override;
    int32_t NotifyEvent(const std::string& dhId,
                       const std::string& eventType,
                       const std::vector<uint8_t>& eventData) override;

    // Configuration methods for testing
    void AddMockCamera(const std::string& cameraId, const CameraCapability& capabilities);
    void ClearMockCameras();
    void SetOperationResult(int32_t result);
    void SetCameraIds(const std::vector<std::string>& cameraIds);

private:
    struct MockCameraInfo {
        std::string cameraId;
        CameraCapability capabilities;
    };

    std::vector<MockCameraInfo> mockCameras_;
    std::vector<std::string> mockCameraIds_;
    int32_t operationResult_ = 0; // Default success
    std::mutex hdfLock_;
};

class MockDataBuffer : public IDataBuffer {
public:
    explicit MockDataBuffer(size_t size = 0);
    MockDataBuffer(const uint8_t* data, size_t size);
    ~MockDataBuffer() override = default;

    uint8_t* Data() override;
    const uint8_t* ConstData() const override;
    size_t Size() const override;
    void Resize(size_t newSize) override;
    bool IsValid() const override;

    void FillWithPattern(uint8_t pattern = 0);
    void CopyFrom(const uint8_t* data, size_t size);

private:
    std::vector<uint8_t> buffer_;
    bool isValid_ = true;
};

} // namespace DistributedHardware
} // namespace OHOS

#endif // OHOS_MOCK_PLATFORM_INTERFACES_H