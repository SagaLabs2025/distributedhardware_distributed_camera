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

#include "platform_interface.h"
#include "ffmpeg_codec.h"
#include "socket_communication_adapter.h"
#include "mock_platform_interfaces.h"
#include <memory>

namespace OHOS {
namespace DistributedHardware {

class PlatformFactory : public IPlatformFactory {
public:
    PlatformFactory() = default;
    ~PlatformFactory() override = default;

    std::shared_ptr<IDeviceManager> CreateDeviceManager() override {
        // In real implementation, this would create actual DeviceManager
        // For now, return mock implementation
        return std::make_shared<MockDeviceManagerImpl>();
    }

    std::shared_ptr<IHdfDeviceManager> CreateHdfDeviceManager() override {
        // In real implementation, this would create actual HDF DeviceManager
        // For now, return mock implementation
        return std::make_shared<MockHdfDeviceManagerImpl>();
    }

    std::shared_ptr<ICommunicationAdapter> CreateCommunicationAdapter() override {
        return std::make_shared<SocketCommunicationAdapter>();
    }

    std::shared_ptr<IVideoEncoder> CreateVideoEncoder() override {
        return std::make_shared<FFmpegVideoEncoder>();
    }

    std::shared_ptr<IVideoDecoder> CreateVideoDecoder() override {
        return std::make_shared<FFmpegVideoDecoder>();
    }

    std::shared_ptr<IDataBuffer> CreateDataBuffer(size_t initialSize) override {
        return std::make_shared<MockDataBuffer>(initialSize);
    }
};

// Global factory instance
std::shared_ptr<IPlatformFactory> g_platformFactory = std::make_shared<PlatformFactory>();

} // namespace DistributedHardware
} // namespace OHOS