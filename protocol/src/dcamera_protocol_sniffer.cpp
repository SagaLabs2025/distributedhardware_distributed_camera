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

#include "dcamera_protocol_sniffer.h"
#include "distributed_hardware_log.h"
#include "cJSON.h"

#include <cstring>
#include <chrono>

namespace OHOS {
namespace DistributedHardware {

// 靜態常量定義
const std::vector<std::string> DCameraProtocolSniffer::SUPPORTED_PROTOCOL_VERSIONS = {
    "1.0", "1.1", "2.0"
};

const std::vector<std::string> DCameraProtocolSniffer::SUPPORTED_PROTOCOL_TYPES = {
    "MESSAGE", "OPERATION"
};

const std::vector<std::string> DCameraProtocolSniffer::SUPPORTED_COMMAND_TYPES = {
    "GET_INFO", "CHAN_NEG", "UPDATE_METADATA", "METADATA_RESULT",
    "STATE_NOTIFY", "CAPTURE", "STOP_CAPTURE", "OPEN_CHANNEL",
    "CLOSE_CHANNEL"
};

DCameraProtocolSniffer::DCameraProtocolSniffer()
{
    // 初始化統計信息
    stats_.totalPackets = 0;
    stats_.validPackets = 0;
    stats_.invalidPackets = 0;
    stats_.consistencyErrors = 0;
    stats_.formatErrors = 0;
    stats_.versionMismatches = 0;
    stats_.unknownCommands = 0;
}

DCameraProtocolSniffer::~DCameraProtocolSniffer()
{
}

void DCameraProtocolSniffer::SetCallback(std::shared_ptr<IProtocolSnifferCallback> callback)
{
    std::lock_guard<std::mutex> lock(callbackMutex_);
    callback_ = callback;
}

bool DCameraProtocolSniffer::MonitorIncomingPacket(const std::shared_ptr<DataBuffer>& buffer)
{
    if (!enabled_.load() || !buffer) {
        return false;
    }

    stats_.totalPackets++;

    DCameraCmdPack pack;
    if (!ParseDCameraCmdPack(buffer, pack)) {
        stats_.invalidPackets++;
        HandleProtocolError("FORMAT_ERROR", "Failed to parse incoming packet");
        return false;
    }

    if (!ValidateProtocolConsistency(pack)) {
        stats_.consistencyErrors++;
        return false;
    }

    stats_.validPackets++;
    DHLOGD("Incoming packet validated successfully, command: %s", pack.command.c_str());
    return true;
}

bool DCameraProtocolSniffer::MonitorOutgoingPacket(const std::shared_ptr<DataBuffer>& buffer)
{
    if (!enabled_.load() || !buffer) {
        return false;
    }

    stats_.totalPackets++;

    DCameraCmdPack pack;
    if (!ParseDCameraCmdPack(buffer, pack)) {
        stats_.invalidPackets++;
        HandleProtocolError("FORMAT_ERROR", "Failed to parse outgoing packet");
        return false;
    }

    if (!ValidateProtocolConsistency(pack)) {
        stats_.consistencyErrors++;
        return false;
    }

    stats_.validPackets++;
    DHLOGD("Outgoing packet validated successfully, command: %s", pack.command.c_str());
    return true;
}

bool DCameraProtocolSniffer::ValidateProtocolConsistency(const DCameraCmdPack& pack)
{
    bool isValid = true;

    // 驗證協議類型
    if (!ValidateProtocolType(pack.type)) {
        HandleProtocolError("UNKNOWN_PROTOCOL_TYPE", pack.type);
        isValid = false;
    }

    // 驗證命令類型
    if (!ValidateCommandType(pack.command)) {
        HandleProtocolError("UNKNOWN_COMMAND", pack.command);
        stats_.unknownCommands++;
        isValid = false;
    }

    // 驗證協議版本
    if (!ValidateProtocolVersion(pack.version)) {
        HandleProtocolError("VERSION_MISMATCH", pack.version);
        stats_.versionMismatches++;
        isValid = false;
    }

    // 驗證DHID格式
    if (!ValidateDhId(pack.dhId)) {
        HandleProtocolError("INVALID_DHID", pack.dhId);
        isValid = false;
    }

    return isValid;
}

void DCameraProtocolSniffer::Enable(bool enabled)
{
    enabled_.store(enabled);
    DHLOGI("Protocol sniffer %s", enabled ? "enabled" : "disabled");
}

DCameraProtocolSniffer::Statistics DCameraProtocolSniffer::GetStatistics() const
{
    std::lock_guard<std::mutex> lock(statsMutex_);
    return stats_;
}

bool DCameraProtocolSniffer::ParseDCameraCmdPack(const std::shared_ptr<DataBuffer>& buffer, DCameraCmdPack& pack)
{
    if (!buffer || buffer->Size() == 0) {
        return false;
    }

    // 假設數據是JSON格式（基於現有代碼結構）
    std::string jsonString(reinterpret_cast<const char*>(buffer->GetData()), buffer->Size());

    cJSON* root = cJSON_Parse(jsonString.c_str());
    if (!root) {
        DHLOGE("Failed to parse JSON");
        return false;
    }

    // 解析各個字段
    cJSON* type = cJSON_GetObjectItemCaseSensitive(root, "Type");
    if (type && cJSON_IsString(type)) {
        pack.type = type->valuestring;
    } else {
        pack.type = "OPERATION"; // 默認值
    }

    cJSON* dhId = cJSON_GetObjectItemCaseSensitive(root, "dhId");
    if (dhId && cJSON_IsString(dhId)) {
        pack.dhId = dhId->valuestring;
    }

    cJSON* command = cJSON_GetObjectItemCaseSensitive(root, "Command");
    if (command && cJSON_IsString(command)) {
        pack.command = command->valuestring;
    }

    // 序列號和時間戳使用當前時間
    pack.sequence = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    pack.timestamp = pack.sequence;

    // 版本默認為1.0
    pack.version = "1.0";

    // 值字段保持原始JSON字符串
    pack.value = jsonString;

    cJSON_Delete(root);
    return true;
}

bool DCameraProtocolSniffer::ValidateProtocolType(const std::string& type)
{
    for (const auto& supportedType : SUPPORTED_PROTOCOL_TYPES) {
        if (supportedType == type) {
            return true;
        }
    }
    return false;
}

bool DCameraProtocolSniffer::ValidateCommandType(const std::string& command)
{
    for (const auto& supportedCommand : SUPPORTED_COMMAND_TYPES) {
        if (supportedCommand == command) {
            return true;
        }
    }
    return false;
}

bool DCameraProtocolSniffer::ValidateProtocolVersion(const std::string& version)
{
    for (const auto& supportedVersion : SUPPORTED_PROTOCOL_VERSIONS) {
        if (supportedVersion == version) {
            return true;
        }
    }
    return false;
}

bool DCameraProtocolSniffer::ValidateDhId(const std::string& dhId)
{
    // 簡單的DHID驗證：非空且長度合理
    return !dhId.empty() && dhId.length() <= 256;
}

void DCameraProtocolSniffer::HandleProtocolError(const std::string& errorType, const std::string& details)
{
    DHLOGE("Protocol error: %s, details: %s", errorType.c_str(), details.c_str());

    std::lock_guard<std::mutex> lock(callbackMutex_);
    if (callback_) {
        if (errorType == "UNKNOWN_COMMAND") {
            callback_->OnUnknownProtocolCommand(details);
        } else if (errorType == "VERSION_MISMATCH") {
            callback_->OnProtocolVersionMismatch(details, "1.0"); // 假設本地版本為1.0
        } else if (errorType == "FORMAT_ERROR") {
            callback_->OnInvalidProtocolFormat("Protocol format error", details);
        } else {
            callback_->OnProtocolInconsistency("Protocol inconsistency detected", errorType, details);
        }
    }
}

} // namespace DistributedHardware
} // namespace OHOS