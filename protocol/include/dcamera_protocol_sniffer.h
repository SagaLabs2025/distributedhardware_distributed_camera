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

#ifndef OHOS_DCAMERA_PROTOCOL_SNIFFER_H
#define OHOS_DCAMERA_PROTOCOL_SNIFFER_H

#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <functional>

#include "data_buffer.h"

namespace OHOS {
namespace DistributedHardware {

// 協議嗅探器回調接口
class IProtocolSnifferCallback {
public:
    virtual ~IProtocolSnifferCallback() = default;

    // 檢測到協議不一致
    virtual void OnProtocolInconsistency(const std::string& message, const std::string& expected,
                                       const std::string& actual) = 0;

    // 檢測到無效協議格式
    virtual void OnInvalidProtocolFormat(const std::string& message, const std::string& error) = 0;

    // 檢測到協議版本不匹配
    virtual void OnProtocolVersionMismatch(const std::string& localVersion,
                                        const std::string& remoteVersion) = 0;

    // 檢測到未知協議命令
    virtual void OnUnknownProtocolCommand(const std::string& command) = 0;
};

// DCameraCmdPack協議結構
struct DCameraCmdPack {
    std::string type;           // 協議類型 (MESSAGE/OPERATION)
    std::string dhId;          // 設備硬件ID
    std::string command;       // 命令類型
    std::string value;         // 值（JSON格式）
    uint32_t sequence;        // 序列號
    uint64_t timestamp;       // 時間戳
    std::string version;      // 協議版本
};

// 協議嗅探器
class DCameraProtocolSniffer {
public:
    DCameraProtocolSniffer();
    ~DCameraProtocolSniffer();

    // 設置回調
    void SetCallback(std::shared_ptr<IProtocolSnifferCallback> callback);

    // 監控傳入的數據包
    bool MonitorIncomingPacket(const std::shared_ptr<DataBuffer>& buffer);

    // 監控傳出的數據包
    bool MonitorOutgoingPacket(const std::shared_ptr<DataBuffer>& buffer);

    // 驗證協議一致性
    bool ValidateProtocolConsistency(const DCameraCmdPack& pack);

    // 啟用/禁用嗅探器
    void Enable(bool enabled);

    // 獲取統計信息
    struct Statistics {
        uint64_t totalPackets;
        uint64_t validPackets;
        uint64_t invalidPackets;
        uint64_t consistencyErrors;
        uint64_t formatErrors;
        uint64_t versionMismatches;
        uint64_t unknownCommands;
    };
    Statistics GetStatistics() const;

private:
    // 解析DCameraCmdPack
    bool ParseDCameraCmdPack(const std::shared_ptr<DataBuffer>& buffer, DCameraCmdPack& pack);

    // 驗證協議類型
    bool ValidateProtocolType(const std::string& type);

    // 驗證命令類型
    bool ValidateCommandType(const std::string& command);

    // 驗證協議版本
    bool ValidateProtocolVersion(const std::string& version);

    // 驗證DHID格式
    bool ValidateDhId(const std::string& dhId);

    // 處理協議錯誤
    void HandleProtocolError(const std::string& errorType, const std::string& details);

    std::shared_ptr<IProtocolSnifferCallback> callback_;
    mutable std::mutex callbackMutex_;

    std::atomic<bool> enabled_{true};
    mutable std::mutex statsMutex_;
    Statistics stats_;

    // 支持的協議版本
    static const std::vector<std::string> SUPPORTED_PROTOCOL_VERSIONS;

    // 支持的協議類型
    static const std::vector<std::string> SUPPORTED_PROTOCOL_TYPES;

    // 支持的命令類型
    static const std::vector<std::string> SUPPORTED_COMMAND_TYPES;
};

} // namespace DistributedHardware
} // namespace OHOS

#endif // OHOS_DCAMERA_PROTOCOL_SNIFFER_H