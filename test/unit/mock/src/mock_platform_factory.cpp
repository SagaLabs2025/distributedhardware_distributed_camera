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

#include "mock_platform_factory.h"
#include "socket_communication_adapter.h"
#include "ffmpeg_codec.h"
#include <memory>

namespace OHOS {
namespace DistributedHardware {

std::shared_ptr<MockPlatformFactory> MockPlatformFactory::instance_ = nullptr;
std::mutex MockPlatformFactory::instanceMutex_;

MockPlatformFactory::MockPlatformFactory() {}

std::shared_ptr<IDeviceManager> MockPlatformFactory::CreateDeviceManager() {
    return std::make_shared<MockDeviceManagerImpl>();
}

std::shared_ptr<IHdfDeviceManager> MockPlatformFactory::CreateHdfDeviceManager() {
    return std::make_shared<MockHdfDeviceManagerImpl>();
}

std::shared_ptr<ICommunicationAdapter> MockPlatformFactory::CreateCommunicationAdapter() {
    return std::make_shared<SocketCommunicationAdapter>();
}

std::shared_ptr<IVideoEncoder> MockPlatformFactory::CreateVideoEncoder() {
    return std::make_shared<FFmpegVideoEncoder>();
}

std::shared_ptr<IVideoDecoder> MockPlatformFactory::CreateVideoDecoder() {
    return std::make_shared<FFmpegVideoDecoder>();
}

std::shared_ptr<IDataBuffer> MockPlatformFactory::CreateDataBuffer(size_t initialSize) {
    return std::make_shared<MockDataBuffer>(initialSize);
}

std::shared_ptr<MockPlatformFactory> MockPlatformFactory::GetInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex_);
    if (instance_ == nullptr) {
        instance_ = std::make_shared<MockPlatformFactory>();
    }
    return instance_;
}

// Global factory instance
std::shared_ptr<IPlatformFactory> g_mockPlatformFactory = MockPlatformFactory::GetInstance();

} // namespace DistributedHardware
} // namespace OHOS