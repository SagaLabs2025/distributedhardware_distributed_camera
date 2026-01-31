/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef SOURCE_SERVICE_IMPL_H
#define SOURCE_SERVICE_IMPL_H

#include "distributed_camera_source.h"
#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>

// 前向声明
class SocketReceiver;

class SourceServiceImpl : public IDistributedCameraSource {
public:
    SourceServiceImpl();
    ~SourceServiceImpl() override;

    // IDistributedCameraSource 实现
    int32_t InitSource(const std::string& params, ISourceCallback* callback) override;
    int32_t ReleaseSource() override;
    int32_t RegisterDistributedHardware(const std::string& devId, const std::string& dhId) override;
    int32_t UnregisterDistributedHardware(const std::string& devId, const std::string& dhId) override;
    int32_t StartCapture() override;
    int32_t StopCapture() override;

private:
    ISourceCallback* callback_;
    std::unique_ptr<SocketReceiver> socketReceiver_;

    std::mutex mutex_;
    std::atomic<bool> initialized_;
    std::atomic<bool> capturing_;

    std::string deviceId_;
    std::string dhId_;
};

#endif // SOURCE_SERVICE_IMPL_H
