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

#include "dcamera_virtual_bus.h"
#include "distributed_hardware_log.h"

#include <cstring>
#include <chrono>
#include <thread>

namespace OHOS {
namespace DistributedHardware {

DCameraVirtualBus::DCameraVirtualBus(const SharedMemoryConfig& config) : config_(config)
{
}

DCameraVirtualBus::~DCameraVirtualBus()
{
    Destroy();
}

int32_t DCameraVirtualBus::Initialize()
{
    if (initialized_) {
        DHLOGI("Virtual bus already initialized");
        return 0;
    }

    // 創建共享內存
    int32_t ret = CreateSharedMemory();
    if (ret != 0) {
        DHLOGE("Failed to create shared memory, ret: %d", ret);
        return ret;
    }

    // 創建信號量
    ret = CreateSemaphores();
    if (ret != 0) {
        DHLOGE("Failed to create semaphores, ret: %d", ret);
        CloseSharedMemory();
        return ret;
    }

    initialized_ = true;
    DHLOGI("Virtual bus initialized successfully, name: %s", config_.name.c_str());
    return 0;
}

void DCameraVirtualBus::Destroy()
{
    if (!initialized_) {
        return;
    }

    DestroySemaphores();
    CloseSharedMemory();
    initialized_ = false;
    readIndex_.store(0);
    writeIndex_.store(0);
    DHLOGI("Virtual bus destroyed, name: %s", config_.name.c_str());
}

int32_t DCameraVirtualBus::SendData(const std::shared_ptr<DataBuffer>& buffer, uint8_t priority)
{
    if (!initialized_ || !buffer) {
        DHLOGE("Invalid state or buffer");
        return -1;
    }

    if (buffer->Size() == 0 || buffer->Size() > config_.bufferSize) {
        DHLOGE("Invalid buffer size: %zu, max allowed: %zu",
               buffer->Size(), config_.bufferSize);
        return -1;
    }

    // 等待寫入信號量（實現流控）
    if (!WaitSemaphore("write", 1000)) { // 1秒超時
        DHLOGE("Timeout waiting for write semaphore");
        return -1;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);

        // 計算可用空間
        uint32_t currentRead = readIndex_.load();
        uint32_t currentWrite = writeIndex_.load();
        size_t availableSpace = (currentRead > currentWrite) ?
            (currentRead - currentWrite - sizeof(VirtualBusMessageHeader)) :
            (config_.bufferSize - currentWrite - sizeof(VirtualBusMessageHeader));

        if (availableSpace < buffer->Size()) {
            PostSemaphore("write"); // 釋放信號量
            DHLOGE("Insufficient space in buffer, needed: %zu, available: %zu",
                   buffer->Size(), availableSpace);
            return -1;
        }

        // 寫入消息頭
        VirtualBusMessageHeader header;
        header.messageId = currentWrite;
        header.dataSize = static_cast<uint32_t>(buffer->Size());
        header.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        header.priority = priority;
        memset(header.reserved, 0, sizeof(header.reserved));

        uint8_t* shmPtr = static_cast<uint8_t*>(sharedMemory_);
        memcpy(shmPtr + currentWrite, &header, sizeof(header));
        currentWrite += sizeof(header);

        // 寫入數據
        memcpy(shmPtr + currentWrite, buffer->GetData(), buffer->Size());
        currentWrite += buffer->Size();

        // 更新寫入索引
        writeIndex_.store(currentWrite);
    }

    // 發送讀取信號量，通知有數據可讀
    PostSemaphore("read");

    DHLOGD("Data sent successfully, size: %zu, priority: %u",
           buffer->Size(), priority);
    return 0;
}

std::shared_ptr<DataBuffer> DCameraVirtualBus::ReceiveData()
{
    if (!initialized_) {
        DHLOGE("Virtual bus not initialized");
        return nullptr;
    }

    // 等待讀取信號量
    if (!WaitSemaphore("read", 0)) { // 非阻塞
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    uint32_t currentRead = readIndex_.load();
    uint32_t currentWrite = writeIndex_.load();

    if (currentRead == currentWrite) {
        PostSemaphore("read"); // 釋放信號量
        return nullptr;
    }

    uint8_t* shmPtr = static_cast<uint8_t*>(sharedMemory_);

    // 讀取消息頭
    VirtualBusMessageHeader header;
    memcpy(&header, shmPtr + currentRead, sizeof(header));
    currentRead += sizeof(header);

    if (header.dataSize == 0 || header.dataSize > config_.bufferSize) {
        PostSemaphore("read");
        DHLOGE("Invalid message size: %u", header.dataSize);
        return nullptr;
    }

    // 創建數據緩衝區
    auto dataBuffer = std::make_shared<DataBuffer>(header.dataSize);
    if (!dataBuffer) {
        PostSemaphore("read");
        DHLOGE("Failed to create data buffer");
        return nullptr;
    }

    // 讀取數據
    memcpy(dataBuffer->GetMutableData(), shmPtr + currentRead, header.dataSize);
    currentRead += header.dataSize;

    // 更新讀取索引
    readIndex_.store(currentRead);

    // 如果緩衝區已滿，釋放寫入信號量
    if (currentRead + sizeof(VirtualBusMessageHeader) + 1024 >= config_.bufferSize) {
        PostSemaphore("write");
    }

    DHLOGD("Data received successfully, size: %u, priority: %u",
           header.dataSize, header.priority);
    return dataBuffer;
}

bool DCameraVirtualBus::WaitForData(int32_t timeoutMs)
{
    if (!initialized_) {
        return false;
    }

    if (timeoutMs == 0) {
        return HasData();
    }

    if (HasData()) {
        return true;
    }

    if (timeoutMs < 0) {
        // 永久等待
        return WaitSemaphore("read", -1);
    }

    // 有限時間等待
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
           std::chrono::steady_clock::now() - start).count() < timeoutMs) {
        if (HasData()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return HasData();
}

bool DCameraVirtualBus::HasData() const
{
    if (!initialized_) {
        return false;
    }

    uint32_t currentRead = readIndex_.load();
    uint32_t currentWrite = writeIndex_.load();
    return currentRead != currentWrite;
}

float DCameraVirtualBus::GetBufferUsage() const
{
    if (!initialized_) {
        return 0.0f;
    }

    uint32_t currentRead = readIndex_.load();
    uint32_t currentWrite = writeIndex_.load();
    size_t usedSize = (currentWrite >= currentRead) ?
        (currentWrite - currentRead) : (config_.bufferSize - currentRead + currentWrite);

    return static_cast<float>(usedSize) / static_cast<float>(config_.bufferSize);
}

// Windows平台實現
#ifdef _WIN32

int32_t DCameraVirtualBus::CreateSharedMemory()
{
    // 創建或打開共享內存
    std::string shmName = "DCameraShm_" + config_.name;
    shmHandle_ = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        static_cast<DWORD>(config_.bufferSize),
        shmName.c_str()
    );

    if (shmHandle_ == nullptr) {
        DHLOGE("Failed to create file mapping, error: %lu", GetLastError());
        return -1;
    }

    sharedMemory_ = MapViewOfFile(shmHandle_, FILE_MAP_ALL_ACCESS, 0, 0, config_.bufferSize);
    if (sharedMemory_ == nullptr) {
        DHLOGE("Failed to map view of file, error: %lu", GetLastError());
        CloseHandle(shmHandle_);
        shmHandle_ = nullptr;
        return -1;
    }

    sharedMemorySize_ = config_.bufferSize;
    return 0;
}

int32_t DCameraVirtualBus::CreateSemaphores()
{
    std::string writeSemName = "DCameraWriteSem_" + config_.name;
    std::string readSemName = "DCameraReadSem_" + config_.name;
    std::string mutexName = "DCameraMutex_" + config_.name;

    // 創建寫入信號量（初始值為緩衝區大小/消息大小）
    writeSem_ = CreateSemaphoreA(nullptr, static_cast<LONG>(config_.maxMessages),
                                static_cast<LONG>(config_.maxMessages), writeSemName.c_str());
    if (writeSem_ == nullptr) {
        DHLOGE("Failed to create write semaphore, error: %lu", GetLastError());
        return -1;
    }

    // 創建讀取信號量（初始值為0）
    readSem_ = CreateSemaphoreA(nullptr, 0, static_cast<LONG>(config_.maxMessages), readSemName.c_str());
    if (readSem_ == nullptr) {
        DHLOGE("Failed to create read semaphore, error: %lu", GetLastError());
        CloseHandle(writeSem_);
        writeSem_ = nullptr;
        return -1;
    }

    // 創建互斥鎖
    mutex_ = CreateMutexA(nullptr, FALSE, mutexName.c_str());
    if (mutex_ == nullptr) {
        DHLOGE("Failed to create mutex, error: %lu", GetLastError());
        CloseHandle(writeSem_);
        CloseHandle(readSem_);
        writeSem_ = nullptr;
        readSem_ = nullptr;
        return -1;
    }

    return 0;
}

void DCameraVirtualBus::CloseSharedMemory()
{
    if (sharedMemory_) {
        UnmapViewOfFile(sharedMemory_);
        sharedMemory_ = nullptr;
    }
    if (shmHandle_) {
        CloseHandle(shmHandle_);
        shmHandle_ = nullptr;
    }
}

void DCameraVirtualBus::DestroySemaphores()
{
    if (writeSem_) {
        CloseHandle(writeSem_);
        writeSem_ = nullptr;
    }
    if (readSem_) {
        CloseHandle(readSem_);
        readSem_ = nullptr;
    }
    if (mutex_) {
        CloseHandle(mutex_);
        mutex_ = nullptr;
    }
}

bool DCameraVirtualBus::WaitSemaphore(const std::string& semName, int32_t timeoutMs)
{
    HANDLE sem = (semName == "write") ? writeSem_ : readSem_;
    if (sem == nullptr) {
        return false;
    }

    DWORD timeout = (timeoutMs < 0) ? INFINITE : static_cast<DWORD>(timeoutMs);
    DWORD result = WaitForSingleObject(sem, timeout);
    return result == WAIT_OBJECT_0;
}

void DCameraVirtualBus::PostSemaphore(const std::string& semName)
{
    HANDLE sem = (semName == "write") ? writeSem_ : readSem_;
    if (sem != nullptr) {
        ReleaseSemaphore(sem, 1, nullptr);
    }
}

#else // POSIX平台

int32_t DCameraVirtualBus::CreateSharedMemory()
{
    std::string shmName = "/DCameraShm_" + config_.name;

    // 創建共享內存對象
    shmFd_ = shm_open(shmName.c_str(), O_CREAT | O_RDWR, 0666);
    if (shmFd_ == -1) {
        DHLOGE("Failed to create shared memory, error: %s", strerror(errno));
        return -1;
    }

    // 設置共享內存大小
    if (ftruncate(shmFd_, config_.bufferSize) == -1) {
        DHLOGE("Failed to set shared memory size, error: %s", strerror(errno));
        close(shmFd_);
        shmFd_ = -1;
        return -1;
    }

    // 映射共享內存
    sharedMemory_ = mmap(nullptr, config_.bufferSize, PROT_READ | PROT_WRITE, MAP_SHARED, shmFd_, 0);
    if (sharedMemory_ == MAP_FAILED) {
        DHLOGE("Failed to map shared memory, error: %s", strerror(errno));
        close(shmFd_);
        shmFd_ = -1;
        return -1;
    }

    sharedMemorySize_ = config_.bufferSize;
    return 0;
}

int32_t DCameraVirtualBus::CreateSemaphores()
{
    std::string writeSemName = "/DCameraWriteSem_" + config_.name;
    std::string readSemName = "/DCameraReadSem_" + config_.name;

    // 創建寫入信號量
    writeSem_ = sem_open(writeSemName.c_str(), O_CREAT, 0666, config_.maxMessages);
    if (writeSem_ == SEM_FAILED) {
        DHLOGE("Failed to create write semaphore, error: %s", strerror(errno));
        return -1;
    }

    // 創建讀取信號量
    readSem_ = sem_open(readSemName.c_str(), O_CREAT, 0666, 0);
    if (readSem_ == SEM_FAILED) {
        DHLOGE("Failed to create read semaphore, error: %s", strerror(errno));
        sem_close(writeSem_);
        sem_unlink(writeSemName.c_str());
        writeSem_ = nullptr;
        return -1;
    }

    return 0;
}

void DCameraVirtualBus::CloseSharedMemory()
{
    if (sharedMemory_ && sharedMemory_ != MAP_FAILED) {
        munmap(sharedMemory_, sharedMemorySize_);
        sharedMemory_ = nullptr;
    }
    if (shmFd_ != -1) {
        close(shmFd_);
        shmFd_ = -1;
    }
}

void DCameraVirtualBus::DestroySemaphores()
{
    if (writeSem_ && writeSem_ != SEM_FAILED) {
        sem_close(writeSem_);
        writeSem_ = nullptr;
    }
    if (readSem_ && readSem_ != SEM_FAILED) {
        sem_close(readSem_);
        readSem_ = nullptr;
    }
}

bool DCameraVirtualBus::WaitSemaphore(const std::string& semName, int32_t timeoutMs)
{
    sem_t* sem = (semName == "write") ? writeSem_ : readSem_;
    if (sem == nullptr || sem == SEM_FAILED) {
        return false;
    }

    if (timeoutMs == 0) {
        // 非阻塞
        return sem_trywait(sem) == 0;
    } else if (timeoutMs < 0) {
        // 永久阻塞
        return sem_wait(sem) == 0;
    } else {
        // 有限時間阻塞
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeoutMs / 1000;
        ts.tv_nsec += (timeoutMs % 1000) * 1000000;
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000;
        }
        return sem_timedwait(sem, &ts) == 0;
    }
}

void DCameraVirtualBus::PostSemaphore(const std::string& semName)
{
    sem_t* sem = (semName == "write") ? writeSem_ : readSem_;
    if (sem && sem != SEM_FAILED) {
        sem_post(sem);
    }
}

#endif // 平台特定實現

} // namespace DistributedHardware
} // namespace OHOS