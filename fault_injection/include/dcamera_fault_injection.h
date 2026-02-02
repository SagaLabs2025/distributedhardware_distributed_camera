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

#ifndef OHOS_DCAMERA_FAULT_INJECTION_H
#define OHOS_DCAMERA_FAULT_INJECTION_H

#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <random>
#include <functional>

#include "data_buffer.h"

namespace OHOS {
namespace DistributedHardware {

// 故障注入類型
enum class FaultInjectionType {
    PACKET_DROP = 0,        // 丟包
    PACKET_DELAY = 1,       // 延遲
    FORMAT_CORRUPTION = 2,  // 格式衝突/損壞
    MEMORY_LEAK = 3,        // 記憶體洩漏模擬
    THREAD_BLOCK = 4,       // 線程阻塞
    RESOURCE_EXHAUSTION = 5 // 資源耗盡
};

// 故障注入配置
struct FaultInjectionConfig {
    FaultInjectionType type;           // 故障類型
    float probability;                 // 觸發概率 (0.0 - 1.0)
    int32_t delayMs;                  // 延遲毫秒數（僅用於PACKET_DELAY）
    std::string corruptionPattern;    // 損壞模式（僅用於FORMAT_CORRUPTION）
    bool enabled;                     // 是否啟用
};

// 故障注入統計
struct FaultInjectionStats {
    uint64_t totalPacketsProcessed;
    uint64_t packetsDropped;
    uint64_t packetsDelayed;
    uint64_t packetsCorrupted;
    uint64_t memoryLeakSimulations;
    uint64_t threadBlockSimulations;
    uint64_t resourceExhaustionSimulations;
};

// 故障注入接口
class DCameraFaultInjection {
public:
    DCameraFaultInjection();
    ~DCameraFaultInjection();

    // 設置故障注入配置
    void SetFaultInjectionConfig(const FaultInjectionConfig& config);

    // 批量設置多個故障注入配置
    void SetMultipleFaultInjectionConfigs(const std::vector<FaultInjectionConfig>& configs);

    // 注入故障到數據包
    std::shared_ptr<DataBuffer> InjectFault(const std::shared_ptr<DataBuffer>& originalBuffer);

    // 模擬記憶體洩漏
    void SimulateMemoryLeak(size_t leakSize = 1024);

    // 模擬線程阻塞
    void SimulateThreadBlock(int32_t blockMs = 100);

    // 模擬資源耗盡
    void SimulateResourceExhaustion();

    // 啟用/禁用故障注入
    void Enable(bool enabled);

    // 獲取故障注入統計
    FaultInjectionStats GetStatistics() const;

    // 重置統計
    void ResetStatistics();

private:
    // 內部故障注入實現
    std::shared_ptr<DataBuffer> InjectPacketDrop(const std::shared_ptr<DataBuffer>& buffer);
    std::shared_ptr<DataBuffer> InjectPacketDelay(const std::shared_ptr<DataBuffer>& buffer);
    std::shared_ptr<DataBuffer> InjectFormatCorruption(const std::shared_ptr<DataBuffer>& buffer);

    // 隨機數生成器
    bool ShouldInjectFault(float probability);

    std::vector<FaultInjectionConfig> configs_;
    mutable std::mutex configMutex_;

    std::atomic<bool> enabled_{false};
    mutable std::mutex statsMutex_;
    FaultInjectionStats stats_;

    std::random_device rd_;
    std::mt19937 gen_;
};

} // namespace DistributedHardware
} // namespace OHOS

#endif // OHOS_DCAMERA_FAULT_INJECTION_H