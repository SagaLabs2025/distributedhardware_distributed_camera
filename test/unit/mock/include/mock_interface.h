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

#ifndef OHOS_MOCK_INTERFACE_H
#define OHOS_MOCK_INTERFACE_H

#include <string>
#include <vector>
#include <memory>

namespace OHOS {
namespace DistributedHardware {

// Device Manager Mock Interface
struct DmDeviceInfo {
    std::string networkId;
    std::string udid;
    std::string name;
    int32_t deviceType;
    int32_t deviceTypeId;
};

class IDeviceManager {
public:
    virtual ~IDeviceManager() = default;
    virtual int32_t GetTrustedDeviceList(const std::string &pkgName, const std::string &extra,
        std::vector<DmDeviceInfo> &deviceList) = 0;
    virtual int32_t InitDeviceManager(const std::string &pkgName, void* dmInitCallback) = 0;
    virtual int32_t GetUdidByNetworkId(const std::string &pkgName, const std::string &networkId,
        std::string &udid) = 0;
    virtual bool CheckSrcAccessControl(void* caller, void* callee) = 0;
};

// HDF Device Manager Mock Interface
struct CameraInfo {
    std::string cameraId;
    int32_t width;
    int32_t height;
    int32_t fps;
    std::string format;
};

class IHdfDeviceManager {
public:
    virtual ~IHdfDeviceManager() = default;
    virtual int32_t LoadDCameraHDF(const std::string& dhId) = 0;
    virtual int32_t UnloadDCameraHDF(const std::string& dhId) = 0;
    virtual int32_t GetCameraIds(std::vector<std::string>& cameraIds) = 0;
    virtual int32_t GetCameraInfo(const std::string& cameraId, CameraInfo& cameraInfo) = 0;
    virtual int32_t SetCallback(const std::string& cameraId, void* callback) = 0;
};

// Camera Framework Mock Interface
class ICameraFramework {
public:
    virtual ~ICameraFramework() = default;
    virtual void* GetCameraService() = 0;
    virtual int32_t CreateCameraDevice(const std::string& cameraId, void*& device) = 0;
    virtual std::vector<std::string> GetCameraIds() = 0;
};

// System Service Mock Interface
class ISystemService {
public:
    virtual ~ISystemService() = default;
    virtual void LogInfo(const std::string& tag, const std::string& message) = 0;
    virtual void LogError(const std::string& tag, const std::string& message) = 0;
    virtual void LogDebug(const std::string& tag, const std::string& message) = 0;
    virtual void LogWarn(const std::string& tag, const std::string& message) = 0;
};

// Data Buffer Interface
class IDataBuffer {
public:
    virtual ~IDataBuffer() = default;
    virtual uint8_t* Data() = 0;
    virtual size_t Size() const = 0;
};

} // namespace DistributedHardware
} // namespace OHOS

#endif // OHOS_MOCK_INTERFACE_H