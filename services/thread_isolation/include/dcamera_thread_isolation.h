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

#ifndef OHOS_DCAMERA_THREAD_ISOLATION_H
#define OHOS_DCAMERA_THREAD_ISOLATION_H

#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <functional>
#include <atomic>

namespace OHOS {
namespace DistributedHardware {

enum class ThreadRole {
    SOURCE,
    SINK
};

class DCameraThreadIsolation {
public:
    explicit DCameraThreadIsolation(ThreadRole role);
    ~DCameraThreadIsolation();

    // 啟動隔離線程
    int32_t Start();

    // 停止隔離線程
    int32_t Stop();

    // 提交任務到指定線程
    void PostTask(std::function<void()> task);

    // 獲取當前線程角色
    ThreadRole GetRole() const { return role_; }

    // 檢查是否在正確的線程中執行
    bool IsInCorrectThread() const;

    // 等待所有任務完成
    void WaitForTasksCompletion();

private:
    void ThreadMain();

    ThreadRole role_;
    std::unique_ptr<std::thread> thread_;
    mutable std::mutex queueMutex_;
    std::queue<std::function<void()>> taskQueue_;
    std::condition_variable condition_;
    std::atomic<bool> running_{false};
    std::atomic<bool> tasksCompleted_{true};
    std::atomic<int32_t> pendingTasks_{0};
    mutable std::mutex completionMutex_;
    std::condition_variable completionCondition_;
};

} // namespace DistributedHardware
} // namespace OHOS

#endif // OHOS_DCAMERA_THREAD_ISOLATION_H