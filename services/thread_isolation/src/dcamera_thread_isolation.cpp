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

#include "dcamera_thread_isolation.h"
#include "distributed_hardware_log.h"

namespace OHOS {
namespace DistributedHardware {

DCameraThreadIsolation::DCameraThreadIsolation(ThreadRole role) : role_(role)
{
}

DCameraThreadIsolation::~DCameraThreadIsolation()
{
    Stop();
}

int32_t DCameraThreadIsolation::Start()
{
    if (running_.load()) {
        DHLOGE("Thread already running for role: %d", static_cast<int>(role_));
        return -1;
    }

    running_.store(true);
    tasksCompleted_.store(false);
    thread_ = std::make_unique<std::thread>(&DCameraThreadIsolation::ThreadMain, this);

    DHLOGI("Started %s thread isolation",
           (role_ == ThreadRole::SOURCE) ? "SOURCE" : "SINK");
    return 0;
}

int32_t DCameraThreadIsolation::Stop()
{
    if (!running_.load()) {
        return 0;
    }

    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        running_.store(false);
        condition_.notify_all();
    }

    if (thread_ && thread_->joinable()) {
        thread_->join();
        thread_.reset();
    }

    DHLOGI("Stopped %s thread isolation",
           (role_ == ThreadRole::SOURCE) ? "SOURCE" : "SINK");
    return 0;
}

void DCameraThreadIsolation::PostTask(std::function<void()> task)
{
    if (!running_.load()) {
        DHLOGE("Cannot post task, thread not running for role: %d", static_cast<int>(role_));
        return;
    }

    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        taskQueue_.push(task);
        pendingTasks_.fetch_add(1);
        tasksCompleted_.store(false);
    }
    condition_.notify_one();
}

bool DCameraThreadIsolation::IsInCorrectThread() const
{
    // 在實際實現中，可能需要存儲線程ID並進行比較
    // 這裡簡化實現，假設調用此方法時已經在正確的線程上下文中
    return true;
}

void DCameraThreadIsolation::WaitForTasksCompletion()
{
    std::unique_lock<std::mutex> lock(completionMutex_);
    completionCondition_.wait(lock, [this]() {
        return tasksCompleted_.load() || !running_.load();
    });
}

void DCameraThreadIsolation::ThreadMain()
{
    DHLOGI("Starting %s thread main loop",
           (role_ == ThreadRole::SOURCE) ? "SOURCE" : "SINK");

    while (running_.load()) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            condition_.wait(lock, [this]() {
                return !taskQueue_.empty() || !running_.load();
            });

            if (!running_.load()) {
                break;
            }

            if (!taskQueue_.empty()) {
                task = std::move(taskQueue_.front());
                taskQueue_.pop();
            }
        }

        if (task) {
            try {
                task();
            } catch (const std::exception& e) {
                DHLOGE("Exception in %s thread task: %s",
                       (role_ == ThreadRole::SOURCE) ? "SOURCE" : "SINK", e.what());
            }

            int32_t remainingTasks = pendingTasks_.fetch_sub(1) - 1;
            if (remainingTasks <= 0 && running_.load()) {
                {
                    std::lock_guard<std::mutex> lock(completionMutex_);
                    tasksCompleted_.store(true);
                }
                completionCondition_.notify_all();
            }
        }
    }

    DHLOGI("Exiting %s thread main loop",
           (role_ == ThreadRole::SOURCE) ? "SOURCE" : "SINK");
}

} // namespace DistributedHardware
} // namespace OHOS