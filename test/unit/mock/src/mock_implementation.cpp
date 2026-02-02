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

#include "mock_implementation.h"
#include <iostream>
#include <thread>
#include <chrono>

namespace OHOS {
namespace DistributedHardware {

// MockDeviceManager implementation
int32_t MockDeviceManager::GetTrustedDeviceList(const std::string &pkgName, const std::string &extra,
    std::vector<DmDeviceInfo> &deviceList) {
    std::lock_guard<std::mutex> lock(deviceLock_);
    deviceList = mockDevices_;
    std::cout << "[MOCK] GetTrustedDeviceList: returning " << mockDevices_.size() << " devices" << std::endl;
    return 0;
}

int32_t MockDeviceManager::InitDeviceManager(const std::string &pkgName, void* dmInitCallback) {
    std::cout << "[MOCK] InitDeviceManager for package: " << pkgName << std::endl;
    return 0;
}

int32_t MockDeviceManager::GetUdidByNetworkId(const std::string &pkgName, const std::string &networkId,
    std::string &udid) {
    udid = "LOCAL_TEST_UDID_" + networkId;
    std::cout << "[MOCK] GetUdidByNetworkId: " << networkId << " -> " << udid << std::endl;
    return 0;
}

bool MockDeviceManager::CheckSrcAccessControl(void* caller, void* callee) {
    std::cout << "[MOCK] CheckSrcAccessControl: returning " << (accessControlResult_ ? "true" : "false") << std::endl;
    return accessControlResult_;
}

void MockDeviceManager::AddMockDevice(const DmDeviceInfo& device) {
    std::lock_guard<std::mutex> lock(deviceLock_);
    mockDevices_.push_back(device);
    std::cout << "[MOCK] Added mock device: " << device.networkId << std::endl;
}

void MockDeviceManager::ClearMockDevices() {
    std::lock_guard<std::mutex> lock(deviceLock_);
    mockDevices_.clear();
    std::cout << "[MOCK] Cleared all mock devices" << std::endl;
}

void MockDeviceManager::SetAccessControlResult(bool result) {
    accessControlResult_ = result;
    std::cout << "[MOCK] Set access control result to: " << (result ? "true" : "false") << std::endl;
}

// MockHdfDeviceManager implementation
int32_t MockHdfDeviceManager::LoadDCameraHDF(const std::string& dhId) {
    std::lock_guard<std::mutex> lock(hdfLock_);
    std::cout << "[MOCK] Loading HDF for device: " << dhId << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return loadResult_;
}

int32_t MockHdfDeviceManager::UnloadDCameraHDF(const std::string& dhId) {
    std::lock_guard<std::mutex> lock(hdfLock_);
    std::cout << "[MOCK] Unloading HDF for device: " << dhId << std::endl;
    return 0;
}

int32_t MockHdfDeviceManager::GetCameraIds(std::vector<std::string>& cameraIds) {
    std::lock_guard<std::mutex> lock(hdfLock_);
    cameraIds.clear();
    for (const auto& camera : mockCameras_) {
        cameraIds.push_back(camera.cameraId);
    }
    std::cout << "[MOCK] GetCameraIds: returning " << mockCameras_.size() << " cameras" << std::endl;
    return 0;
}

int32_t MockHdfDeviceManager::GetCameraInfo(const std::string& cameraId, CameraInfo& cameraInfo) {
    std::lock_guard<std::mutex> lock(hdfLock_);
    for (const auto& camera : mockCameras_) {
        if (camera.cameraId == cameraId) {
            cameraInfo = camera.info;
            std::cout << "[MOCK] GetCameraInfo for: " << cameraId << std::endl;
            return 0;
        }
    }
    std::cout << "[MOCK] Camera ID not found: " << cameraId << std::endl;
    return -1;
}

int32_t MockHdfDeviceManager::SetCallback(const std::string& cameraId, void* callback) {
    std::lock_guard<std::mutex> lock(hdfLock_);
    std::cout << "[MOCK] SetCallback for camera: " << cameraId << std::endl;
    return 0;
}

void MockHdfDeviceManager::AddMockCamera(const std::string& cameraId, const CameraInfo& cameraInfo) {
    std::lock_guard<std::mutex> lock(hdfLock_);
    MockCameraInfo mockInfo;
    mockInfo.cameraId = cameraId;
    mockInfo.info = cameraInfo;
    mockCameras_.push_back(mockInfo);
    std::cout << "[MOCK] Added mock camera: " << cameraId << std::endl;
}

void MockHdfDeviceManager::ClearMockCameras() {
    std::lock_guard<std::mutex> lock(hdfLock_);
    mockCameras_.clear();
    std::cout << "[MOCK] Cleared all mock cameras" << std::endl;
}

void MockHdfDeviceManager::SetLoadResult(int32_t result) {
    loadResult_ = result;
    std::cout << "[MOCK] Set load result to: " << result << std::endl;
}

// MockCameraFramework implementation
void* MockCameraFramework::GetCameraService() {
    std::cout << "[MOCK] GetCameraService called" << std::endl;
    return nullptr;
}

int32_t MockCameraFramework::CreateCameraDevice(const std::string& cameraId, void*& device) {
    std::cout << "[MOCK] CreateCameraDevice for: " << cameraId << std::endl;
    device = nullptr;
    return 0;
}

std::vector<std::string> MockCameraFramework::GetCameraIds() {
    std::lock_guard<std::mutex> lock(frameworkLock_);
    std::cout << "[MOCK] GetCameraIds returning " << mockCameraIds_.size() << " cameras" << std::endl;
    return mockCameraIds_;
}

void MockCameraFramework::AddMockCamera(const std::string& cameraId) {
    std::lock_guard<std::mutex> lock(frameworkLock_);
    mockCameraIds_.push_back(cameraId);
    std::cout << "[MOCK] Added mock camera to framework: " << cameraId << std::endl;
}

void MockCameraFramework::ClearMockCameras() {
    std::lock_guard<std::mutex> lock(frameworkLock_);
    mockCameraIds_.clear();
    std::cout << "[MOCK] Cleared mock cameras from framework" << std::endl;
}

// MockSystemService implementation
void MockSystemService::LogInfo(const std::string& tag, const std::string& message) {
    std::cout << "[INFO][" << tag << "] " << message << std::endl;
}

void MockSystemService::LogError(const std::string& tag, const std::string& message) {
    std::cout << "[ERROR][" << tag << "] " << message << std::endl;
}

void MockSystemService::LogDebug(const std::string& tag, const std::string& message) {
    std::cout << "[DEBUG][" << tag << "] " << message << std::endl;
}

void MockSystemService::LogWarn(const std::string& tag, const std::string& message) {
    std::cout << "[WARN][" << tag << "] " << message << std::endl;
}

} // namespace DistributedHardware
} // namespace OHOS