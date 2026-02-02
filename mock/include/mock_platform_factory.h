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

#ifndef OHOS_MOCK_PLATFORM_FACTORY_H
#define OHOS_MOCK_PLATFORM_FACTORY_H

#include "platform_interface.h"
#include "mock_platform_interfaces.h"
#include <memory>

namespace OHOS {
namespace DistributedHardware {

class MockPlatformFactory : public IPlatformFactory {
public:
    MockPlatformFactory();
    ~MockPlatformFactory() override = default;

    std::shared_ptr<IDeviceManager> CreateDeviceManager() override;
    std::shared_ptr<IHdfDeviceManager> CreateHdfDeviceManager() override;
    std::shared_ptr<ICommunicationAdapter> CreateCommunicationAdapter() override;
    std::shared_ptr<IVideoEncoder> CreateVideoEncoder() override;
    std::shared_ptr<IVideoDecoder> CreateVideoDecoder() override;
    std::shared_ptr<IDataBuffer> CreateDataBuffer(size_t initialSize = 0) override;

    // Singleton access
    static std::shared_ptr<MockPlatformFactory> GetInstance();

private:
    static std::shared_ptr<MockPlatformFactory> instance_;
    static std::mutex instanceMutex_;
};

// Global factory instance
extern std::shared_ptr<IPlatformFactory> g_mockPlatformFactory;

} // namespace DistributedHardware
} // namespace OHOS

#endif // OHOS_MOCK_PLATFORM_FACTORY_H