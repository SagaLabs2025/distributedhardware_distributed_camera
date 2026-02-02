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

#ifndef OHOS_DCAMERA_VIRTUAL_BUS_H
#define OHOS_DCAMERA_VIRTUAL_BUS_H

#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include "data_buffer.h"

namespace OHOS {
namespace DistributedHardware {

// 共享內存配置
struct SharedMemoryConfig {
    size_t bufferSize;      // 緩衝區大小
    size_t maxMessages;     // 最大消息數量
    std::string name;       // 共享內存名稱
};

// 虛擬總線消息頭
struct VirtualBusMessageHeader {
    uint32_t messageId;     // 消息ID
    uint32_t dataSize;      // 數據大小
    uint64_t timestamp;     // 時間戳
    uint8_t priority;       // 優先級
    uint8_t reserved[3];    // 保留字段
};

// 虛擬總線接口
class DCameraVirtualBus {
public:
    explicit DCameraVirtualBus(const SharedMemoryConfig& config);
    ~DCameraVirtualBus();

    // 初始化虛擬總線
    int32_t Initialize();

    // 銷毀虛擬總線
    void Destroy();

    // 發送數據
    int32_t SendData(const std::shared_ptr<DataBuffer>& buffer, uint8_t priority = 0);

    // 接收數據
    std::shared_ptr<DataBuffer> ReceiveData();

    // 等待數據到達
    bool WaitForData(int32_t timeoutMs = -1);

    // 檢查是否有數據可讀
    bool HasData() const;

    // 獲取緩衝區使用率
    float GetBufferUsage() const;

private:
    // 平台相關的共享內存實現
    int32_t CreateSharedMemory();
    int32_t OpenSharedMemory();
    void CloseSharedMemory();

    // 平台相關的信號量實現
    int32_t CreateSemaphores();
    void DestroySemaphores();
    bool WaitSemaphore(const std::string& semName, int32_t timeoutMs);
    void PostSemaphore(const std::string& semName);

    SharedMemoryConfig config_;

    // 共享內存相關
    void* sharedMemory_ = nullptr;
    size_t sharedMemorySize_ = 0;
    bool initialized_ = false;

    // 信號量相關
    mutable std::mutex mutex_;
    std::atomic<uint32_t> readIndex_{0};
    std::atomic<uint32_t> writeIndex_{0};

    // 平台特定實現
#ifdef _WIN32
    HANDLE shmHandle_ = nullptr;
    HANDLE writeSem_ = nullptr;
    HANDLE readSem_ = nullptr;
    HANDLE mutex_ = nullptr;
#else
    int shmFd_ = -1;
    sem_t* writeSem_ = nullptr;
    sem_t* readSem_ = nullptr;
#endif
};

} // namespace DistributedHardware
} // namespace OHOS

#endif // OHOS_DCAMERA_VIRTUAL_BUS_H