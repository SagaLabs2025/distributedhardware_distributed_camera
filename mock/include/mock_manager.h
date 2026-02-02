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

#ifndef OHOS_MOCK_MANAGER_H
#define OHOS_MOCK_MANAGER_H

#include <memory>
#include <string>
#include <map>
#include <mutex>

namespace OHOS {
namespace DistributedHardware {

class MockManager {
public:
    static MockManager& GetInstance();

    // Register a mock object
    template<typename T>
    void RegisterMock(const std::string& name, std::shared_ptr<T> mock);

    // Get a mock object
    template<typename T>
    std::shared_ptr<T> GetMock(const std::string& name);

    // Check if mock is enabled
    bool IsMockEnabled(const std::string& name);

    // Enable/disable specific mock
    void SetMockEnabled(const std::string& name, bool enabled);

    // Enable all mocks
    void EnableAllMocks();

    // Disable all mocks
    void DisableAllMocks();

private:
    MockManager() = default;
    ~MockManager() = default;

    std::map<std::string, bool> mockEnabled_;
    std::mutex managerLock_;
};

// Template implementation
template<typename T>
void MockManager::RegisterMock(const std::string& name, std::shared_ptr<T> mock) {
    // Store the mock object (implementation would go here)
    // For now, we'll just track enabled status
    std::lock_guard<std::mutex> lock(managerLock_);
    mockEnabled_[name] = true;
}

template<typename T>
std::shared_ptr<T> MockManager::GetMock(const std::string& name) {
    std::lock_guard<std::mutex> lock(managerLock_);
    if (mockEnabled_.find(name) != mockEnabled_.end() && mockEnabled_[name]) {
        // Return the actual mock object
        // This would be implemented in the .cpp file
    }
    return nullptr;
}

} // namespace DistributedHardware
} // namespace OHOS

#endif // OHOS_MOCK_MANAGER_H