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

#ifndef OHOS_PLATFORM_COMPATIBILITY_ADAPTER_H
#define OHOS_PLATFORM_COMPATIBILITY_ADAPTER_H

#include "platform_interface.h"
#include "icamera_channel.h"
#include "device_manager.h"
#include "dcamera_hdf_operate.h"
#include <memory>

namespace OHOS {
namespace DistributedHardware {

// Adapter to bridge platform interfaces with existing OpenHarmony interfaces

class PlatformDeviceManagerAdapter : public IDeviceManager {
public:
    PlatformDeviceManagerAdapter();
    ~PlatformDeviceManagerAdapter() override = default;

    int32_t GetTrustedDeviceList(const std::string& pkgName,
                               const std::string& extra,
                               std::vector<DeviceInfo>& deviceList) override;

    int32_t InitDeviceManager(const std::string& pkgName,
                            void* dmInitCallback) override;

    int32_t GetUdidByNetworkId(const std::string& pkgName,
                             const std::string& networkId,
                             std::string& udid) override;

    bool CheckSrcAccessControl(void* caller, void* callee) override;

private:
    // Use actual DeviceManager implementation
    std::shared_ptr<OHOS::DistributedHardware::DeviceManager> realDeviceManager_;
};

class PlatformHdfAdapter : public IHdfDeviceManager {
public:
    PlatformHdfAdapter();
    ~PlatformHdfAdapter() override = default;

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

private:
    std::shared_ptr<DCameraHdfOperate> hdfOperate_;
};

class PlatformChannelAdapter : public ICameraChannel {
public:
    explicit PlatformChannelAdapter(std::shared_ptr<ICommunicationAdapter> commAdapter);
    ~PlatformChannelAdapter() override = default;

    int32_t CloseSession() override;
    int32_t CreateSession(std::vector<DCameraIndex>& camIndexs,
                        std::string sessionFlag,
                        DCameraSessionMode sessionMode,
                        std::shared_ptr<ICameraChannelListener>& listener) override;
    int32_t ReleaseSession() override;
    int32_t SendData(std::shared_ptr<DataBuffer>& buffer) override;

private:
    std::shared_ptr<ICommunicationAdapter> commAdapter_;
    int32_t currentSocketId_ = -1;
    DCameraSessionMode currentSessionMode_ = DCAMERA_SESSION_MODE_CTRL;
    std::shared_ptr<ICameraChannelListener> channelListener_;
};

} // namespace DistributedHardware
} // namespace OHOS

#endif // OHOS_PLATFORM_COMPATIBILITY_ADAPTER_H