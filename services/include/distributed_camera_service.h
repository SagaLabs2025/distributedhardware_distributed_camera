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

#ifndef OHOS_DISTRIBUTED_CAMERA_SERVICE_H
#define OHOS_DISTRIBUTED_CAMERA_SERVICE_H

#include <string>
#include <memory>
#include "data_buffer.h"

namespace OHOS {
namespace DistributedHardware {

// Callback interfaces
class IDCameraSourceCallback {
public:
    virtual ~IDCameraSourceCallback() = default;
    virtual void OnSourceError(int32_t errorCode, const std::string& errorMsg) = 0;
    virtual void OnSourceEvent(const std::string& event) = 0;
};

class IDCameraSinkCallback {
public:
    virtual ~IDCameraSinkCallback() = default;
    virtual void OnSinkError(int32_t errorCode, const std::string& errorMsg) = 0;
    virtual void OnSinkEvent(const std::string& event) = 0;
    virtual void OnVideoDataReceived(std::shared_ptr<DataBuffer> buffer) = 0;
};

// Source Service Interface
class IDistributedCameraSource {
public:
    virtual ~IDistributedCameraSource() = default;
    virtual int32_t InitSource(const std::string& params,
                              std::shared_ptr<IDCameraSourceCallback> callback) = 0;
    virtual int32_t ReleaseSource() = 0;
    virtual int32_t RegisterDistributedHardware(const std::string& devId,
                                              const std::string& dhId,
                                              const std::string& reqId,
                                              const std::string& param) = 0;
    virtual int32_t UnregisterDistributedHardware(const std::string& devId,
                                                const std::string& dhId,
                                                const std::string& reqId) = 0;
    virtual int32_t DCameraNotify(const std::string& devId,
                                 const std::string& dhId,
                                 std::string& events) = 0;
};

// Sink Service Interface
class IDistributedCameraSink {
public:
    virtual ~IDistributedCameraSink() = default;
    virtual int32_t InitSink(const std::string& params,
                            std::shared_ptr<IDCameraSinkCallback> callback) = 0;
    virtual int32_t ReleaseSink() = 0;
    virtual int32_t SubscribeLocalHardware(const std::string& dhId,
                                         const std::string& parameters) = 0;
    virtual int32_t UnsubscribeLocalHardware(const std::string& dhId) = 0;
    virtual int32_t StopCapture(const std::string& dhId) = 0;
    virtual int32_t ChannelNeg(const std::string& dhId, std::string& channelInfo) = 0;
    virtual int32_t GetCameraInfo(const std::string& dhId, std::string& cameraInfo) = 0;
    virtual int32_t OpenChannel(const std::string& dhId, std::string& openInfo) = 0;
    virtual int32_t CloseChannel(const std::string& dhId) = 0;
};

// Service factory
class DistributedCameraServiceFactory {
public:
    static std::shared_ptr<IDistributedCameraSource> CreateSourceService();
    static std::shared_ptr<IDistributedCameraSink> CreateSinkService();
};

} // namespace DistributedHardware
} // namespace OHOS

#endif // OHOS_DISTRIBUTED_CAMERA_SERVICE_H