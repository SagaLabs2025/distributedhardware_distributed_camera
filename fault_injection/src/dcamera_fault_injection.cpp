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

#include "dcamera_fault_injection.h"
#include "distributed_hardware_log.h"

#include <chrono>
#include <thread>
#include <vector>

namespace OHOS {
namespace DistributedHardware {

DCameraFaultInjection::DCameraFaultInjection() : gen_(rd_())
{
    // 初始化統計信息
    stats_.totalPacketsProcessed = 0;
    stats_.packetsDropped = 0;
    stats_.packetsDelayed = 0;
    stats_.packetsCorrupted = 0;
    stats_.memoryLeakSimulations = 0;
    stats_.threadBlockSimulations = 0;
    stats_.resourceExhaustionSimulations = 0;
}

DCameraFaultInjection::~DCameraFaultInjection()
{
}

void DCameraFaultInjection::SetFaultInjectionConfig(const FaultInjectionConfig& config)
{
    std::lock_guard<std::mutex> lock(configMutex_);
    configs_.clear();
    configs_.push_back(config);
    DHLOGI("Set single fault injection config, type: %d, probability: %.2f, enabled: %s",
           static_cast<int>(config.type), config.probability, config.enabled ? "true" : "false");
}

void DCameraFaultInjection::SetMultipleFaultInjectionConfigs(const std::vector<FaultInjectionConfig>& configs)
{
    std::lock_guard<std::mutex> lock(configMutex_);
    configs_ = configs;
    DHLOGI("Set %zu fault injection configs", configs.size());
}

std::shared_ptr<DataBuffer> DCameraFaultInjection::InjectFault(const std::shared_ptr<DataBuffer>& originalBuffer)
{
    if (!enabled_.load() || !originalBuffer || originalBuffer->Size() == 0) {
        return originalBuffer;
    }

    stats_.totalPacketsProcessed++;

    std::lock_guard<std::mutex> lock(configMutex_);

    // 遍歷所有配置，應用故障注入
    std::shared_ptr<DataBuffer> currentBuffer = originalBuffer;

    for (const auto& config : configs_) {
        if (!config.enabled) {
            continue;
        }

        if (!ShouldInjectFault(config.probability)) {
            continue;
        }

        switch (config.type) {
            case FaultInjectionType::PACKET_DROP:
                currentBuffer = InjectPacketDrop(currentBuffer);
                break;
            case FaultInjectionType::PACKET_DELAY:
                currentBuffer = InjectPacketDelay(currentBuffer);
                if (config.delayMs > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(config.delayMs));
                }
                break;
            case FaultInjectionType::FORMAT_CORRUPTION:
                currentBuffer = InjectFormatCorruption(currentBuffer);
                break;
            default:
                // 其他故障類型在這裡不處理數據包
                break;
        }

        // 如果數據包被丟棄，直接返回nullptr
        if (!currentBuffer) {
            break;
        }
    }

    return currentBuffer;
}

void DCameraFaultInjection::SimulateMemoryLeak(size_t leakSize)
{
    if (!enabled_.load()) {
        return;
    }

    // 分配內存但不釋放（模擬洩漏）
    void* leakedMemory = malloc(leakSize);
    if (leakedMemory) {
        // 填充一些數據以確保內存被實際使用
        memset(leakedMemory, 0xAB, leakSize);
        stats_.memoryLeakSimulations++;
        DHLOGW("Simulated memory leak of %zu bytes", leakSize);
    }
}

void DCameraFaultInjection::SimulateThreadBlock(int32_t blockMs)
{
    if (!enabled_.load() || blockMs <= 0) {
        return;
    }

    stats_.threadBlockSimulations++;
    DHLOGW("Simulating thread block for %d ms", blockMs);
    std::this_thread::sleep_for(std::chrono::milliseconds(blockMs));
}

void DCameraFaultInjection::SimulateResourceExhaustion()
{
    if (!enabled_.load()) {
        return;
    }

    stats_.resourceExhaustionSimulations++;
    DHLOGW("Simulating resource exhaustion");

    // 模擬資源耗盡：創建大量小對象
    std::vector<std::shared_ptr<std::vector<uint8_t>>> resources;
    try {
        for (int i = 0; i < 1000; i++) {
            auto resource = std::make_shared<std::vector<uint8_t>>(1024);
            resources.push_back(resource);
        }
    } catch (const std::bad_alloc& e) {
        DHLOGE("Resource exhaustion simulation caught bad_alloc: %s", e.what());
    }
}

void DCameraFaultInjection::Enable(bool enabled)
{
    enabled_.store(enabled);
    DHLOGI("Fault injection %s", enabled ? "enabled" : "disabled");
}

DCameraFaultInjection::FaultInjectionStats DCameraFaultInjection::GetStatistics() const
{
    std::lock_guard<std::mutex> lock(statsMutex_);
    return stats_;
}

void DCameraFaultInjection::ResetStatistics()
{
    std::lock_guard<std::mutex> lock(statsMutex_);
    stats_.totalPacketsProcessed = 0;
    stats_.packetsDropped = 0;
    stats_.packetsDelayed = 0;
    stats_.packetsCorrupted = 0;
    stats_.memoryLeakSimulations = 0;
    stats_.threadBlockSimulations = 0;
    stats_.resourceExhaustionSimulations = 0;
    DHLOGI("Fault injection statistics reset");
}

std::shared_ptr<DataBuffer> DCameraFaultInjection::InjectPacketDrop(const std::shared_ptr<DataBuffer>& buffer)
{
    stats_.packetsDropped++;
    DHLOGW("Injected packet drop");
    return nullptr; // 返回nullptr表示丟包
}

std::shared_ptr<DataBuffer> DCameraFaultInjection::InjectPacketDelay(const std::shared_ptr<DataBuffer>& buffer)
{
    stats_.packetsDelayed++;
    DHLOGW("Injected packet delay");
    return buffer; // 延遲在調用方處理
}

std::shared_ptr<DataBuffer> DCameraFaultInjection::InjectFormatCorruption(const std::shared_ptr<DataBuffer>& buffer)
{
    if (!buffer || buffer->Size() == 0) {
        return buffer;
    }

    stats_.packetsCorrupted++;

    // 創建損壞的緩衝區副本
    auto corruptedBuffer = std::make_shared<DataBuffer>(*buffer);
    if (!corruptedBuffer) {
        DHLOGE("Failed to create corrupted buffer");
        return buffer;
    }

    // 隨機損壞一些字節
    uint8_t* data = corruptedBuffer->GetMutableData();
    size_t size = corruptedBuffer->Size();

    // 損壞前幾個字節（通常是協議頭）
    size_t corruptCount = std::min(static_cast<size_t>(4), size);
    for (size_t i = 0; i < corruptCount; i++) {
        data[i] ^= 0xFF; // 反轉所有位
    }

    DHLOGW("Injected format corruption, corrupted %zu bytes", corruptCount);
    return corruptedBuffer;
}

bool DCameraFaultInjection::ShouldInjectFault(float probability)
{
    if (probability <= 0.0f) {
        return false;
    }
    if (probability >= 1.0f) {
        return true;
    }

    std::uniform_real_distribution<float> dis(0.0f, 1.0f);
    return dis(gen_) < probability;
}

} // namespace DistributedHardware
} // namespace OHOS