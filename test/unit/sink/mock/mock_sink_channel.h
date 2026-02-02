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

#ifndef OHOS_MOCK_SINK_CHANNEL_H
#define OHOS_MOCK_SINK_CHANNEL_H

#include <memory>
#include <vector>
#include <string>
#include <atomic>
#include <mutex>

#include "dcamera_protocol.h"
#include "data_buffer.h"

namespace OHOS {
namespace DistributedHardware {

/**
 * @brief Mock Sink端通道类
 *
 * 模拟Channel模块接收Source端命令的功能
 */
class MockSinkChannel {
public:
    MockSinkChannel();
    ~MockSinkChannel();

    /**
     * @brief 初始化通道
     */
    int32_t Init();

    /**
     * @brief 释放通道
     */
    int32_t Release();

    /**
     * @brief 发送数据到Source端
     * @param buffer 数据缓冲区
     * @return 成功返回DCAMERA_OK，失败返回错误码
     */
    int32_t SendData(std::shared_ptr<DataBuffer>& buffer);

    /**
     * @brief 接收来自Source端的命令数据
     * @param buffer 输出参数，接收到的数据
     * @return 成功返回DCAMERA_OK，失败返回错误码
     */
    int32_t ReceiveData(std::shared_ptr<DataBuffer>& buffer);

    /**
     * @brief 模拟接收Source端的控制命令
     * @param command 命令类型
     * @param params 命令参数
     * @return 成功返回DCAMERA_OK
     */
    int32_t ReceiveSourceCommand(const std::string& command,
                                 const std::string& params);

    /**
     * @brief 获取接收到的命令列表（用于测试验证）
     */
    std::vector<std::string> GetReceivedCommands() const;

    /**
     * @brief 清空命令历史
     */
    void ClearCommandHistory();

    /**
     * @brief 设置接收延迟（模拟网络延迟）
     */
    void SetReceiveDelayMs(int32_t delayMs);

    /**
     * @brief 获取发送数据次数
     */
    int32_t GetSendCount() const { return sendCount_.load(); }

    /**
     * @brief 获取接收数据次数
     */
    int32_t GetReceiveCount() const { return receiveCount_.load(); }

private:
    std::atomic<int32_t> sendCount_{0};
    std::atomic<int32_t> receiveCount_{0};
    std::vector<std::string> receivedCommands_;
    mutable std::mutex commandMutex_;
    int32_t receiveDelayMs_{0};
    bool isInitialized_{false};
};

} // namespace DistributedHardware
} // namespace OHOS

#endif // OHOS_MOCK_SINK_CHANNEL_H
