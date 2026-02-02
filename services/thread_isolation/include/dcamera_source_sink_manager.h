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

#ifndef OHOS_DCAMERA_SOURCE_SINK_MANAGER_H
#define OHOS_DCAMERA_SOURCE_SINK_MANAGER_H

#include <memory>
#include <string>
#include <map>
#include <mutex>

#include "dcamera_thread_isolation.h"
#include "icamera_channel.h"
#include "data_buffer.h"

namespace OHOS {
namespace DistributedHardware {

class DCameraSourceSinkManager {
public:
    static DCameraSourceSinkManager& GetInstance();

    // 初始化Source端
    int32_t InitSource(const std::string& params);

    // 初始化Sink端
    int32_t InitSink(const std::string& params);

    // 釋放Source端
    int32_t ReleaseSource();

    // 釋放Sink端
    int32_t ReleaseSink();

    // 在Source線程中執行任務
    void PostSourceTask(std::function<void()> task);

    // 在Sink線程中執行任務
    void PostSinkTask(std::function<void()> task);

    // 發送數據到對端
    int32_t SendData(DCameraSessionMode mode, std::shared_ptr<DataBuffer>& buffer);

    // 設置通道監聽器
    void SetChannelListener(std::shared_ptr<ICameraChannelListener>& listener);

    // 檢查是否已初始化
    bool IsSourceInitialized() const;
    bool IsSinkInitialized() const;

private:
    DCameraSourceSinkManager() = default;
    ~DCameraSourceSinkManager() = default;

    // 禁止拷貝和移動
    DISALLOW_COPY_AND_MOVE(DCameraSourceSinkManager);

    std::unique_ptr<DCameraThreadIsolation> sourceThread_;
    std::unique_ptr<DCameraThreadIsolation> sinkThread_;

    std::atomic<bool> sourceInitialized_{false};
    std::atomic<bool> sinkInitialized_{false};

    std::shared_ptr<ICameraChannelListener> channelListener_;
    mutable std::mutex listenerMutex_;
};

} // namespace DistributedHardware
} // namespace OHOS

#endif // OHOS_DCAMERA_SOURCE_SINK_MANAGER_H