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

#include "mock_sink_channel.h"
#include "distributed_hardware_log.h"
#include <thread>
#include <chrono>

namespace OHOS {
namespace DistributedHardware {

MockSinkChannel::MockSinkChannel()
{
    DHLOGI("[SINK] MockSinkChannel created");
}

MockSinkChannel::~MockSinkChannel()
{
    if (isInitialized_) {
        Release();
    }
    DHLOGI("[SINK] MockSinkChannel destroyed");
}

int32_t MockSinkChannel::Init()
{
    if (isInitialized_) {
        DHLOGW("[SINK] MockSinkChannel already initialized");
        return DCAMERA_OK;
    }

    isInitialized_ = true;
    sendCount_.store(0);
    receiveCount_.store(0);
    receivedCommands_.clear();
    DHLOGI("[SINK] MockSinkChannel initialized SUCCESS");
    return DCAMERA_OK;
}

int32_t MockSinkChannel::Release()
{
    if (!isInitialized_) {
        DHLOGW("[SINK] MockSinkChannel not initialized");
        return DCAMERA_OK;
    }

    isInitialized_ = false;
    DHLOGI("[SINK] MockSinkChannel released");
    return DCAMERA_OK;
}

int32_t MockSinkChannel::SendData(std::shared_ptr<DataBuffer>& buffer)
{
    if (!isInitialized_) {
        DHLOGE("[SINK] MockSinkChannel not initialized");
        return DCAMERA_BAD_VALUE;
    }

    if (buffer == nullptr) {
        DHLOGE("[SINK] SendData: buffer is nullptr");
        return DCAMERA_BAD_VALUE;
    }

    sendCount_.fetch_add(1);
    DHLOGI("[SINK] Sending encoded data via Channel, size: %{public}zu, count: %{public}d",
           buffer->GetSize(), sendCount_.load());

    // 模拟通过网络发送数据
    return DCAMERA_OK;
}

int32_t MockSinkChannel::ReceiveData(std::shared_ptr<DataBuffer>& buffer)
{
    if (!isInitialized_) {
        DHLOGE("[SINK] MockSinkChannel not initialized");
        return DCAMERA_BAD_VALUE;
    }

    // 模拟接收延迟
    if (receiveDelayMs_ > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(receiveDelayMs_));
    }

    receiveCount_.fetch_add(1);
    DHLOGI("[SINK] Received data from Source, count: %{public}d", receiveCount_.load());

    return DCAMERA_OK;
}

int32_t MockSinkChannel::ReceiveSourceCommand(const std::string& command,
                                              const std::string& params)
{
    if (!isInitialized_) {
        DHLOGE("[SINK] MockSinkChannel not initialized");
        return DCAMERA_BAD_VALUE;
    }

    {
        std::lock_guard<std::mutex> lock(commandMutex_);
        receivedCommands_.push_back(command);
    }

    DHLOGI("[SINK] Received command from Source: %{public}s, params: %{public}s",
           command.c_str(), params.c_str());

    receiveCount_.fetch_add(1);
    return DCAMERA_OK;
}

std::vector<std::string> MockSinkChannel::GetReceivedCommands() const
{
    std::lock_guard<std::mutex> lock(commandMutex_);
    return receivedCommands_;
}

void MockSinkChannel::ClearCommandHistory()
{
    std::lock_guard<std::mutex> lock(commandMutex_);
    receivedCommands_.clear();
    DHLOGI("[SINK] Command history cleared");
}

void MockSinkChannel::SetReceiveDelayMs(int32_t delayMs)
{
    receiveDelayMs_ = delayMs;
    DHLOGI("[SINK] Receive delay set to: %{public}d ms", delayMs);
}

} // namespace DistributedHardware
} // namespace OHOS
