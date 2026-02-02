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

#include "mock_platform_interfaces.h"
#include <algorithm>
#include <cstring>

namespace OHOS {
namespace DistributedHardware {

MockDeviceManagerImpl::MockDeviceManagerImpl() : initResult_(0) {}

int32_t MockDeviceManagerImpl::GetTrustedDeviceList(const std::string& pkgName,
                                                  const std::string& extra,
                                                  std::vector<DeviceInfo>& deviceList) {
    std::lock_guard<std::mutex> lock(deviceLock_);
    deviceList = mockDevices_;
    return 0; // Success
}

int32_t MockDeviceManagerImpl::InitDeviceManager(const std::string& pkgName,
                                               void* dmInitCallback) {
    return initResult_;
}

int32_t MockDeviceManagerImpl::GetUdidByNetworkId(const std::string& pkgName,
                                                 const std::string& networkId,
                                                 std::string& udid) {
    std::lock_guard<std::mutex> lock(deviceLock_);
    for (const auto& device : mockDevices_) {
        if (device.networkId == networkId) {
            udid = device.udid;
            return 0; // Success
        }
    }
    return -1; // Not found
}

bool MockDeviceManagerImpl::CheckSrcAccessControl(void* caller, void* callee) {
    return accessControlResult_;
}

void MockDeviceManagerImpl::AddMockDevice(const DeviceInfo& device) {
    std::lock_guard<std::mutex> lock(deviceLock_);
    mockDevices_.push_back(device);
}

void MockDeviceManagerImpl::ClearMockDevices() {
    std::lock_guard<std::mutex> lock(deviceLock_);
    mockDevices_.clear();
}

void MockDeviceManagerImpl::SetAccessControlResult(bool result) {
    accessControlResult_ = result;
}

void MockDeviceManagerImpl::SetInitResult(int32_t result) {
    initResult_ = result;
}

// MockHdfDeviceManagerImpl implementation
MockHdfDeviceManagerImpl::MockHdfDeviceManagerImpl() : operationResult_(0) {}

int32_t MockHdfDeviceManagerImpl::LoadDCameraHDF(const std::string& dhId) {
    return operationResult_;
}

int32_t MockHdfDeviceManagerImpl::UnloadDCameraHDF(const std::string& dhId) {
    return operationResult_;
}

int32_t MockHdfDeviceManagerImpl::GetCameraIds(std::vector<std::string>& cameraIds) {
    std::lock_guard<std::mutex> lock(hdfLock_);
    cameraIds = mockCameraIds_;
    return 0; // Success
}

int32_t MockHdfDeviceManagerImpl::GetCameraCapabilities(const std::string& cameraId,
                                                      CameraCapability& capabilities) {
    std::lock_guard<std::mutex> lock(hdfLock_);
    for (const auto& mockCamera : mockCameras_) {
        if (mockCamera.cameraId == cameraId) {
            capabilities = mockCamera.capabilities;
            return 0; // Success
        }
    }
    return -1; // Not found
}

int32_t MockHdfDeviceManagerImpl::OpenSession(const std::string& dhId) {
    return operationResult_;
}

int32_t MockHdfDeviceManagerImpl::CloseSession(const std::string& dhId) {
    return operationResult_;
}

int32_t MockHdfDeviceManagerImpl::ConfigureStreams(const std::string& dhId,
                                                 const std::vector<StreamConfig>& streamConfigs) {
    return operationResult_;
}

int32_t MockHdfDeviceManagerImpl::ReleaseStreams(const std::string& dhId,
                                               const std::vector<int32_t>& streamIds) {
    return operationResult_;
}

int32_t MockHdfDeviceManagerImpl::StartCapture(const std::string& dhId,
                                             const std::vector<CaptureConfig>& captureConfigs) {
    return operationResult_;
}

int32_t MockHdfDeviceManagerImpl::StopCapture(const std::string& dhId,
                                            const std::vector<int32_t>& streamIds) {
    return operationResult_;
}

int32_t MockHdfDeviceManagerImpl::UpdateSettings(const std::string& dhId,
                                               const std::vector<uint8_t>& settings) {
    return operationResult_;
}

int32_t MockHdfDeviceManagerImpl::NotifyEvent(const std::string& dhId,
                                            const std::string& eventType,
                                            const std::vector<uint8_t>& eventData) {
    return operationResult_;
}

void MockHdfDeviceManagerImpl::AddMockCamera(const std::string& cameraId,
                                           const CameraCapability& capabilities) {
    std::lock_guard<std::mutex> lock(hdfLock_);
    MockCameraInfo info;
    info.cameraId = cameraId;
    info.capabilities = capabilities;
    mockCameras_.push_back(info);

    // Also add to cameraIds list if not already present
    if (std::find(mockCameraIds_.begin(), mockCameraIds_.end(), cameraId) == mockCameraIds_.end()) {
        mockCameraIds_.push_back(cameraId);
    }
}

void MockHdfDeviceManagerImpl::ClearMockCameras() {
    std::lock_guard<std::mutex> lock(hdfLock_);
    mockCameras_.clear();
    mockCameraIds_.clear();
}

void MockHdfDeviceManagerImpl::SetOperationResult(int32_t result) {
    operationResult_ = result;
}

void MockHdfDeviceManagerImpl::SetCameraIds(const std::vector<std::string>& cameraIds) {
    std::lock_guard<std::mutex> lock(hdfLock_);
    mockCameraIds_ = cameraIds;
}

// MockDataBuffer implementation
MockDataBuffer::MockDataBuffer(size_t size) {
    if (size > 0) {
        buffer_.resize(size);
        isValid_ = true;
    } else {
        isValid_ = false;
    }
}

MockDataBuffer::MockDataBuffer(const uint8_t* data, size_t size) {
    if (data != nullptr && size > 0) {
        buffer_.assign(data, data + size);
        isValid_ = true;
    } else {
        isValid_ = false;
    }
}

uint8_t* MockDataBuffer::Data() {
    return isValid_ ? buffer_.data() : nullptr;
}

const uint8_t* MockDataBuffer::ConstData() const {
    return isValid_ ? buffer_.data() : nullptr;
}

size_t MockDataBuffer::Size() const {
    return isValid_ ? buffer_.size() : 0;
}

void MockDataBuffer::Resize(size_t newSize) {
    if (newSize == 0) {
        buffer_.clear();
        isValid_ = false;
    } else {
        buffer_.resize(newSize);
        isValid_ = true;
    }
}

bool MockDataBuffer::IsValid() const {
    return isValid_;
}

void MockDataBuffer::FillWithPattern(uint8_t pattern) {
    if (isValid_) {
        std::fill(buffer_.begin(), buffer_.end(), pattern);
    }
}

void MockDataBuffer::CopyFrom(const uint8_t* data, size_t size) {
    if (data != nullptr && size > 0) {
        buffer_.assign(data, data + size);
        isValid_ = true;
    } else {
        isValid_ = false;
    }
}

} // namespace DistributedHardware
} // namespace OHOS